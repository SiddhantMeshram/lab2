#ifndef FCFS_H
#define FCFS_H

#include <memory>
#include <queue>

#include "scheduler.h"

using namespace std;

class Fcfs: public Scheduler {
  public:
    void AddProcess(shared_ptr<Process> process);

    shared_ptr<Process> GetNextProcess();
  
  private:
    queue<shared_ptr<Process>> _run_queue;
};

#endif