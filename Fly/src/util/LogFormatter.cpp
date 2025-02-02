#include "pch.h"

#include "LogFormatter.h"

bool LogFormatter::s_checkedColorSupport = false;
bool LogFormatter::s_hasColorSupport = false;
LogFormatter::LogConfig LogFormatter::s_config;
std::vector<LogFormatter::LogCallback> LogFormatter::s_callbacks;

void LogFormatter::Initialize()
{
	HasColorSupport();      // Initialize color support
	s_config = LogConfig{}; // Initialize with defaults
}

void LogFormatter::SetConfig(const LogConfig& config)
{
	s_config = config;
}

LogFormatter::LogConfig& LogFormatter::GetConfig()
{
	return s_config;
}

bool LogFormatter::IsRunningInVisualStudio()
{
	return GetModuleHandleA("devenv.exe") != nullptr;
}

bool LogFormatter::EnableVirtualTerminalProcessing()
{
	// Try to enable ANSI support for both console and Visual Studio
	bool success = false;
	HANDLE handles[] = { GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE) };

	for (HANDLE handle: handles)
	{
		if (handle != INVALID_HANDLE_VALUE)
		{
			DWORD mode = 0;
			if (GetConsoleMode(handle, &mode))
			{
				mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				if (SetConsoleMode(handle, mode))
				{
					success = true;
				}
			}
		}
	}
	return success;
}

bool LogFormatter::HasColorSupport()
{
	if (!s_checkedColorSupport)
	{
		// Try to enable ANSI support early
		s_hasColorSupport = EnableVirtualTerminalProcessing();

		if (IsRunningInVisualStudio())
		{
			// In Visual Studio, only use ANSI if we successfully enabled it
			s_config.useAnsiColors = s_hasColorSupport;
			s_config.useConsoleColors = false;
		}
		else
		{
			// In regular console, prefer console colors
			s_config.useAnsiColors = false;
			s_config.useConsoleColors = true;
		}

		s_checkedColorSupport = true;
	}
	return s_hasColorSupport;
}

bool LogFormatter::IsDebuggerPresent()
{
	return ::IsDebuggerPresent() == TRUE;
}

std::string LogFormatter::GetTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::stringstream ss;
	std::tm timeinfo;
	localtime_s(&timeinfo, &time);

	ss << std::put_time(&timeinfo, s_config.timeFormat.c_str());
	if (s_config.showMilliseconds)
	{
		ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
	}

	return ss.str();
}

void LogFormatter::SetConsoleColor(LogColor color)
{
	if (s_config.useConsoleColors && HasColorSupport())
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
	}
}

const char* LogFormatter::GetAnsiColor(LogColor color)
{
	if (!s_config.useAnsiColors)
		return "";

	switch (color)
	{
		case LogColor::Red:
			return "\033[31;1m";
		case LogColor::Green:
			return "\033[32;1m";
		case LogColor::Blue:
			return "\033[34;1m";
		case LogColor::Yellow:
			return "\033[33;1m";
		case LogColor::White:
			return "\033[37;1m";
		case LogColor::Cyan:
			return "\033[36;1m";
		case LogColor::Magenta:
			return "\033[35;1m";
		default:
			return "\033[0m";
	}
}

LogFormatter::LogColor LogFormatter::GetColorForLevel(LogLevel level)
{
	switch (level)
	{
		case LogLevel::Debug:
			return LogColor::Cyan;
		case LogLevel::Info:
			return LogColor::White;
		case LogLevel::Warning:
			return LogColor::Yellow;
		case LogLevel::Error:
			return LogColor::Red;
		case LogLevel::Critical:
			return LogColor::Magenta;
		default:
			return LogColor::Default;
	}
}

std::string LogFormatter::GetLevelString(LogLevel level)
{
	switch (level)
	{
		case LogLevel::Debug:
			return "DEBUG";
		case LogLevel::Info:
			return "INFO";
		case LogLevel::Warning:
			return "WARN";
		case LogLevel::Error:
			return "ERROR";
		case LogLevel::Critical:
			return "CRITICAL";
		default:
			return "UNKNOWN";
	}
}

void LogFormatter::AddLogCallback(LogCallback callback)
{
	s_callbacks.push_back(callback);
}

std::string LogFormatter::FormatLogMessage(const char* file, int line, const std::string& msg, LogLevel level)
{
	std::filesystem::path filePath(file);
	std::stringstream ss;
	LogColor color = GetColorForLevel(level);

	bool useAnsi = s_config.useAnsiColors && HasColorSupport();
	bool useConsole = s_config.useConsoleColors && !IsRunningInVisualStudio();

	// Start color
	if (useAnsi)
		ss << GetAnsiColor(color);
	if (useConsole)
		SetConsoleColor(color);

	// Timestamp
	if (s_config.showTimestamp)
		ss << "[" << GetTimestamp() << "] ";

	// Log level
	if (s_config.showLogLevel)
		ss << "[" << GetLevelString(level) << "] ";

	// File and line
	if (s_config.showFileLine)
		ss << "[" << filePath.filename().string() << ":" << line << "] ";

	// Message
	ss << msg;

	// Reset color
	if (useAnsi)
		ss << GetAnsiColor(LogColor::Default);
	if (useConsole)
		SetConsoleColor(LogColor::Default);

	return ss.str();
}
