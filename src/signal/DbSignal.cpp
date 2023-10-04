#include "DbSignal.h"
#include "../cache/StackTrace.h"
#include "../utils/Log.h"

namespace storage {
void SignalHandle::SetSignal() {
  signal(SIGINT, HandleQuit);
  signal(SIGQUIT, HandleQuit);
  signal(SIGKILL, HandleAbort);
  signal(SIGKILL, HandleKill);
  signal(SIGSEGV, HandleInvalidMemory);
}

void SignalHandle::HandleQuit(int sig) {
  LOG_INFO << "Signal " << sig;
  exit(sig);
}

void SignalHandle::HandleAbort(int sig) {
  LOG_ERROR << "Abort: " << PrintStack();
}

void SignalHandle::HandleKill(int sig) {
  LOG_WARN << "Signal kill";
  exit(sig);
}

void SignalHandle::HandleInvalidMemory(int sig) {
  LOG_ERROR << "Invalid access to memory: \n" << PrintStack();
}
} // namespace storage