//-----------------------------------------------------------------------------
//           Name: processpool.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#pragma once

#include <Utility/disallow_copy_and_assign.h>

#include <string>
#include <vector>
#include <queue>
#include <map>

using std::map;
using std::queue;
using std::string;
using std::vector;

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
class OSProcess {
   public:
    OSProcess(const string& worker_path);
    ~OSProcess();

    string WaitForChildMessage();
    void SendMessageToChild(const string& msg);

   private:
    HANDLE read_pipe_;
    HANDLE write_pipe_;
    PROCESS_INFORMATION process_info_;

    DISALLOW_COPY_AND_ASSIGN(OSProcess);
};
#else
#include <pthread.h>
#include <unistd.h>
class OSProcess {
   public:
    OSProcess(const string& worker_path);
    ~OSProcess();

    string WaitForChildMessage();
    void SendMessageToChild(const string& msg);

   private:
    int read_pipe_;
    int write_pipe_;
    int process_info_;

    DISALLOW_COPY_AND_ASSIGN(OSProcess);
};
#endif  //_WIN32

class ProcessPool;
class ProcessHandle {
   public:
    void Process(const string& task);
    inline bool idle() { return idle_; }
    void ProcessInBackground();

    ProcessHandle(const string& worker_path, ProcessPool* parent_process_pool);

   private:
    bool idle_;
    OSProcess os_process_;
    ProcessPool* parent_process_pool_;

    bool ProcessMessageFromChild(const string& msg);
    DISALLOW_COPY_AND_ASSIGN(ProcessHandle);
};

class ProcessPool {
   public:
    // Change number of process handlers
    void Resize(int _size);

    typedef int (*JobFunctionPtr)(int argc, const char* argv[]);
    typedef map<string, ProcessPool::JobFunctionPtr> JobMap;
    enum Error { SUCCESS = 0,
                 NO_IDLE_PROCESS = -1,
                 NO_TASK_IN_QUEUE = -2 };
    static bool AmIAWorkerProcess(int argc, char* argv[]);
    static int WorkerProcessMain(const JobMap& job_map);

    void NotifyTaskComplete();
    void Schedule(const string& task);
    void WaitForTasksToComplete();
    void ClearQueuedTasks();
    int NumProcesses();

    ProcessPool(const string& worker_path, int size = 0);
    ~ProcessPool();

   private:
    // Attempts to start processing the first task in the queue in the first
    // idle process. Can return SUCCESS, NO_IDLE_PROCESS or NO_TASK_IN_QUEUE.
    ProcessPool::Error ProcessFirstTaskInQueue();

    // Returns index of the first idle process in pool, or -1 if there
    // are no idle processes
    int GetIdleProcessIndex();

    string worker_path_;
    vector<ProcessHandle*> processes_;
    queue<string> tasks_;
#ifdef _WIN32
    HANDLE mutex_;
    HANDLE idle_event_;
#else
    pthread_mutex_t mutex_;
    pthread_mutex_t idle_event_mutex_;
    pthread_cond_t idle_event_cond_;
    bool idle_event_bool_;
#endif

    DISALLOW_COPY_AND_ASSIGN(ProcessPool);
};
