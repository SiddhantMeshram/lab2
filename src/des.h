#ifndef DES_H
#define DES_H

#include <memory>
#include <set>

#include "event.h"

using namespace std;

class Des {
  public:
    shared_ptr<Event> GetEvent();

    void AddEvent(shared_ptr<Event> event);

    int GetNextEventTime();

    int GetNextEventTime(int pid);

    // Used to delete any existing event for a process with the given pid.
    void RmEvent(int pid);

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
    set<shared_ptr<Event>, Comparator> _event_q;
};

#endif