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

using namespace std;

enum ProcessState{
  CREATED=0,
  READY,
  RUNNING,
  BLOCKED
};

string ProcessStateToString(int ps) {

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

vector<int> randvals;
int ofs = 0;
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
    randvals.push_back(number);
  }

  input_file.close();
}

int MyRandom(int burst) {
  if (ofs >= randvals.size()) {
    ofs = 0;
  }
  return 1 + (randvals[ofs++] % burst);
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
      current_cpu_burst(0) {}

  public:
    static int count;
    int pid;
    int arrival_time;
    int total_cpu_time;
    int max_cpu_burst;
    int max_io_burst;
    int static_priority;
    int current_state;
    int state_transition_ts;
    int remaining_cpu_time;
    int finish_time;
    int turnaround_time;
    int time_in_blocked;
    int cpu_wait_time;
    int current_cpu_burst;
    int ready_state_transition_event_id;
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
    class Comparator{
      public:
        bool operator() (const shared_ptr<Process> process1,
                         const shared_ptr<Process> process2) {
          if (process1->remaining_cpu_time != process2->remaining_cpu_time) {
            return process1->remaining_cpu_time > process2->remaining_cpu_time;
          } else {
            // To handle cases where remaining time is same, use the event id
            // to figure out which process was queued first.
            return (process1->ready_state_transition_event_id >
                      process2->ready_state_transition_event_id);
          }
        }
    };

    priority_queue<shared_ptr<Process>, vector<shared_ptr<Process>>,
                   Comparator> run_queue;
    priority_queue<shared_ptr<Process>, vector<shared_ptr<Process>>,
                  Comparator> copy_run_queue;
  
  public:
    void AddProcess(shared_ptr<Process> process) {
      run_queue.push(process);
      // PrintRunQueue();
    }

    shared_ptr<Process> GetNextProcess() {
      if (run_queue.size() != 0) {
        shared_ptr<Process> p = run_queue.top();
        run_queue.pop();
        return p;
      }

      return nullptr;
    }

    void PrintRunQueue() {
      priority_queue<shared_ptr<Process>, vector<shared_ptr<Process>>,
                     Comparator> copy_run_queue(run_queue);
      
      while (!copy_run_queue.empty()) {
        auto p = copy_run_queue.top();
        cout << "PrintRunQueue " << p->pid << " " << p->remaining_cpu_time << endl;
        copy_run_queue.pop();
      }
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

  public:
    static int count;
    int timestamp;
    shared_ptr<Process> process;
    int old_process_state;
    int new_process_state;
    int event_id;
  
  public:
    int GetTransition() {
      if (old_process_state == ProcessState::CREATED &&
          new_process_state == ProcessState::READY) {
        return TRANS_TO_READY;
      } else if (old_process_state == ProcessState::READY &&
                 new_process_state == ProcessState::RUNNING) {
        return TRANS_TO_RUN;
      } else if (old_process_state == ProcessState::RUNNING &&
                 new_process_state == ProcessState::BLOCKED) {
        return TRANS_TO_BLOCK;
      } else if (old_process_state == ProcessState::BLOCKED &&
                 new_process_state == ProcessState::READY) {
        return TRANS_TO_READY;
      } else if (old_process_state == ProcessState::RUNNING &&
                 new_process_state == ProcessState::READY) {
        return TRANS_TO_PREEMPT;
      }

      return -1;
    }
};

class Des {
  private:
    class Comparator{
      public:
        bool operator() (const shared_ptr<Event> event1,
                         const shared_ptr<Event> event2) {
          if (event1->timestamp != event2->timestamp) {
            return event1->timestamp > event2->timestamp;
          } else {
            // To honour stability.
            return event1->event_id > event2->event_id;
          }
        }
    };

    priority_queue<shared_ptr<Event>, vector<shared_ptr<Event>>,
                   Comparator> event_q;

  public:
    shared_ptr<Event> GetEvent() {
      if (event_q.size() != 0) {
        shared_ptr<Event> e = event_q.top();
        event_q.pop();
        return e;
      }

      return nullptr;
    }

    void AddEvent(shared_ptr<Event> event) {
      event_q.push(event);
    }

    int GetNextEventTime() {
      if (event_q.size() == 0) {
        return -1;
      }
      return event_q.top()->timestamp;
    }
};

// Global variables.

shared_ptr<Scheduler> _scheduler = nullptr;
deque<shared_ptr<Process>> _process_queue;
Des _des;
bool _verbose;
shared_ptr<Process> _current_running_process = nullptr;
int Event::count = 0;
int Process::count = 0;
int _total_blocked_count = 0;
int _block_start_time = 0;
int _total_block_time = 0;
int _quantum = 10000;

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

string GetCommonLogString(int current_time, int pid, int time_in_prev_state,
                          int old_process_state = -1,
                          int new_process_state = -1) {
  stringstream s;
  s << current_time << " " << pid << " " << time_in_prev_state << ": ";

  if (old_process_state != -1) {
    s << ProcessStateToString(old_process_state) << " -> "
      << ProcessStateToString(new_process_state);
  }
  
  return s.str();
}

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
        if (event->old_process_state == ProcessState::BLOCKED) {
          process->time_in_blocked += time_in_prev_state;
          --_total_blocked_count;
          if (_total_blocked_count == 0) {
            _total_block_time += current_time - _block_start_time;
          }
        }
        process->current_state = ProcessState::READY;
        process->state_transition_ts = current_time;
        process->ready_state_transition_event_id = event->event_id;
        _scheduler->AddProcess(process);
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
               << " prio=" << process->static_priority - 1
               << endl;
        }

        process->current_state = ProcessState::READY;
        process->state_transition_ts = current_time;
        _current_running_process = nullptr;
        _scheduler->AddProcess(process);
        call_scheduler = true;
        break;
      }
      case Event::TransitionState::TRANS_TO_RUN: {
        // Compute its burst and schedule its next steps (block or preempt).
        process->cpu_wait_time += time_in_prev_state;
        process->current_state = ProcessState::RUNNING;
        process->state_transition_ts = current_time;
        if (process->current_cpu_burst == 0) {
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
               << " prio=" << process->static_priority - 1
               << endl;
        }

        shared_ptr<Event> new_event;
        if (process->current_cpu_burst > _quantum) {
          // The process needs to preempt after the quantum expires.
          new_event = make_shared<Event>(
            current_time + _quantum, process, process->current_state,
            ProcessState::READY);
        } else {
          new_event = make_shared<Event>(
            current_time + process->current_cpu_burst, process,
            process->current_state, ProcessState::BLOCKED);
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
          process->current_state = ProcessState::BLOCKED;
          process->state_transition_ts = current_time;
          if (_total_blocked_count == 0) {
            _block_start_time = current_time;
          }
          ++_total_blocked_count;
          shared_ptr<Event> new_event = make_shared<Event>(
            current_time + io_burst, process, process->current_state,
            ProcessState::READY);
          _des.AddEvent(new_event);
        } else {
          if (_verbose) {
            cout << GetCommonLogString(current_time, process->pid,
                                       time_in_prev_state)
                 << "Done"
                 << endl;
          }
          process->finish_time = current_time;
          process->turnaround_time = current_time - process->arrival_time;
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
          _current_running_process->current_state, ProcessState::RUNNING);
        _des.AddEvent(new_event);
      }
    }
  }
}

void PrintOutput(string scheduler_type) {

  if (scheduler_type == "F") {
    cout << "FCFS" << endl;
  } else if (scheduler_type == "L") {
    cout << "LCFS" << endl;
  } else if (scheduler_type == "S") {
    cout << "SRTF" << endl;
  } else if (scheduler_type[0] == 'R') {
    cout << "RR " << _quantum << endl;
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

int main(int argc, char *argv[]) {

  string scheduler_type;
  int c;
  int maxprio = 4;

  while((c = getopt(argc, argv, "vs:")) != -1)
    switch (c) {
      case 'v':
        _verbose = true;
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

  ReadInput(input_file, maxprio);

  for (const auto& process : _process_queue) {
    shared_ptr<Event> event = make_shared<Event>(
      process->arrival_time, process, process->current_state,
      ProcessState::READY);
    _des.AddEvent(event);
  }

  Simulate();

  PrintOutput(scheduler_type);
}