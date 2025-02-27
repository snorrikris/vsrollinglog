#include <Windows.h>
#include "VSRL.h"

namespace VSRL
{
	VSRL_IF* s_pVSRL = nullptr;

	/*static*/ bool VSRL::CreateTheOneAndOnlyInstance(const std::wstring& folder_path,
			const std::wstring& log_name, const std::wstring& log_ext, LogLevel logLevel)
	{
		if (s_pVSRL)
		{
#ifdef _DEBUG
			throw std::runtime_error("VSRL instance already created.");
#endif
			return false;
		}
		VSRL* pVSRL = new VSRL(folder_path, log_name, log_ext, logLevel);
		s_pVSRL = pVSRL;
		return pVSRL->Initialize();
	}

	/*static*/ void VSRL::DeleteTheOneAndOnlyInstance()
	{
		if (!s_pVSRL)
		{
#ifdef _DEBUG
			throw std::runtime_error("VSRL instance has not been created.");
#endif
			return;
		}
		VSRL* pVSRL = (VSRL*)s_pVSRL;
		pVSRL->Uninitialize();
		delete pVSRL;
		s_pVSRL = nullptr;
	}

	VSRL::VSRL(const std::wstring& folder_path, const std::wstring& log_name, const std::wstring& log_ext,
			LogLevel logLevel) : m_folder_path(folder_path), m_log_name(log_name), m_log_ext(log_ext),
			m_logLevel(logLevel)
	{
		_AllocBuffer(m_buffer_size);
	}

	Logger& VSRL::GetInstance(const std::string& logger_name)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_loggers.find(logger_name);
		if (it == m_loggers.end())
		{
			m_loggers[logger_name] = std::make_unique<Logger>(this, logger_name);
			it = m_loggers.find(logger_name);
		}
		if (it != m_loggers.end())
		{
			return *((*it).second.get());
		}

		throw std::runtime_error("VSRL logger not found - std::map failure.");
	}

	void VSRL::SetLogLevel(LogLevel level)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_logLevel = level;
	}

	void VSRL::SetFileSizeLimit(uint64_t size_limit)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_file_size_limit = size_limit;
	}

	uint64_t VSRL::GetFileSizeLimit()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_file_size_limit;
	}

	void VSRL::SetRollingLogsCount(uint16_t rolling_count)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (rolling_count >= 1)
		{
			m_rolling_logs_limit = rolling_count;
		}
	}

	uint16_t VSRL::GetRollingLogsCount()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_rolling_logs_limit;
	}

	std::wstring VSRL::GetCurrentLogFilePathname()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_current_log_file_full_pathname;
	}

	void VSRL::Log(LogLevel level, const std::string& logger_name, const std::string& msg)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		while (level >= m_logLevel && m_hFile != 0)
		{
			char* pEnd = nullptr;
			size_t rem_size = 0;
			HRESULT hr = _PrepareLogLine(level, logger_name, &pEnd, rem_size);
			if (SUCCEEDED(hr))
			{
				// write to file - first the metadata string and then the message and then LF.
				if (_WriteToFile(m_buffer.get(), (pEnd - m_buffer.get()), msg.c_str(), msg.size(), Append::LF))
				{
					if (m_file_size > m_file_size_limit)	// file size over limits? -> rollover log files.
					{
						_RolloverLogFile();
					}
				}
				break;
			}

			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				// Need bigger buffer - before retry.
				_AllocBuffer(m_buffer_size * 2);
			}
			else
			{
				break;
			}
		}
	}

	void VSRL::LogFmt(LogLevel level, const std::string& logger_name, const char* fmt, va_list args)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		while (level >= m_logLevel && m_hFile != 0)
		{
			char* pPrepEnd = nullptr, * pEnd = nullptr;
			size_t rem_size_prep = 0, rem_size = 0;
			HRESULT hr = _PrepareLogLine(level, logger_name, &pPrepEnd, rem_size_prep);
			if (SUCCEEDED(hr))
			{
				va_list args_copy;
				va_copy(args_copy, args);
				HRESULT hr = ::StringCbVPrintfExA(pPrepEnd, rem_size_prep, &pEnd, &rem_size,
						STRSAFE_IGNORE_NULLS, fmt, args_copy);
				va_end(args_copy);
				if (SUCCEEDED(hr))
				{
					// write to file - the entire line, append LF.
					if (_WriteToFile(m_buffer.get(), (pEnd - m_buffer.get()), nullptr, 0, Append::LF))
					{
						if (m_file_size > m_file_size_limit)	// file size over limits? -> rollover log files.
						{
							_RolloverLogFile();
						}
					}
					break;
				}
			}

			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				_AllocBuffer(m_buffer_size * 2);	// Need bigger buffer - before retry.
			}
			else
			{
				break;
			}
		}
	}

	void VSRL::LogW(LogLevel level, const std::string& logger_name, const std::wstring& msgW)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		while (level >= m_logLevel && m_hFile != 0)
		{
			char* pEnd = nullptr;
			size_t rem_size = 0, msgW_size = msgW.size();
			HRESULT hr = _PrepareLogLine(level, logger_name, &pEnd, rem_size);
			if (SUCCEEDED(hr))
			{
				if (msgW_size)
				{
					int utf8_size = ::WideCharToMultiByte(CP_UTF8, 0, msgW.c_str(), (int)msgW_size,
						pEnd, (int)rem_size, nullptr, nullptr);
					if (utf8_size > 0)
					{
						pEnd += utf8_size;
					}
					else
					{
						hr = ::GetLastError() == ERROR_INSUFFICIENT_BUFFER
								? STRSAFE_E_INSUFFICIENT_BUFFER : E_FAIL;
					}
				}
				if (SUCCEEDED(hr)	// write to file - the entire line, append LF.
						&& _WriteToFile(m_buffer.get(), (pEnd - m_buffer.get()), nullptr, 0, Append::LF))
				{
					if (m_file_size > m_file_size_limit)	// file size over limits? -> rollover log files.
					{
						_RolloverLogFile();
					}
					break;
				}
			}

			if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
			{
				_AllocBuffer(m_buffer_size * 2);	// Need bigger buffer - before retry.
			}
			else
			{
				break;
			}
		}
	}

	bool _FileExists(const std::wstring& pathName)
	{
		DWORD dwAttrib = ::GetFileAttributesW(pathName.c_str());
		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	// Open and close and existing file for read/write with share=none.
	// Return true if open succeeded. Return false if file not found or write permissions not available.
	bool _HasExclusivePermissionsToFile(const std::wstring& pathName)
	{
		HANDLE hFile = ::CreateFileW(pathName.c_str(), GENERIC_READ | GENERIC_WRITE,
			0 /* FILE_SHARE_none */, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hFile);
			return true;	// Open existing file for read/write succeeded.
		}
		return false;
	}

	bool VSRL::Initialize()
	{
		::CreateDirectory(m_folder_path.c_str(), NULL);
		std::wstring logfile = m_folder_path + m_log_name + m_log_ext;
		wchar_t suffix_to_try = 'A';
		bool doesFileExist = false, hasExclusivePermissions = false;;
		while (true)
		{
			doesFileExist = _FileExists(logfile);
			hasExclusivePermissions = _HasExclusivePermissionsToFile(logfile);
			if (!doesFileExist || hasExclusivePermissions)
			{
				m_log_name += L"_" + suffix_to_try;
				break;
			}
			logfile = m_folder_path + m_log_name + L"_" + suffix_to_try + m_log_ext;
			++suffix_to_try;
		}
		m_current_log_file_full_pathname = logfile;

		DWORD dwCreationDisposition = doesFileExist ? OPEN_EXISTING : CREATE_NEW;
		HANDLE hFile = ::CreateFileW(m_current_log_file_full_pathname.c_str(), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		if (doesFileExist)
		{
			LARGE_INTEGER li;
			if (!::GetFileSizeEx(hFile, &li) || !::SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
			{
				::CloseHandle(hFile);
				return false;
			}
			m_file_size = li.QuadPart;
		}
		m_hFile = hFile;
		return true;
	}

	void VSRL::Uninitialize()
	{
		if (m_hFile)
		{
			::CloseHandle(m_hFile);
		}
	}

	void VSRL::_RolloverLogFile()
	{
		::CloseHandle(m_hFile);
		m_hFile = 0;
		std::wstring dot = L".";
		std::wstring last_file = m_folder_path + m_log_name + m_log_ext
				+ dot + std::to_wstring(m_rolling_logs_limit);
		if (_FileExists(last_file))
		{
			::DeleteFile(last_file.c_str());
		}
		for (uint16_t idx = m_rolling_logs_limit - 1; idx > 0; --idx)
		{
			std::wstring e_fn = m_folder_path + m_log_name + m_log_ext + dot + std::to_wstring(idx);
			if (_FileExists(e_fn))
			{
				std::wstring n_fn = m_folder_path + m_log_name + m_log_ext + dot + std::to_wstring(idx + 1);
				::MoveFileW(e_fn.c_str(), n_fn.c_str());
			}
		}
		HANDLE hFile = ::CreateFileW(m_current_log_file_full_pathname.c_str(), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return;
		}

		m_file_size = 0;
		m_hFile = hFile;
	}
}	// namespace VSRL
