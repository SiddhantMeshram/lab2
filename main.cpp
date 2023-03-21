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

using namespace std;

vector<int> _randvals;
int _ofs = 0;
void PopulateRandArray(const string& rand_file) {

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

int MyRandom(int burst) {
  if (_ofs >= _randvals.size()) {
    _ofs = 0;
  }
  return 1 + (_randvals[_ofs++] % burst);
}

class Process {
  public: 
    Process(int arrival_time, int total_cpu_time, int max_cpu_burst,
            int max_io_burst, int maxprio) :
      pid (count++),
      arrival_time(arrival_time),
      total_cpu_time(total_cpu_time),
      max_cpu_burst(max_cpu_burst),
      max_io_burst(max_io_burst),
      static_priority(MyRandom(maxprio)),
      current_state(ProcessState::CREATED),
      state_transition_ts(arrival_time),
      remaining_cpu_time(total_cpu_time),
      time_in_blocked(0),
      cpu_wait_time(0),
      current_cpu_burst(0),
      dynamic_priority(static_priority - 1) {}

    enum ProcessState{
      CREATED=0,
      READY,
      RUNNING,
      BLOCKED
    };

    static string ProcessStateToString(int ps) {

      switch (ps) {
      case CREATED:
        return "CREATED";
      case READY:
        return "READY";
      case RUNNING:
        return "RUNNG";
      case BLOCKED:
        return "BLOCK";
      }

      return "";
    }

    void ComputePostCompletionStats(int finish_time) {
      this->finish_time = finish_time;
      this->turnaround_time = finish_time - this->arrival_time;
    }

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

class Scheduler {
  public:
    virtual void AddProcess(shared_ptr<Process> process) = 0;

    virtual shared_ptr<Process> GetNextProcess() = 0;
};

class Fcfs: public Scheduler {
  private:
    queue<shared_ptr<Process>> run_queue;
  public:
    void AddProcess(shared_ptr<Process> process) {
      run_queue.push(process);
    }

    shared_ptr<Process> GetNextProcess() {
      if (run_queue.size() == 0) {
        return nullptr;
      }
      auto process = run_queue.front();
      run_queue.pop();
      return process;
    }
};

class Lcfs: public Scheduler {
  private:
    deque<shared_ptr<Process>> run_queue;
  public:
    void AddProcess(shared_ptr<Process> process) {
      run_queue.push_back(process);
    }

    shared_ptr<Process> GetNextProcess() {
      if (run_queue.size() == 0) {
        return nullptr;
      }
      auto process = run_queue.back();
      run_queue.pop_back();
      return process;
    }
};

class Srtf: public Scheduler {
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
                   Comparator> run_queue;
  
  public:
    void AddProcess(shared_ptr<Process> process) {
      run_queue.push(process);
    }

    shared_ptr<Process> GetNextProcess() {
      if (run_queue.size() != 0) {
        shared_ptr<Process> p = run_queue.top();
        run_queue.pop();
        return p;
      }

      return nullptr;
    }
};

class RoundRobin: public Scheduler {
  private:
    queue<shared_ptr<Process>> run_queue;
  public:
    void AddProcess(shared_ptr<Process> process) {
      run_queue.push(process);
    }

    shared_ptr<Process> GetNextProcess() {
      if (run_queue.size() == 0) {
        return nullptr;
      }
      auto process = run_queue.front();
      run_queue.pop();
      return process;
    }
};

class Prio: public Scheduler {
  private:
    vector<queue<shared_ptr<Process>>> active_queue;
    vector<queue<shared_ptr<Process>>> expired_queue;

    int maxprio;

  public:
    Prio (int maxprio) :
      maxprio(maxprio) {

      active_queue.resize(maxprio);
      expired_queue.resize(maxprio);
    }

    void AddProcess(shared_ptr<Process> process) {
      if (process->dynamic_priority == -1) {
        process->dynamic_priority = process->static_priority - 1;
        expired_queue[process->dynamic_priority].push(process);
        return;
      }

      active_queue[process->dynamic_priority].push(process);
    }

    shared_ptr<Process> GetInActiveQueue() {
      for (int ii = active_queue.size() - 1; ii >= 0; --ii) {
        if (active_queue[ii].size() != 0) {
          shared_ptr<Process> p = active_queue[ii].front();
          active_queue[ii].pop();
          return p;
        }
      }

      return nullptr;
    }

    shared_ptr<Process> GetNextProcess() {
      shared_ptr<Process> p = GetInActiveQueue();
      if (p) {
        return p;
      }

      // If we reach here, it means active_queue is empty. Replace it with
      // expired queue.
      active_queue = expired_queue;
      expired_queue.clear();
      expired_queue.resize(maxprio);
      
      return GetInActiveQueue();
    }
};

class PrePrio: public Scheduler {
  private:
    vector<queue<shared_ptr<Process>>> active_queue;
    vector<queue<shared_ptr<Process>>> expired_queue;

    int maxprio;

  public:
    PrePrio (int maxprio) :
      maxprio(maxprio) {

      active_queue.resize(maxprio);
      expired_queue.resize(maxprio);
    }

    void AddProcess(shared_ptr<Process> process) {
      if (process->dynamic_priority == -1) {
        process->dynamic_priority = process->static_priority - 1;
        expired_queue[process->dynamic_priority].push(process);
        return;
      }

      active_queue[process->dynamic_priority].push(process);
    }

    shared_ptr<Process> GetInActiveQueue() {
      for (int ii = active_queue.size() - 1; ii >= 0; --ii) {
        if (active_queue[ii].size() != 0) {
          shared_ptr<Process> p = active_queue[ii].front();
          active_queue[ii].pop();
          return p;
        }
      }

      return nullptr;
    }

    shared_ptr<Process> GetNextProcess() {
      shared_ptr<Process> p = GetInActiveQueue();
      if (p) {
        return p;
      }

      // If we reach here, it means active_queue is empty. Replace it with
      // expired queue.
      active_queue = expired_queue;
      expired_queue.clear();
      expired_queue.resize(maxprio);
      
      return GetInActiveQueue();
    }
};

class Event {
  public:
    Event(int timestamp, shared_ptr<Process> process, int old_process_state,
          int new_process_state) :
      timestamp(timestamp),
      process(process),
      old_process_state(old_process_state),
      new_process_state(new_process_state),
      event_id(count++) {}
  
    enum TransitionState {
      TRANS_TO_READY = 0,
      TRANS_TO_PREEMPT,
      TRANS_TO_RUN,
      TRANS_TO_BLOCK
    };

    int GetTransition() {
      if (old_process_state == Process::ProcessState::CREATED &&
          new_process_state == Process::ProcessState::READY) {
        return TRANS_TO_READY;
      } else if (old_process_state == Process::ProcessState::READY &&
                 new_process_state == Process::ProcessState::RUNNING) {
        return TRANS_TO_RUN;
      } else if (old_process_state == Process::ProcessState::RUNNING &&
                 new_process_state == Process::ProcessState::BLOCKED) {
        return TRANS_TO_BLOCK;
      } else if (old_process_state == Process::ProcessState::BLOCKED &&
                 new_process_state == Process::ProcessState::READY) {
        return TRANS_TO_READY;
      } else if (old_process_state == Process::ProcessState::RUNNING &&
                 new_process_state == Process::ProcessState::READY) {
        return TRANS_TO_PREEMPT;
      }

      return -1;
    }

  public:
    static int count;

    int timestamp;
    shared_ptr<Process> process;
    int old_process_state;
    int new_process_state;
    int event_id;
};

class Des {
  private:
    struct Comparator {
      bool operator() (const shared_ptr<Event> event1,
                       const shared_ptr<Event> event2) const {
        if (event1->timestamp != event2->timestamp) {
          return event1->timestamp < event2->timestamp;
        } else {
          // To honour stability, ie. event getting queued first will be
          // executed before the event which gets queued second.
          return event1->event_id < event2->event_id;
        }
      }
    };

    // Using a set to act as a priority queue with the help of a comparator.
    set<shared_ptr<Event>, Comparator> event_q;

  public:
    shared_ptr<Event> GetEvent() {
      if (event_q.size() != 0) {
        shared_ptr<Event> e = *(event_q.begin());
        event_q.erase(event_q.begin());
        return e;
      }

      return nullptr;
    }

    void AddEvent(shared_ptr<Event> event) {
      event_q.insert(event);
    }

    int GetNextEventTime() {
      if (event_q.size() == 0) {
        return -1;
      }
      return (*(event_q.begin()))->timestamp;
    }

    int GetNextEventTime(int pid) {
      for (auto itr = event_q.begin(); itr != event_q.end(); ++itr) {
        if ((*itr)->process->pid == pid) {
          return (*itr)->timestamp;
          break;
        }
      }
      
      return -1;
    }

    void RmEvent(int pid) {
      for (auto itr = event_q.begin(); itr != event_q.end(); ++itr) {
        if ((*itr)->process->pid == pid) {
          event_q.erase(itr);
          break;
        }
      }
    }
};

//-----------------------------------------------------------------------------

// Global variables.
shared_ptr<Scheduler> _scheduler = nullptr;
deque<shared_ptr<Process>> _process_queue;
Des _des;
bool _verbose = false;
bool _preemption_logs = false;
shared_ptr<Process> _current_running_process = nullptr;
int Event::count = 0;
int Process::count = 0;
int _total_blocked_count = 0;
int _block_start_time = 0;
int _total_block_time = 0;
int _quantum = 10000;
int _maxprio = 4;
bool _is_prio_scheduler = false;
bool _is_pre_prio_scheduler = false;

void ReadInput(const string& input_file, const int maxprio) {

  ifstream input(input_file);
  string line, word;
  while(getline(input, line)) {
    istringstream iss(line);
    int arrival_time, total_cpu_time, max_cpu_burst, max_io_burst;
    iss >> arrival_time >> total_cpu_time >> max_cpu_burst >> max_io_burst;

    shared_ptr<Process> process = make_shared<Process>(
      arrival_time, total_cpu_time, max_cpu_burst, max_io_burst,
      maxprio);
    _process_queue.push_back(process);
  }

  input.close();
}

//-----------------------------------------------------------------------------

string GetCommonLogString(int current_time, int pid, int time_in_prev_state,
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

void Simulate() {
  shared_ptr<Event> event;
  bool call_scheduler = false;
  while ((event = _des.GetEvent())) {
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
          
          int next_event_ts = _des.GetNextEventTime(
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
            _des.RmEvent(_current_running_process->pid);
            shared_ptr<Event> new_event = make_shared<Event>(
              current_time, _current_running_process,
              _current_running_process->current_state,
              Process::ProcessState::READY);
            _des.AddEvent(new_event);
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
        _des.AddEvent(new_event);
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
          _des.AddEvent(new_event);
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
      if (_des.GetNextEventTime() == current_time) {
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
        _des.AddEvent(new_event);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void PrintOutput(string scheduler_type) {

  if (scheduler_type == "F") {
    cout << "FCFS" << endl;
  } else if (scheduler_type == "L") {
    cout << "LCFS" << endl;
  } else if (scheduler_type == "S") {
    cout << "SRTF" << endl;
  } else if (scheduler_type[0] == 'R') {
    cout << "RR " << _quantum << endl;
  } else if (scheduler_type[0] == 'P') {
    cout << "PRIO " << _quantum << endl;
  } else if (scheduler_type[0] == 'E') {
    cout << "PREPRIO " << _quantum << endl;
  }

  int total_cpu_usage = 0, total_io_usage = 0, total_cpu_wait_time = 0;
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

//-----------------------------------------------------------------------------

int main(int argc, char *argv[]) {

  string scheduler_type;
  int c;
  while((c = getopt(argc, argv, "vps:")) != -1)
    switch (c) {
      case 'v':
        _verbose = true;
        break;
      case 'p':
        _preemption_logs = true;
        break;
      case 's':
        scheduler_type = optarg;
        if (scheduler_type == "F") {
          _scheduler = make_shared<Fcfs>();
        } else if (scheduler_type == "L") {
          _scheduler = make_shared<Lcfs>();
        } else if (scheduler_type == "S") {
          _scheduler = make_shared<Srtf>();
        } else if (scheduler_type[0] == 'R') {
          _scheduler = make_shared<RoundRobin>();
          _quantum = stoi(scheduler_type.substr(1));
        } else if (scheduler_type[0] == 'P' || scheduler_type[0] == 'E') {
          _is_prio_scheduler = true;
          if (scheduler_type[0] == 'E') {
            _is_pre_prio_scheduler = true;
          }
          stringstream ss(scheduler_type.substr(1));
          string temp_str;
          getline(ss, temp_str, ':');
          _quantum = stoi(temp_str);
          if (!ss.eof()) {
            getline(ss, temp_str, ':');
            _maxprio = stoi(temp_str);
          }
          if (scheduler_type[0] == 'E') {
            _is_pre_prio_scheduler = true;
            _scheduler = make_shared<PrePrio>(_maxprio);
          } else {
            _scheduler = make_shared<Prio>(_maxprio);
          }
        }
        break;
      case '?':
        if (optopt == 's')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
    }

  const string& input_file = argv[optind];
  const string& rand_file = argv[optind + 1];  

  PopulateRandArray(rand_file);

  ReadInput(input_file, _maxprio);

  for (const auto& process : _process_queue) {
    shared_ptr<Event> event = make_shared<Event>(
      process->arrival_time, process, process->current_state,
      Process::ProcessState::READY);
    _des.AddEvent(event);
  }

  Simulate();

  PrintOutput(scheduler_type);
}