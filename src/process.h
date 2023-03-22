#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <string>

class Process {
  public: 
    Process(int arrival_time, int total_cpu_time, int max_cpu_burst,
            int max_io_burst, int maxprio);

    enum ProcessState{
      CREATED=0,
      READY,
      RUNNING,
      BLOCKED
    };

    static std::string ProcessStateToString(const int& ps);

    void ComputePostCompletionStats(int finish_time);

  public:
    // Static counter to keep count of number of processes.
    static int count;

    int pid;
    int arrival_time;
    int total_cpu_time;
    int max_cpu_burst;
    int max_io_burst;
    int static_priority;
    int current_state;
    int finish_time;
    int turnaround_time;
    int time_in_blocked;
    int cpu_wait_time;
    int dynamic_priority;

    // Event ID of the last event which transitioned this process to ready
    // state.
    int last_ready_state_trans_event_id;

    // Time at which previous state transition happened.
    int state_transition_ts;

    // Currently available CPU burst for this process.
    int current_cpu_burst;

    // Total amount of work remaining for this process.
    int remaining_cpu_time;
};

#endif