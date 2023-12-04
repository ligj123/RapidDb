#include "../cache/Mallocator.h"
#include "Session.h"
#include <thread>

namespace storage {
using namespace std;
class SessionPool {
public:
  SessionPool(uint16_t threadNum) : _threadNum(threadNum) {}

protected:
  // Create how much threads to run session.
  uint16_t _threadNum;
  // The pointer point to an address of array. Every thread will response to a
  // map. All sessions will be allocated to the threads according their hashs.
  MHashMap<uint32_t, Session> *_arMapSession;
  // The pointer to an array to save the threads.
  thread *_arThread;
};
} // namespace storage