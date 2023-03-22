#include <string>

#include "process.h"

using namespace std;

int Process::count = 0;

Process::Process(int arrival_time, int total_cpu_time, int max_cpu_burst,
                 int max_io_burst, int static_priority) :
  pid (count++),
  arrival_time(arrival_time),
  total_cpu_time(total_cpu_time),
  max_cpu_burst(max_cpu_burst),
  max_io_burst(max_io_burst),
  static_priority(static_priority),
  current_state(ProcessState::CREATED),
  time_in_blocked(0),
  cpu_wait_time(0),
  dynamic_priority(static_priority - 1),
  state_transition_ts(arrival_time),
  current_cpu_burst(0),
  remaining_cpu_time(total_cpu_time) {}

//-----------------------------------------------------------------------------

string Process::ProcessStateToString(const int& ps) {

    switch (ps) {
    case ProcessState::CREATED:
    return "CREATED";
    case ProcessState::READY:
    return "READY";
    case ProcessState::RUNNING:
    return "RUNNG";
    case ProcessState::BLOCKED:
    return "BLOCK";
    }

    return "";
}

//-----------------------------------------------------------------------------

void Process::ComputePostCompletionStats(int finish_time) {

  this->finish_time = finish_time;
  this->turnaround_time = finish_time - this->arrival_time;
}