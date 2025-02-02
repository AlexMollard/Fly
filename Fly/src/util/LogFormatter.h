#pragma once

#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004

class LogFormatter
{
public:
	enum class LogColor
	{
		Red = FOREGROUND_RED | FOREGROUND_INTENSITY,
		Green = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		Blue = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		Yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		Cyan = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		Magenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		Default = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
	};

	enum class LogLevel
	{
		Debug,
		Info,
		Warning,
		Error,
		Critical
	};

	struct LogConfig
	{
		bool showTimestamp = true;
		bool showFileLine = true;
		bool showLogLevel = true;
		bool useAnsiColors = true;
		bool useConsoleColors = true;
		std::string timeFormat = "%Y-%m-%d %H:%M:%S";
		bool showMilliseconds = true;
	};

	static void Initialize();
	static void SetConfig(const LogConfig& config);
	static LogConfig& GetConfig();

	static std::string GetTimestamp();
	static void SetConsoleColor(LogColor color);
	static const char* GetAnsiColor(LogColor color);
	static LogColor GetColorForLevel(LogLevel level);
	static std::string GetLevelString(LogLevel level);

	static std::string FormatLogMessage(const char* file, int line, const std::string& msg, LogLevel level = LogLevel::Info);

	// Add callback for log interceptors (useful for GUI display, file logging, etc)
	using LogCallback = std::function<void(LogLevel, const std::string&)>;
	static void AddLogCallback(LogCallback callback);

private:
	static bool EnableVirtualTerminalProcessing();
	static bool IsRunningInVisualStudio();
	static bool HasColorSupport();
	static bool IsDebuggerPresent();
	static bool s_checkedColorSupport;
	static bool s_hasColorSupport;
	static LogConfig s_config;
	static std::vector<LogCallback> s_callbacks;
};
