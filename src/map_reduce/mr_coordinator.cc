#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>

#include "map_reduce/protocol.h"

namespace mapReduce {


    AskTaskResponse Coordinator::askTask(int) {
    auto task = findTask();
    AskTaskResponse response;

    if (task) {
        response.index = task->index;
        response.tasktype = static_cast<mr_tasktype>(task->taskType);
        response.filename = this->files[response.index];
        response.nfiles = files.size();
        // std::cout << "coordinator: assign file " << response.filename << " to worker" << std::endl;
    } else {
        // std::cout << "coordinator: no available tasks" << std::endl;
        response.index = -1;
        response.tasktype = NONE;
        response.filename = "";
    }

    return response;
}

    std::optional<Task> Coordinator::findTask() {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (this->completedMapCount < long(this->mapTasks.size())) {
        for (Task &task : this->mapTasks) {
            if (!task.isCompleted && !task.isAssigned) {
                task.isAssigned = true;
                return task;
            }
        }
    } else {
        for (Task &task : this->reduceTasks) {
            if (!task.isCompleted && !task.isAssigned) {
                task.isAssigned = true;
                return task;
            }
        }
    }
    return std::nullopt; // No available task
}

    int Coordinator::submitTask(int taskType, int index) {
    std::unique_lock<std::mutex> lock(this->mtx);
    switch (taskType) {
        case MAP:
            // std::cout << "A worker is trying to submit a map task with id: " << index <<std:: endl;
            mapTasks[index].isCompleted = true;
            mapTasks[index].isAssigned = false;
            this->completedMapCount++;
            break;
        case REDUCE:
            reduceTasks[index].isCompleted = true;
            reduceTasks[index].isAssigned = false;
            this->completedReduceCount++;
            break;
        default:
            break;
    }
    if (this->completedMapCount >= (long) mapTasks.size() && this->completedReduceCount >= (long) reduceTasks.size()) {
        this->isFinished = true;
    }
    // std::cout << "coordinator: submit succeeded" << std::endl;
    return 0;
    }

    // mr_coordinator calls Done() periodically to find out
    // if the entire job has finished.
    bool Coordinator::Done() {
        std::unique_lock<std::mutex> uniqueLock(this->mtx);
        return this->isFinished;
    }

    // create a Coordinator.
    // nReduce is the number of reduce tasks to use.
    Coordinator::Coordinator(MR_CoordinatorConfig config, const std::vector<std::string> &files, int nReduce) {
        this->files = files;
        this->isFinished = false;
        // Lab4: Your code goes here (Optional).

        this->completedMapCount = 0;
        this->completedReduceCount = 0;

        int filesize = files.size();
        for (int i = 0; i < filesize; i++) {
            this->mapTasks.push_back(Task{mr_tasktype::MAP, false, false, i});
        }
        for (int i = 0; i < nReduce; i++) {
            this->reduceTasks.push_back(Task{mr_tasktype::REDUCE, false, false, i});
        }

        rpc_server = std::make_unique<chfs::RpcServer>(config.ip_address, config.port);
        rpc_server->bind(ASK_TASK, [this](int i) { return this->askTask(i); });
        rpc_server->bind(SUBMIT_TASK, [this](int taskType, int index) { return this->submitTask(taskType, index); });
        rpc_server->run(true, 1);
    }
}