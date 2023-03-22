#include "lcfs.h"

using namespace std;

void Lcfs::AddProcess(shared_ptr<Process> process) {
  _run_queue.push_back(process);
}

shared_ptr<Process> Lcfs::GetNextProcess() {
  if (_run_queue.size() == 0) {
    return nullptr;
  }
  auto process = _run_queue.back();
  _run_queue.pop_back();
  return process;
}