#include "../cache/StackTrace.h"

namespace storage {
class SignalHandle {
public:
  SignalHandle() = delete;

public:
  static void SetSignal();

protected:
  static void HandleQuit(int sig);          // Signal 3
  static void HandleAbort(int sig);         // Signal 6
  static void HandleKill(int sig);          // Signal 9
  static void HandleInvalidMemory(int sig); // Signal 11
};
} // namespace storage