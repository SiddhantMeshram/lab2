#ifndef SRTF_H
#define SRTF_H

#include <memory>
#include <queue>

#include "process.h"
#include "scheduler.h"

using namespace std;

class Srtf: public Scheduler {
  public:
    void AddProcess(shared_ptr<Process> process);

    shared_ptr<Process> GetNextProcess();
  
  private:
    struct Comparator {
      bool operator() (const shared_ptr<Process> process1,
                       const shared_ptr<Process> process2) const {
        if (process1->remaining_cpu_time != process2->remaining_cpu_time) {
          return process1->remaining_cpu_time > process2->remaining_cpu_time;
        } else {
          // To handle cases where remaining time is same, use the event id
          // to figure out which process was queued first.
          return (process1->last_ready_state_trans_event_id >
                    process2->last_ready_state_trans_event_id);
        }
      }
    };

    priority_queue<shared_ptr<Process>, vector<shared_ptr<Process>>,
                   Comparator> _run_queue;
};

#endif