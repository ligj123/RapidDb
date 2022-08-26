#include "LogServer.h"
#include <fstream>

namespace storage {
void LogServer::LogWriteThread() {
  fstream _file(Configure::GetLogPath() + StrSecTime() + ".bin",
                ios::in | ios::out | ios::binary);
  while (true) {
  }
}
} // namespace storage