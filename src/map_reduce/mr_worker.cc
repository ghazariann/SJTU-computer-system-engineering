#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "map_reduce/protocol.h"

namespace mapReduce {

std::vector<KeyVal> Map(const std::string &content) {
  std::vector<KeyVal> ret;
  size_t start = 0; // Index to track the start of a word

  for (size_t i = 0; i <= content.length(); ++i) {
    // Check if we have reached the end of a word
    if (i == content.length() || !std::isalpha(content[i])) {
      if (i > start) {
        // Extract the word from 'start' to 'i'
        std::string word = content.substr(start, i - start);
        ret.push_back({word, "1"});
      }
      start = i + 1; // Move 'start' to the next character after the delimiter
    }
  }

  return ret;
}
std::string Reduce(const std::string &key,
                   const std::vector<std::string> &values) {
  // Your code goes here
  int count = 0;
  for (const auto &value : values) {
    count += std::stoi(value);
  }
  return std::to_string(count);
}

int strHash(const std::string &str) {
  unsigned int hashVal = 0;
  for (char ch : str) {
    hashVal = hashVal * 131 + (int)ch;
  }
  return hashVal % REDUCER_COUNT;
}

std::string Worker::readFileContent(const std::string& filename) {
    std::unique_lock<std::mutex> uniqueLock(this->mtx);
    auto inode_lookup = chfs_client->lookup(1, filename);
    if (!inode_lookup.is_ok()) {
        std::cerr << "Error: Unable to find file " << filename << std::endl;
        return "";
    }
    auto inode_id = inode_lookup.unwrap();

    auto res_type = chfs_client->get_type_attr(inode_id);
    if (!res_type.is_ok()) {
        std::cerr << "Error: Unable to get file attributes for " << filename << std::endl;
        return "";
    }
    auto length = res_type.unwrap().second.size;

    auto content = chfs_client->read_file(inode_id, 0, length);
    if (!content.is_ok()) {
        std::cerr << "Error: Unable to read file " << filename << std::endl;
        return "";
    }
    auto content_vec = content.unwrap();
    return std::string(content_vec.begin(), content_vec.end());
}
void Worker::writeWithAppend(const std::string& filename, const std::string& newData) {
    std::unique_lock<std::mutex> uniqueLock(this->mtx);

    // Step 1: Lookup the inode for the filename
    auto inode_lookup = chfs_client->lookup(1, filename);
    if (!inode_lookup.is_ok()) {
        std::cerr << "Error: Unable to find file " << filename << std::endl;
        return;
    }
    auto inode_id = inode_lookup.unwrap();

   // Step 2: Read the existing content if file is not empty
    std::string existing_content;
    auto res_type = chfs_client->get_type_attr(inode_id);
    if (!res_type.is_ok()) {
        std::cerr << "Error: Unable to get file attributes for " << filename << std::endl;
        return;
    }
    auto length = res_type.unwrap().second.size;
    if (length > 0) {
        auto content = chfs_client->read_file(inode_id, 0, length);
        if (!content.is_ok()) {
            std::cerr << "Error: Unable to read file " << filename << std::endl;
            return;
        }
        auto existing_content_vec = content.unwrap();
        existing_content.assign(existing_content_vec.begin(), existing_content_vec.end());
    }

    // Step 3: Append the new data to the existing content
    existing_content += newData;

    // Step 4: Write the combined content back to the file
    std::vector<chfs::u8> contentBytes(existing_content.begin(), existing_content.end());
    chfs_client->write_file(inode_id, 0, contentBytes);
}
Worker::Worker(MR_CoordinatorConfig config) {
  mr_client =
      std::make_unique<chfs::RpcClient>(config.ip_address, config.port, true);
  outPutFile = config.resultFile;
  chfs_client = config.client;
  work_thread = std::make_unique<std::thread>(&Worker::doWork, this);
  // Lab4: Your code goes here (Optional).
}

void Worker::doMap(int index, const std::string &filename) {
  // Lab4: Your code goes here.
  working = true;
  std::string intermediatePrefix =
      "mr-" + std::to_string(index) + "-"; // add basedir

  std::string content_str = readFileContent(filename);
  std::vector<KeyVal> keyVals = Map(content_str);

  std::vector<std::string> contents(REDUCER_COUNT);

  for (const KeyVal &keyVal : keyVals) {
    int reducerId = strHash(keyVal.key);
    contents[reducerId] += keyVal.key + ' ' + keyVal.val + '\n';
  }

  for (int i = 0; i < REDUCER_COUNT; ++i) {
    const std::string &content = contents[i];
    if (!content.empty()) {
      std::string intermediateFilepath = intermediatePrefix + std::to_string(i);
      auto res_create =
          chfs_client->mknode(chfs::ChfsClient::FileType::REGULAR, 1,
                              intermediateFilepath); // change 1 to base dir id
      auto inode_id = res_create.unwrap();
      std::vector<chfs::u8> contentBytes(content.begin(), content.end());
      chfs_client->write_file(inode_id, 0, contentBytes);
    }
  }
}

void Worker::doReduce(int index, int nfiles) {
  // Lab4: Your code goes here.
  std::string filepath;
  std::unordered_map<std::string, unsigned long long> wordFreqs;
  for (int i = 0; i < nfiles; ++i) {
    filepath = "mr-" + std::to_string(i) + '-' + std::to_string(index);
    std::string content_str = readFileContent(filepath);
    std::istringstream contentStream(content_str);
    std::string key, value;
    while (contentStream >> key >> value)
        wordFreqs[key] += std::atoll(value.c_str());
  }

  std::string content;
  for (const std::pair<const std::string, unsigned long long>& keyVal : wordFreqs)
        content += keyVal.first + ' ' + std::to_string(keyVal.second) + '\n';
    writeWithAppend(outPutFile, content);
  working = true;
}

void Worker::doSubmit(mr_tasktype taskType, int index) {
  // Lab4: Your code goes here.
  auto ret = mr_client->call(SUBMIT_TASK, (int)taskType, index);
  if (!ret.is_ok()) {
    fprintf(stderr, "submit task failed\n");
    exit(-1);
  }
  working = false;
}

void Worker::stop() {
  shouldStop = true;
  work_thread->join();
}

void Worker::doWork() {
  while (!shouldStop) {
    AskTaskResponse res;
    if (!working) {
      auto rpcResult = mr_client->call(ASK_TASK, id);
      if (rpcResult.is_ok()) {
        auto rpcResponse = rpcResult.unwrap(); // Extract the RpcResponse

        // Deserialize the AskTaskResponse from rpcResponse
        clmdep_msgpack::v1::object_handle *oh =
            rpcResponse.get();
        (*oh).get().convert(
            res); 
      } else {
        std::cerr << "Error in RPC call" << std::endl;
        sleep(1); 
      }
    }

    if (res.tasktype == MAP) {
    //   std::cout << "worker: receive map task " << res.index << std::endl;
      doMap(res.index, res.filename);
      doSubmit(MAP, res.index);
    } else if (res.tasktype == REDUCE) {
    //   std::cout << "worker: receive reduce task " << res.index << std::endl;
      doReduce(res.index, res.nfiles);
      doSubmit(REDUCE, res.index);
    } else if (res.tasktype == NONE) {
    //   std::cout << "No available task, sleep for 1 second" << std::endl;
      sleep(1);
    }
  }
}
}