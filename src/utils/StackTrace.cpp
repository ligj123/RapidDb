#include "StackTrace.h"

namespace storage {
#ifdef _MSVC_LANG
std::string StackTrace(int depth) { return ""; }
#else
#include <boost/stacktrace.hpp>
#include <iostream>
#include <sstream>
#include <string>

std::string StackTrace(int depth) {
  boost::stacktrace::stacktrace st;
  auto iter = st.begin() + 1;
  std::stringstream ss;

  for (int i = 0; iter != st.end() && i < depth; iter++, i++) {
    if (iter->name().size() == 0)
      break;
    ss << i << "# " << iter->name() << "\n";
  }

  return ss.str();
}

#endif // _MSVC_LANG

} // namespace storage