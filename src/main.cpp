#include <iostream>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <queue>
#include <sstream>
#include <memory>
#include <set>

#include "main.h"
#include "process.h"
#include "fcfs.h"
#include "lcfs.h"
#include "srtf.h"
#include "round_robin.h"
#include "prio.h"
#include "event.h"
#include "des.h"

using namespace std;

//-----------------------------------------------------------------------------

Main::Main(const string& scheduler_type, const string& input_file,
           const string& rand_file, const bool verbose,
           const bool preemption_logs) :
  _scheduler_type(scheduler_type),
  _verbose(verbose),
  _preemption_logs(preemption_logs),
  _quantum(10000),
  _maxprio(4),
  _is_prio_scheduler(false),
  _is_pre_prio_scheduler(false),
  _total_blocked_count(0),
  _block_start_time(0),
  _total_block_time(0),
  _ofs(0),
  _scheduler(nullptr),
  _des(make_shared<Des>()),
  _current_running_process(nullptr) {

  ProcessInput(input_file, rand_file);

  // Populate the event queue with the first event for each process.
  for (const auto& process : _process_queue) {
    shared_ptr<Event> event = make_shared<Event>(
      process->arrival_time, process, process->current_state,
      Process::ProcessState::READY);
    _des->AddEvent(event);
  }

  Simulate();

  PrintOutput();
}

//-----------------------------------------------------------------------------

void Main::ProcessInput(const string& input_file, const string& rand_file) {

  if (_scheduler_type == "F") {
    _scheduler = make_shared<Fcfs>();
  } else if (_scheduler_type == "L") {
    _scheduler = make_shared<Lcfs>();
  } else if (_scheduler_type == "S") {
    _scheduler = make_shared<Srtf>();
  } else if (_scheduler_type[0] == 'R') {
    _scheduler = make_shared<RoundRobin>();
    _quantum = stoi(_scheduler_type.substr(1));
  } else if (_scheduler_type[0] == 'P' || _scheduler_type[0] == 'E') {
    _is_prio_scheduler = true;
    if (_scheduler_type[0] == 'E') {
      _is_pre_prio_scheduler = true;
    }

    stringstream ss(_scheduler_type.substr(1));
    string temp_str;
    getline(ss, temp_str, ':');
    _quantum = stoi(temp_str);
    if (!ss.eof()) {
      getline(ss, temp_str, ':');
      _maxprio = stoi(temp_str);
    }

    if (_scheduler_type[0] == 'E') {
      _is_pre_prio_scheduler = true;
    }

    _scheduler = make_shared<Prio>(_maxprio);
  }

  PopulateRandArray(rand_file);

  ReadInput(input_file, _maxprio);
}

//-----------------------------------------------------------------------------

void Main::PopulateRandArray(const string& rand_file) {

  ifstream input_file(rand_file);
  if (!input_file.is_open()) {
    cout << "Error opening the file: " << rand_file << endl;
    return;
  }

  int number;
  // Take the size of file as input.
  input_file >> number;
  while (input_file >> number) {
    _randvals.push_back(number);
  }

  input_file.close();
}

//-----------------------------------------------------------------------------

int Main::MyRandom(int burst) {
  if (_ofs >= _randvals.size()) {
    _ofs = 0;
  }
  return 1 + (_randvals[_ofs++] % burst);
}

//-----------------------------------------------------------------------------

void Main::ReadInput(const string& input_file, const int maxprio) {

  ifstream input(input_file);
  string line, word;
  while(getline(input, line)) {
    istringstream iss(line);
    int arrival_time, total_cpu_time, max_cpu_burst, max_io_burst;
    iss >> arrival_time >> total_cpu_time >> max_cpu_burst >> max_io_burst;

    shared_ptr<Process> process = make_shared<Process>(
      arrival_time, total_cpu_time, max_cpu_burst, max_io_burst,
      MyRandom(maxprio));
    _process_queue.push_back(process);
  }

  input.close();
}

//-----------------------------------------------------------------------------

string Main::GetCommonLogString(
  int current_time, int pid, int time_in_prev_state,
  int old_process_state = -1,
  int new_process_state = -1) {
  
  stringstream s;
  s << current_time << " " << pid << " " << time_in_prev_state << ": ";
  if (old_process_state != -1) {
    s << Process::ProcessStateToString(old_process_state) << " -> "
      << Process::ProcessStateToString(new_process_state);
  }
  
  return s.str();
}

//-----------------------------------------------------------------------------

void Main::Simulate() {
  shared_ptr<Event> event;
  bool call_scheduler = false;
  while ((event = _des->GetEvent())) {
    shared_ptr<Process> process = event->process;
    int current_time = event->timestamp;
    int time_in_prev_state = current_time - process->state_transition_ts;
    int transition = event->GetTransition();

    switch (transition) {
      case Event::TransitionState::TRANS_TO_READY: {
        if (_verbose) {
          cout << GetCommonLogString(current_time, process->pid,
                                     time_in_prev_state,
                                     event->old_process_state,
                                     event->new_process_state)
               << endl;
        }

        if (event->old_process_state == Process::ProcessState::BLOCKED) {
          process->time_in_blocked += time_in_prev_state;
          --_total_blocked_count;
          if (_total_blocked_count == 0) {
            // If there are no processes in blocked state, calculate the time
            // when atleast one process was blocked.
            _total_block_time += current_time - _block_start_time;
          }
        }

        process->current_state = Process::ProcessState::READY;
        process->state_transition_ts = current_time;
        process->last_ready_state_trans_event_id = event->event_id;
        process->dynamic_priority = process->static_priority - 1;
        _scheduler->AddProcess(process);

        if (_is_pre_prio_scheduler && _current_running_process) {
          bool cond1 = (process->dynamic_priority >
                          _current_running_process->dynamic_priority);
          
          int next_event_ts = _des->GetNextEventTime(
            _current_running_process->pid);
          bool cond2 = (next_event_ts > current_time);
          if (_preemption_logs) {
            cout << "    --> PrioPreempt Cond1=" << cond1 << " " << "Cond2="
                 << cond2 << " ("
                 << next_event_ts - current_time << ")" << " --> "
                 << ((cond1 && cond2) ? "YES" : "NO") << endl;
          }
          if (cond1 && cond2) {
            // There is a process with a higher priority waiting to run. Delete
            // the future event scheduled for currently running process and
            // create a preempt event for current running process at current
            // time.
            _des->RmEvent(_current_running_process->pid);
            shared_ptr<Event> new_event = make_shared<Event>(
              current_time, _current_running_process,
              _current_running_process->current_state,
              Process::ProcessState::READY);
            _des->AddEvent(new_event);
          }
        }

        call_scheduler = true;
        break;
      }
      case Event::TransitionState::TRANS_TO_PREEMPT: {
        process->remaining_cpu_time -= time_in_prev_state;
        process->current_cpu_burst -= time_in_prev_state;
        if (_verbose) {
          cout << GetCommonLogString(current_time, process->pid,
                                     time_in_prev_state,
                                     event->old_process_state,
                                     event->new_process_state)
               << "  cb=" << process->current_cpu_burst
               << " rem=" << process->remaining_cpu_time
               << " prio="
               << (_is_prio_scheduler ? process->dynamic_priority :
                     process->static_priority - 1)
               << endl;
        }

        --process->dynamic_priority;
        process->current_state = Process::ProcessState::READY;
        process->state_transition_ts = current_time;
        _current_running_process = nullptr;
        _scheduler->AddProcess(process);
        call_scheduler = true;
        break;
      }
      case Event::TransitionState::TRANS_TO_RUN: {
        // Compute its burst and schedule its next steps (block or preempt).
        process->cpu_wait_time += time_in_prev_state;
        process->current_state = Process::ProcessState::RUNNING;
        process->state_transition_ts = current_time;
        if (process->current_cpu_burst == 0) {
          // If available cpu burst has expired, get a new cpu burst.
          process->current_cpu_burst = min(MyRandom(process->max_cpu_burst),
                                           process->remaining_cpu_time);
        }

        if (_verbose) {
          cout << GetCommonLogString(current_time, process->pid,
                                     time_in_prev_state,
                                     event->old_process_state,
                                     event->new_process_state)
               << " cb=" << process->current_cpu_burst
               << " rem=" << process->remaining_cpu_time
               << " prio="
               << (_is_prio_scheduler ? process->dynamic_priority :
                     process->static_priority - 1)
               << endl;
        }

        shared_ptr<Event> new_event;
        if (process->current_cpu_burst > _quantum) {
          // The process needs to preempt after the quantum expires.
          new_event = make_shared<Event>(
            current_time + _quantum, process, process->current_state,
            Process::ProcessState::READY);
        } else {
          new_event = make_shared<Event>(
            current_time + process->current_cpu_burst, process,
            process->current_state, Process::ProcessState::BLOCKED);
        }
        _des->AddEvent(new_event);
        break;
      }
      case Event::TransitionState::TRANS_TO_BLOCK: {
        // Get the remaining CPU time by subtracting the previous state
        // transition ts from current time.
        process->remaining_cpu_time -= time_in_prev_state;
        process->current_cpu_burst -= time_in_prev_state;
        if (process->remaining_cpu_time) {
          // Compute io burst and schedule next steps.
          int io_burst = MyRandom(process->max_io_burst);
          if (_verbose) {
            cout << GetCommonLogString(current_time, process->pid,
                                       time_in_prev_state,
                                       event->old_process_state,
                                       event->new_process_state)
                 << "  ib=" << io_burst << " rem="
                 << process->remaining_cpu_time
                 << endl;
          }

          if (_total_blocked_count == 0) {
            // Note down the time of blocking if no other process is currently
            // blocked.
            _block_start_time = current_time;
          }

          ++_total_blocked_count;
          process->current_state = Process::ProcessState::BLOCKED;
          process->state_transition_ts = current_time;
          shared_ptr<Event> new_event = make_shared<Event>(
            current_time + io_burst, process, process->current_state,
            Process::ProcessState::READY);
          _des->AddEvent(new_event);
        } else {
          // Process has finished execution.
          if (_verbose) {
            cout << GetCommonLogString(current_time, process->pid,
                                       time_in_prev_state)
                 << "Done"
                 << endl;
          }
          process->ComputePostCompletionStats(current_time);
        }
        _current_running_process = nullptr;
        call_scheduler = true;
        break;
      }
      default:
        break;
    }

    if (call_scheduler) {
      if (_des->GetNextEventTime() == current_time) {
        continue;
      }

      call_scheduler = false;
      if (_current_running_process == nullptr) {
        _current_running_process = _scheduler->GetNextProcess();
        if (_current_running_process == nullptr) {
          continue;
        }
        // We have a process that can run. Transition it to running state.
        shared_ptr<Event> new_event = make_shared<Event>(
          current_time, _current_running_process,
          _current_running_process->current_state,
          Process::ProcessState::RUNNING);
        _des->AddEvent(new_event);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void Main::PrintOutput() {

  if (_scheduler_type == "F") {
    cout << "FCFS" << endl;
  } else if (_scheduler_type == "L") {
    cout << "LCFS" << endl;
  } else if (_scheduler_type == "S") {
    cout << "SRTF" << endl;
  } else if (_scheduler_type[0] == 'R') {
    cout << "RR " << _quantum << endl;
  } else if (_scheduler_type[0] == 'P') {
    cout << "PRIO " << _quantum << endl;
  } else if (_scheduler_type[0] == 'E') {
    cout << "PREPRIO " << _quantum << endl;
  }

  int total_cpu_usage = 0, total_cpu_wait_time = 0;
  int finish_time = 0, total_turnaround_time = 0;
  for (const auto& process : _process_queue) {
    printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
           process->pid, process->arrival_time, process->total_cpu_time,
           process->max_cpu_burst, process->max_io_burst,
           process->static_priority, process->finish_time,
           process->turnaround_time, process->time_in_blocked,
           process->cpu_wait_time);
    total_cpu_usage += process->total_cpu_time;
    total_cpu_wait_time += process->cpu_wait_time;
    finish_time = max(finish_time, process->finish_time);
    total_turnaround_time += process->turnaround_time; 
  }

  int num_processes = _process_queue.size();
  double cpu_util = 100.0 * (total_cpu_usage / (double) finish_time);
  double io_util = 100.0 * (_total_block_time / (double) finish_time);
  double throughput = 100.0 * (num_processes / (double) finish_time);
  double avg_turnaround_time =
    (total_turnaround_time / (double) num_processes);
  double avg_cpu_wait_time = (total_cpu_wait_time / (double) num_processes);

  printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", finish_time, cpu_util,
         io_util, avg_turnaround_time, avg_cpu_wait_time, throughput);
}