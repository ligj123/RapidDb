#include "Log.h"
#include <filesystem>

namespace utils {
BOOST_LOG_ATTRIBUTE_KEYWORD(log_severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(log_timestamp, "TimeStamp", boost::posix_time::ptime)

std::ostream& operator<< (std::ostream& strm, severity_level level)
{
  static const char* strings[] = { "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };

  if (static_cast<std::size_t>(level) < sizeof(strings) / sizeof(*strings))
    strm << strings[level];
  else
    strm << static_cast<int>(level);

  return strm;
}

std::string FileName(std::string pathName)
{
  return pathName.substr(pathName.find_last_of("/\\") + 1);
}

src::severity_logger< severity_level > Logger::slg_;

void Logger::init(severity_level filterFile, severity_level filterConsole)
{
  logging::formatter formatter = expr::stream << expr::format_date_time(log_timestamp, "%Y-%m-%d %H:%M:%S")
    << " [" << log_severity << "] " << expr::message;

  auto console_sink = logging::add_console_log(std::clog, keywords::format = formatter);
  console_sink->set_filter(log_severity >= filterConsole);
  std::filesystem::path logPath = std::filesystem::current_path();
  logPath += "/log";
  if (!std::filesystem::exists(logPath)) std::filesystem::create_directories(logPath);

  auto file_sink = logging::add_file_log
  (
    keywords::file_name = logPath.string() + "/Normal_%Y-%m-%d_%N.log", //文件名
    keywords::rotation_size = 10 * 1024 * 1024, //单个文件限制大小
    keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
    keywords::format = formatter
  );

  file_sink->locked_backend()->auto_flush(true);
  file_sink->set_filter(log_severity >= filterFile);

  logging::add_common_attributes();
  logging::core::get()->add_sink(console_sink);
  logging::core::get()->add_sink(file_sink);
}
}
