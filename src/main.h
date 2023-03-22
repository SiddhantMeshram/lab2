#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// Forward Declaration.
class Scheduler;
class Des;
class Process;

class Main {
  public:
    Main(const string& scheduler_type,
         const string& input_file, const string& rand_file,
         const bool verbose, const bool preemption_logs);
  
  private:
    void ProcessInput(const string& input_file, const string& rand_file);
    
    void PopulateRandArray(const string& rand_file);

    int MyRandom(int burst);

    void ReadInput(const string& input_file, const int maxprio);

    void Simulate();

    void PrintOutput();

    string GetCommonLogString(
      int current_time, int pid, int time_in_prev_state,
      int old_process_state, int new_process_state);
  
  private:
    string _scheduler_type;
    bool _verbose;
    bool _preemption_logs;
    int _quantum;
    int _maxprio;
    bool _is_prio_scheduler;
    bool _is_pre_prio_scheduler;
    int _total_blocked_count;
    int _block_start_time;
    int _total_block_time;
    int _ofs;

    vector<int> _randvals;
    shared_ptr<Scheduler> _scheduler;
    shared_ptr<Des> _des;
    shared_ptr<Process> _current_running_process;
    vector<shared_ptr<Process>> _process_queue;
};

#endif