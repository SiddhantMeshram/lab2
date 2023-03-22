#include "round_robin.h"

void RoundRobin::AddProcess(shared_ptr<Process> process) {
  _run_queue.push(process);
}

shared_ptr<Process> RoundRobin::GetNextProcess() {
  if (_run_queue.size() == 0) {
    return nullptr;
  }
  auto process = _run_queue.front();
  _run_queue.pop();
  return process;
}