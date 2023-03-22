#ifndef EVENT_H
#define EVENT_H

#include <memory>

class Process;

using namespace std;

class Event {
  public:
    Event(int timestamp, shared_ptr<Process> process, int old_process_state,
          int new_process_state);
  
    enum TransitionState {
      TRANS_TO_READY = 0,
      TRANS_TO_PREEMPT,
      TRANS_TO_RUN,
      TRANS_TO_BLOCK
    };

    int GetTransition();

  public:
    // To keep count of number of events.
    static int count;

    int timestamp;
    shared_ptr<Process> process;
    int old_process_state;
    int new_process_state;
    int event_id;
};

#endif