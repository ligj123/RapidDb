#ifndef LOG_H
#define LOG_H

//#include <string>
//#include <fstream>
//#include <boost/thread/mutex.hpp>
//
//#define CRLF "\r\n"
//
//using namespace std;
//
//namespace rapid {
//	namespace utils {
//		enum emLogType : short
//		{
//			Fatal = 0,	//
//			Exception = 1,	//
//			Error = 2,    //
//			Warning = 3,    //
//			Infor = 4,	//
//			Debug = 5,	//
//			Trace = 6
//		};
//
//		class Log
//		{
//		protected:
//			Log();
//			~Log();
//
//		protected:
//			void WriteLog(string msg, emLogType logType, bool bTime);
//			string FileName();
//			string LogTypeToStr(emLogType logType);
//			bool IsOpen() { return m_pLogStream != nullptr; }
//
//		protected:
//			size_t m_byteCount;
//			ofstream* m_pLogStream;
//			boost::mutex m_mutex;
//
//			string m_strRootPath;
//			emLogType m_logType;	//写入日志的级别，即只有优先级高于或等于此的才会写入日志
//
//			static Log* m_log;
//
//		public:
//			static bool InitLog(string strRootPath, emLogType logTypeFrom);
//			static void Write(wstring location, wstring msg, emLogType logType = LogTypeInforLow, bool bTime = true);
//			static void Write(string location, string msg, emLogType logType = LogTypeInforLow, bool bTime = true);
//		};
//	}
//}
#endif
