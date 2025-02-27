#ifndef _VSRL_IF_H
#define _VSRL_IF_H

#include <string>
#include <cstdarg>

namespace VSRL
{
	enum class LogLevel { None = 0, Debug, Info, Notice, Warn, Error, Crit, Alert, Fatal, Emerg };

	class Logger;

	class VSRL_IF
	{
	public:
		virtual Logger& GetInstance(const std::string& logger_name) = 0;

		virtual void SetLogLevel(LogLevel level) = 0;
		virtual LogLevel GetLogLevel() const = 0;

		virtual void SetFileSizeLimit(uint64_t size_limit) = 0;
		virtual uint64_t GetFileSizeLimit() = 0;

		virtual void SetRollingLogsCount(uint16_t rolling_count) = 0;
		virtual uint16_t GetRollingLogsCount() = 0;

		virtual std::wstring GetCurrentLogFilePathname() = 0;

		virtual void Log(LogLevel level, const std::string& logger_name, const std::string& msg) = 0;
		virtual void LogFmt(LogLevel level, const std::string& logger_name, const char* fmt, va_list args) = 0;
		virtual void LogW(LogLevel level, const std::string& logger_name, const std::wstring& msgW) = 0;
	};

	class Logger
	{
	protected:
		VSRL_IF* m_pVSRL = nullptr;

		std::string m_logger_name;

	public:
		Logger(VSRL_IF* pVSRL, const std::string& logger_name)
			: m_pVSRL(pVSRL), m_logger_name(logger_name) {}

		void debug(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Debug, m_logger_name, fmt, va);
			va_end(va);
		}
		void debug(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Debug, m_logger_name, msg);
		}
		void debug(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Debug, m_logger_name, msgW);
		}
		void info(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Info, m_logger_name, fmt, va);
			va_end(va);
		}
		void info(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Info, m_logger_name, msg);
		}
		void info(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Info, m_logger_name, msgW);
		}
		void notice(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Notice, m_logger_name, fmt, va);
			va_end(va);
		}
		void notice(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Notice, m_logger_name, msg);
		}
		void notice(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Notice, m_logger_name, msgW);
		}
		void warn(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Warn, m_logger_name, fmt, va);
			va_end(va);
		}
		void warn(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Warn, m_logger_name, msg);
		}
		void warn(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Warn, m_logger_name, msgW);
		}
		void error(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Error, m_logger_name, fmt, va);
			va_end(va);
		}
		void error(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Error, m_logger_name, msg);
		}
		void error(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Error, m_logger_name, msgW);
		}
		void crit(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Crit, m_logger_name, fmt, va);
			va_end(va);
		}
		void crit(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Crit, m_logger_name, msg);
		}
		void crit(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Crit, m_logger_name, msgW);
		}
		void alert(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Alert, m_logger_name, fmt, va);
			va_end(va);
		}
		void alert(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Alert, m_logger_name, msg);
		}
		void alert(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Alert, m_logger_name, msgW);
		}
		void emerg(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Emerg, m_logger_name, fmt, va);
			va_end(va);
		}
		void emerg(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Emerg, m_logger_name, msg);
		}
		void emerg(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Emerg, m_logger_name, msgW);
		}
		void fatal(const char* fmt, ...)
		{
			va_list va; va_start(va, fmt);
			m_pVSRL->LogFmt(LogLevel::Fatal, m_logger_name, fmt, va);
			va_end(va);
		}
		void fatal(const std::string& msg)
		{
			m_pVSRL->Log(LogLevel::Fatal, m_logger_name, msg);
		}
		void fatal(const std::wstring& msgW)	// msgW is converted to UTF8
		{
			m_pVSRL->LogW(LogLevel::Fatal, m_logger_name, msgW);
		}
	};

	extern VSRL_IF* s_pVSRL;
}	// namespace VSRL
#endif // _VSRL_IF_H
