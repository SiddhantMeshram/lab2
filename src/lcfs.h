#ifndef LCFS_H
#define LCFS_H

#include <memory>
#include <deque>

#include "scheduler.h"

using namespace std;

class Lcfs: public Scheduler {
  public:
    void AddProcess(shared_ptr<Process> process);

    shared_ptr<Process> GetNextProcess();

  private:
    deque<shared_ptr<Process>> _run_queue;
};

#endif