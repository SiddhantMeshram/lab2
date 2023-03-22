#include "des.h"
#include "process.h"

using namespace std;

shared_ptr<Event> Des::GetEvent() {
  if (_event_q.size() != 0) {
    shared_ptr<Event> e = *(_event_q.begin());
    _event_q.erase(_event_q.begin());
    return e;
  }

  return nullptr;
}

void Des::AddEvent(shared_ptr<Event> event) {
  _event_q.insert(event);
}

int Des::GetNextEventTime() {
  if (_event_q.size() == 0) {
    return -1;
  }
  return (*(_event_q.begin()))->timestamp;
}

int Des::GetNextEventTime(int pid) {
  for (auto itr = _event_q.begin(); itr != _event_q.end(); ++itr) {
    if ((*itr)->process->pid == pid) {
      return (*itr)->timestamp;
      break;
    }
  }
  
  return -1;
}

void Des::RmEvent(int pid) {
  for (auto itr = _event_q.begin(); itr != _event_q.end(); ++itr) {
    if ((*itr)->process->pid == pid) {
      _event_q.erase(itr);
      break;
    }
  }
}