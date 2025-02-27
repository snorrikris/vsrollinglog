#ifndef _VSRL_H
#define _VSRL_H

#include "VSRL_IF.h"
#include <map>
#include <mutex>
#include <strsafe.h>

namespace VSRL
{
	class VSRL : public VSRL_IF
	{
	protected:
		std::mutex m_mutex;

		std::wstring m_folder_path;
		std::wstring m_log_name;
		std::wstring m_log_ext;
		std::wstring m_current_log_file_full_pathname;

		HANDLE m_hFile = 0;
		uint64_t m_file_size = 0;

		uint64_t m_file_size_limit = 20 * 1024 * 1024;

		uint16_t m_rolling_logs_limit = 10;

		LogLevel m_logLevel = LogLevel::Debug;

		std::map<std::string, std::unique_ptr<Logger>> m_loggers;

		std::unique_ptr<char[]> m_buffer;
		size_t m_buffer_size = 1024;

		char m_levelStrings[10][7] = { "NONE  ", "DEBUG ", "INFO  ", "NOTICE", "WARN  ", "ERROR ",
				"CRIT  ", "ALERT ", "FATAL ", "EMERG " };

		enum class Append { Nothing, LF };

		VSRL(const std::wstring& folder_path, const std::wstring& log_name, const std::wstring& log_ext,
				LogLevel logLevel);

	public:
		static bool CreateTheOneAndOnlyInstance(const std::wstring& folder_path,
				const std::wstring& log_name, const std::wstring& log_ext, LogLevel logLevel);
		static void DeleteTheOneAndOnlyInstance();

		bool Initialize();
		void Uninitialize();

		virtual Logger& GetInstance(const std::string& logger_name) override;

		virtual void SetLogLevel(LogLevel level) override;
		virtual LogLevel GetLogLevel() const override { return m_logLevel; }

		virtual void SetFileSizeLimit(uint64_t size_limit) override;
		virtual uint64_t GetFileSizeLimit() override;

		virtual void SetRollingLogsCount(uint16_t rolling_count) override;
		virtual uint16_t GetRollingLogsCount() override;

		virtual std::wstring GetCurrentLogFilePathname() override;

		virtual void Log(LogLevel level, const std::string& logger_name, const std::string& msg) override;
		virtual void LogFmt(LogLevel level, const std::string& logger_name, const char* fmt, va_list args) override;
		virtual void LogW(LogLevel level, const std::string& logger_name, const std::wstring& msgW) override;

	protected:
		// Note - format is fixed as:
		// 2018-09-06 08:52:02,693 [WARN  ] [12345] VerifySign : Invalid hardware key
		// date/time               level    thread  logger       message
		// Note - this is log4cpp format: "%d [%-6p] [%t] %c : %m%n"
		inline HRESULT _PrepareLogLine(LogLevel level, const std::string& logger_name, char** ppEnd, size_t& rem_size)
		{
			int ll_idx = (int)level;
			if (ll_idx < 1 || ll_idx > 9)
			{
				return STRSAFE_E_INVALID_PARAMETER;
			}
			SYSTEMTIME st;
			::GetSystemTime(&st);
			return ::StringCbPrintfExA(m_buffer.get(), m_buffer_size, ppEnd, &rem_size,
				STRSAFE_IGNORE_NULLS, "%04u-%02u-%02u %02u:%02u:%02u,%03u [%s] [%lu] %s : ",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
				m_levelStrings[ll_idx], ::GetCurrentThreadId(), logger_name.c_str());
		}

		void _AllocBuffer(size_t buf_size)
		{
			if (m_buffer.get())
			{
				m_buffer.reset();
			}
			m_buffer_size = buf_size;
			m_buffer = std::make_unique<char[]>(m_buffer_size);
		}

		bool _WriteToFile(const char* pBuf1, size_t len1, const char* pBuf2, size_t len2, Append append)
		{
			// write to file - first the pBuf1 and then pBuf2 and then (optionally) LF.
			DWORD dwBytesWritten = 0;
			if (::WriteFile(m_hFile, pBuf1, (DWORD)len1, &dwBytesWritten, 0))
			{
				m_file_size += dwBytesWritten;
				if (pBuf2 && ::WriteFile(m_hFile, pBuf2, (DWORD)len2, &dwBytesWritten, 0))
				{
					m_file_size += dwBytesWritten;
				}
				if (append == Append::LF && ::WriteFile(m_hFile, "\n", 1, &dwBytesWritten, 0))
				{
					m_file_size += dwBytesWritten;
				}
				return true;
			}
			return false;
		}

		void _RolloverLogFile();
	};
}	// namespace VSRL
#endif // _VSRL_H
