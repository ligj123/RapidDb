#ifndef EXCEPTION_EX_H
#define EXCEPTION_EX_H
#include "../cache/Mallocator.h"
#include <boost/stacktrace.hpp>
#include <exception>
#include <sstream>

using namespace std;
namespace storage {
class BacktraceException : public exception {
public:
  BacktraceException(MString what) {
    std::stringstream ss;
    ss << boost::stacktrace::stacktrace() << "\n";
    m_strTrace = ss.str();
    m_strWhat = what;
  }

  virtual ~BacktraceException(void) {}

  virtual const char *what() {
    std::stringstream ss;
    ss << m_strWhat << "\n" << m_strTrace;
    return ss.str().c_str();
  }

  const char *GetBacktrace() const { return m_strTrace.c_str(); }

  const MString GetMessage() { return m_strWhat; }

private:
  MString m_strWhat;
  MString m_strTrace;
};
} // namespace storage

#endif
