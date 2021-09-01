#pragma once
#include "ThreadPool.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <iostream>
#include <sstream>
#include <string>

#define LOG_DEBUG                                                              \
  BOOST_LOG_SEV(Logger::slg_, DEBUG)                             \
      << "<" << ThreadPool::GetThreadName() << ">  "
#define LOG_INFO                                                               \
  BOOST_LOG_SEV(Logger::slg_, INFO)                              \
      << "<" << ThreadPool::GetThreadName() << ">  "
#define LOG_WARN                                                               \
  BOOST_LOG_SEV(Logger::slg_, WARN)                              \
      << "<" << ThreadPool::GetThreadName() << ">  "
#define LOG_ERROR                                                              \
  BOOST_LOG_SEV(Logger::slg_, ERROR)                             \
      << "<" << ThreadPool::GetThreadName() << ">  "
#define LOG_FATAL                                                              \
  BOOST_LOG_SEV(Logger::slg_, FATAL)                             \
      << "<" << ThreadPool::GetThreadName() << ">  "

namespace storage {
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

enum severity_level : uint8_t { DEBUG = 0, INFO, WARN, ERROR, FATAL };

class Logger {
public:
  static void init(severity_level filterFile = ERROR,
                   severity_level filterConsole = WARN);

public:
  static src::severity_logger<severity_level> slg_;
};
} // namespace storage
