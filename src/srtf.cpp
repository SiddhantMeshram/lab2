#include "srtf.h"

void Srtf::AddProcess(shared_ptr<Process> process) {
  _run_queue.push(process);
}

shared_ptr<Process> Srtf::GetNextProcess() {
  if (_run_queue.size() != 0) {
    shared_ptr<Process> p = _run_queue.top();
    _run_queue.pop();
    return p;
  }

  return nullptr;
}