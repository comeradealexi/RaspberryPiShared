#pragma once

#include <iostream>
#include <ostream>
#include <istream>
#include <mutex>
#include <stdarg.h>
#include <fstream>
#include <iomanip> // put_time
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <ctime>
#include <string>  // string
#include <thread>

class Log
{
private:
	std::ofstream m_LogFile;
	std::mutex m_LogMutex;

public:
	//The LogReturn is used to ensure when the LogReturn destructor is called, it writes the newline to the end of the current line.
	//Since there is never an actual LogReturn
	struct LogReturn
	{
		LogReturn(Log& log) : m_Log(log) { }

		~LogReturn()
		{
			if (m_bWrittenTo == false)
			{
				m_Log.m_LogFile << "\n";
				m_Log.m_LogFile.flush();
				std::cout << "\n";
				m_Log.m_LogMutex.unlock();
			}
		}

		template <typename T>
		LogReturn operator<<(const T& TData)
		{
			m_bWrittenTo = true;
			m_Log.m_LogFile << TData;
			std::cout << TData;
			return LogReturn(m_Log);
		}

		Log& m_Log;
		bool m_bWrittenTo = false;
	};

	Log()
	{
		OpenLog();
	}
	~Log()
	{
		CloseLog();
	}
	static inline Log& Get()
	{
		static Log log;
		return log;
	}

	static std::string Return_current_time_and_date()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::stringstream ss;
		ss << "(" << std::this_thread::get_id() << ") ";
		ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X: "); 
		return ss.str();
	}

	static std::string ToString(const std::chrono::system_clock::time_point& timePoint)
	{
		auto in_time_t = std::chrono::system_clock::to_time_t(timePoint);

		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
		return ss.str();
	}

	//Allows writing to ofstream as well as cout!
	template <typename T>
	LogReturn operator<<(const T& TData)
	{
		m_LogMutex.lock();
		m_LogFile << Return_current_time_and_date();
		m_LogFile << TData;
		std::cout << TData;
		return LogReturn(*this);
	}

	void Printf(const char* __fmt, ...)
	{
		std::unique_lock<std::mutex> lk(m_LogMutex);

		const int k_bufferSize = 1024 * 1024;
		static char cLogBuffer[k_bufferSize];
		if (m_LogFile.good())
		{
			va_list argptr;
			va_start(argptr, __fmt);
			vsprintf(cLogBuffer, __fmt, argptr);
			va_end(argptr);
			m_LogFile << Return_current_time_and_date();
			m_LogFile << cLogBuffer;
			m_LogFile.flush();
			std::cout << cLogBuffer;
		}
	}

	void OpenLog()
	{
#ifdef DEBUG
#define BUILD "DEBUG"
#else
#define BUILD "RELEASE"
#endif

		//Use epoc time as a file log stamp
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		std::string strName = "Log_" BUILD "_";
		strName += std::to_string(ms.count());
		strName += ".txt";

		std::unique_lock<std::mutex> lk(m_LogMutex);
		m_LogFile.open(strName.c_str(), std::ofstream::out);
	}

	void CloseLog()
	{
		*this << "Exiting Program With Normal Termination";

		std::unique_lock<std::mutex> lk(m_LogMutex);
		if (m_LogFile.good())
		{
			m_LogFile << "Log Closed.";
			m_LogFile.close();
		}
	}
};