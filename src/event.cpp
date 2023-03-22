#include "event.h"
#include "process.h"

int Event::count = 0;

Event::Event(int timestamp, shared_ptr<Process> process, int old_process_state,
          int new_process_state) :
  timestamp(timestamp),
  process(process),
  old_process_state(old_process_state),
  new_process_state(new_process_state),
  event_id(count++) {}

int Event::GetTransition() {
  if (old_process_state == Process::ProcessState::CREATED &&
      new_process_state == Process::ProcessState::READY) {
    return TRANS_TO_READY;
  } else if (old_process_state == Process::ProcessState::READY &&
              new_process_state == Process::ProcessState::RUNNING) {
    return TRANS_TO_RUN;
  } else if (old_process_state == Process::ProcessState::RUNNING &&
              new_process_state == Process::ProcessState::BLOCKED) {
    return TRANS_TO_BLOCK;
  } else if (old_process_state == Process::ProcessState::BLOCKED &&
              new_process_state == Process::ProcessState::READY) {
    return TRANS_TO_READY;
  } else if (old_process_state == Process::ProcessState::RUNNING &&
              new_process_state == Process::ProcessState::READY) {
    return TRANS_TO_PREEMPT;
  }

  return -1;
}