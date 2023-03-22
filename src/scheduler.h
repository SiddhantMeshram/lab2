#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>

// Forward Declaration.
class Process;

class Scheduler {
  public:
    virtual void AddProcess(std::shared_ptr<Process> process) = 0;

    virtual std::shared_ptr<Process> GetNextProcess() = 0;
};

#endif