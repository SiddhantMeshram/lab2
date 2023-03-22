#include "fcfs.h"

using namespace std;

void Fcfs::AddProcess(shared_ptr<Process> process) {
  _run_queue.push(process);
}

shared_ptr<Process> Fcfs::GetNextProcess() {
  if (_run_queue.size() == 0) {
    return nullptr;
  }
  auto process = _run_queue.front();
  _run_queue.pop();
  return process;
}