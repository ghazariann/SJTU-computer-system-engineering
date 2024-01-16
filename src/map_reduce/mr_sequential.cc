#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include "map_reduce/protocol.h"

namespace mapReduce {
    SequentialMapReduce::SequentialMapReduce(std::shared_ptr<chfs::ChfsClient> client,
                                             const std::vector<std::string> &files_, std::string resultFile) {
        chfs_client = std::move(client);
        files = files_;
        outPutFile = resultFile;
        // Your code goes here (optional)
    }

    
     void SequentialMapReduce::doWork() {
        std::unordered_map<std::string, std::vector<std::string>> intermediateMap;

        // Map Step
        for (const auto& file : files) {
            // Read file contents using chfs_client
            auto inode_lookup = chfs_client->lookup(1, file);
            auto inode_id = inode_lookup.unwrap();
            auto res_type = chfs_client->get_type_attr(inode_id);
		    auto length = res_type.unwrap().second.size;            
            auto content = chfs_client->read_file(inode_id, 0, length);
            auto content_vec = content.unwrap();
            std::string content_str(content_vec.begin(), content_vec.end());

            // Apply Map function
            auto keyVals = Map(content_str);
            for (const auto& kv : keyVals) {
                intermediateMap[kv.key].push_back(kv.val);
            }
        }

        // Reduce Step and Writing Output
        auto inode_lookup = chfs_client->lookup(1, outPutFile);
       auto inode_id = inode_lookup.unwrap();
        std::vector<uint8_t> output_data;
        for (const auto& pair : intermediateMap) {
            std::string result = Reduce(pair.first, pair.second);
            std::string line = pair.first + " " + result + "\n";
            output_data.insert(output_data.end(), line.begin(), line.end());
        }
        chfs_client->write_file(inode_id, 0, output_data);
    }    
}