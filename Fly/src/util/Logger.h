#pragma once
#include "LogFormatter.h"

// Logging macros
#ifdef _DEBUG
#	define LOG_DEBUG(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Debug) << std::endl
#	define LOG_INFO(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Info) << std::endl
#	define LOG_WARN(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Warning) << std::endl
#	define LOG_ERROR(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Error) << std::endl
#	define LOG_CRITICAL(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Critical) << std::endl
#else
#	define LOG_DEBUG(msg) ((void) 0)
#	define LOG_INFO(msg) ((void) 0)
#	define LOG_WARN(msg) ((void) 0)
#	define LOG_ERROR(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Error) << std::endl
#	define LOG_CRITICAL(...) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, std::format(__VA_ARGS__), LogFormatter::LogLevel::Critical) << std::endl
#endif
