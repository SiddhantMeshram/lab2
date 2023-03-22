#include "prio.h"

Prio::Prio (int maxprio) :
  _maxprio(maxprio) {

  _active_queue.resize(_maxprio);
  _expired_queue.resize(_maxprio);
}

//-----------------------------------------------------------------------------

void Prio::AddProcess(shared_ptr<Process> process) {
  if (process->dynamic_priority == -1) {
    process->dynamic_priority = process->static_priority - 1;
    _expired_queue[process->dynamic_priority].push(process);
    return;
  }

  _active_queue[process->dynamic_priority].push(process);
}

//-----------------------------------------------------------------------------

shared_ptr<Process> Prio::GetInActiveQueue() {
  for (int ii = _active_queue.size() - 1; ii >= 0; --ii) {
    if (_active_queue[ii].size() != 0) {
      shared_ptr<Process> p = _active_queue[ii].front();
      _active_queue[ii].pop();
      return p;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------

shared_ptr<Process> Prio::GetNextProcess() {
  shared_ptr<Process> p = GetInActiveQueue();
  if (p) {
    return p;
  }

  // If we reach here, it means active_queue is empty. Replace it with
  // expired queue.
  _active_queue = _expired_queue;
  _expired_queue.clear();
  _expired_queue.resize(_maxprio);
  
  return GetInActiveQueue();
}

//-----------------------------------------------------------------------------