#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include <memory>
#include <queue>

#include "scheduler.h"

using namespace std;

class RoundRobin: public Scheduler {
  public:
    void AddProcess(shared_ptr<Process> process);

    shared_ptr<Process> GetNextProcess();
  
  private:
    queue<shared_ptr<Process>> _run_queue;
};

#endif