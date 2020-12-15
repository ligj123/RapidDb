#pragma once
#include <iostream>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/named_scope.hpp>

#define LOG_TRACE(msg) utils::Logger::Write(utils::TRACE, msg)
#define LOG_DEBUG(msg) utils::Logger::Write(utils::DEBUG, msg)
#define LOG_INFO(msg) utils::Logger::Write(utils::INFO, msg)
#define LOG_WARN(msg) utils::Logger::Write(utils::WARN, msg)
#define LOG_ERROR(msg) utils::Logger::Write(utils::ERROR, msg)
#define LOG_FATAL(msg) utils::Logger::Write(utils::FATAL, msg)

namespace utils {
  namespace logging = boost::log;
  namespace src = boost::log::sources;
  namespace expr = boost::log::expressions;
  namespace sinks = boost::log::sinks;
  namespace attrs = boost::log::attributes;
  namespace keywords = boost::log::keywords;


  enum severity_level : uint8_t
  {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };

  class Logger {
  public:
    static void Write(severity_level level, std::string msg);
    static void init();
  protected:
    static src::severity_logger< severity_level > slg_;
  };

}
