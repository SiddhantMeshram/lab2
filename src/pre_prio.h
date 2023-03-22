#ifndef PRE_PRIO_H
#define PRE_PRIO_H

#include <memory>
#include <queue>
#include <vector>

#include "process.h"
#include "scheduler.h"

using namespace std;

class PrePrio: public Scheduler {
  public:
    PrePrio (int maxprio);

    void AddProcess(shared_ptr<Process> process);

    shared_ptr<Process> GetNextProcess();
  
  private:
    shared_ptr<Process> GetInActiveQueue();
    
    vector<queue<shared_ptr<Process>>> _active_queue;
    vector<queue<shared_ptr<Process>>> _expired_queue;

    int _maxprio;
};

#endif