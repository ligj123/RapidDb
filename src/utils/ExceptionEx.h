#ifndef EXCEPTION_EX_H
#define EXCEPTION_EX_H
#include <exception>
#include <string>
#include <sstream>
#include <boost/stacktrace.hpp>

using namespace std;
namespace utils {
	class BacktraceException : public exception
	{
	public:
		BacktraceException(string what)
		{
			std::stringstream ss;
			ss << boost::stacktrace::stacktrace() << "\n";
			m_strTrace = ss.str();
			m_strWhat = what;
		}

		virtual ~BacktraceException(void)
		{}

		virtual const char* what()
		{
			std::stringstream ss;
			ss << m_strWhat << "\n" << m_strTrace;
			return ss.str().c_str();
		}

		const char* GetBacktrace() const
		{
			return m_strTrace.c_str();
		}

		const string GetMessage()
		{
			return m_strWhat;
		}

	private:
		string m_strWhat;
		string m_strTrace;
	};
}

#endif
