#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include "librpc/client.h"
#include "librpc/server.h"
#include "distributed/client.h"
#include "rpc/msgpack.hpp"

//Lab4: Free to modify this file

namespace mapReduce {
    struct KeyVal {
        KeyVal(const std::string &key, const std::string &val) : key(key), val(val) {}
        KeyVal(){}
        std::string key;
        std::string val;
    };

    enum mr_tasktype {
        NONE = 0,
        MAP,
        REDUCE
    };
#define REDUCER_COUNT 4
    struct Task {
    int taskType;     // should be either Mapper or Reducer
    bool isAssigned;  // has been assigned to a worker
    bool isCompleted; // has been finised by a worker
    int index;        // index to the file
};

    struct AskTaskResponse {
        // Lab2: Your definition here.
        int tasktype;
        int index;
        int nfiles; // reducer need to know this.
        std::string filename;
        MSGPACK_DEFINE(index, tasktype, filename, nfiles);
    };
    std::vector<KeyVal> Map(const std::string &content);
    std::string Reduce(const std::string &key, const std::vector<std::string> &values);

    const std::string ASK_TASK = "ask_task";
    const std::string SUBMIT_TASK = "submit_task";
  
    struct MR_CoordinatorConfig {
        uint16_t port;
        std::string ip_address;
        std::string resultFile;
        std::shared_ptr<chfs::ChfsClient> client;

        MR_CoordinatorConfig(std::string ip_address, uint16_t port, std::shared_ptr<chfs::ChfsClient> client,
                             std::string resultFile) : port(port), ip_address(std::move(ip_address)),
                                                       resultFile(resultFile), client(std::move(client)) {}
    };

    class SequentialMapReduce {
    public:
        SequentialMapReduce(std::shared_ptr<chfs::ChfsClient> client, const std::vector<std::string> &files, std::string resultFile);
        void doWork();

    private:
        std::shared_ptr<chfs::ChfsClient> chfs_client;
        std::vector<std::string> files;
        std::string outPutFile;
    };

    class Coordinator {
    public:
        Coordinator(MR_CoordinatorConfig config, const std::vector<std::string> &files, int nReduce);
        AskTaskResponse askTask(int);
        int submitTask(int taskType, int index);
        bool Done();
        std::optional<Task> findTask();

    private:
        std::vector<std::string> files;
        std::mutex mtx;
        bool isFinished;
        std::unique_ptr<chfs::RpcServer> rpc_server;
        // lab4: Your code goes here.
        int completedMapCount;
        int completedReduceCount;
        std::vector<Task> mapTasks;
        std::vector<Task> reduceTasks;

    };

    class Worker {
    public:
        explicit Worker(MR_CoordinatorConfig config);
        void doWork();
        void stop();
        std::string readFileContent(const std::string& filename);
        void writeWithAppend(const std::string& filename, const std::string& newData);
    private:
        void doMap(int index, const std::string &filename);
        void doReduce(int index, int nfiles);
        void doSubmit(mr_tasktype taskType, int index);

        std::string outPutFile;
        std::unique_ptr<chfs::RpcClient> mr_client;
        std::shared_ptr<chfs::ChfsClient> chfs_client;
        std::unique_ptr<std::thread> work_thread;
        bool shouldStop = false;
        int working = false;
        std::mutex mtx;
        int id;
    };
}