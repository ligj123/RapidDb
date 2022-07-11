#include "StackTrace.h"

#ifdef _MSVC_LANG
#define BOOST_STACKTRACE_INTERNAL_BUILD_LIBS
#define BOOST_STACKTRACE_LINK
#define BOOST_STACKTRACE_USE_WINDBG_CACHED
#include <boost/stacktrace/detail/frame_msvc.ipp>
#include <boost/stacktrace/safe_dump_to.hpp>
#else
#define BOOST_STACKTRACE_INTERNAL_BUILD_LIBS
#define BOOST_STACKTRACE_USE_ADDR2LINE
#define BOOST_STACKTRACE_LINK

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <boost/stacktrace/detail/frame_unwind.ipp>
#include <boost/stacktrace/safe_dump_to.hpp>
#endif
#include <boost/stacktrace.hpp>
#include <iostream>
#include <sstream>
#include <string>

namespace storage {
std::string StackTrace() {
#ifdef _STACK_DEPTH
  static int depth = _STACK_DEPTH;
#else
  static int depth = 5;
#endif // STACK_DEPTH

  static int count = 0;
  count++;
  boost::stacktrace::stacktrace st;
  auto iter = st.begin() + 1;
  std::stringstream ss;

  for (int i = 0; iter != st.end() && i < depth; iter++, i++) {
    // if (iter->name().size() == 0)
    //   break;
    std::string ff = iter->source_file();
    ff = ff.substr(ff.rfind('/', ff.rfind('/') - 1) + 1);
    ss << i << "# " << iter->name() << " at " << ff << " : "
       << iter->source_line() << "\n";
  }

  return "NO: " + std::to_string(count) + "\n" + ss.str();
}
} // namespace storage