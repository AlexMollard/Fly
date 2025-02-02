#pragma once
#include "LogFormatter.h"

// Logging macros
#ifdef _DEBUG
#	define LOG_DEBUG(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Debug) << std::endl
#	define LOG_INFO(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Info) << std::endl
#	define LOG_WARN(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Warning) << std::endl
#	define LOG_ERROR(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Error) << std::endl
#	define LOG_CRITICAL(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Critical) << std::endl
#else
#	define LOG_DEBUG(msg) ((void) 0)
#	define LOG_INFO(msg) ((void) 0)
#	define LOG_WARN(msg) ((void) 0)
#	define LOG_ERROR(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Error) << std::endl
#	define LOG_CRITICAL(msg) std::cout << LogFormatter::FormatLogMessage(__FILE__, __LINE__, msg, LogFormatter::LogLevel::Critical) << std::endl
#endif
