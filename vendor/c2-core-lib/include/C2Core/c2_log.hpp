#ifndef C2_LOG_HPP
#define C2_LOG_HPP

#include <string_view>

namespace C2Core::Log {

void info(std::string_view message);
void info(const char* format, ...);

void warning(std::string_view message);
void warning(const char* format, ...);

void error(std::string_view message);
void error(const char* format, ...);

void errorDetailed(std::string_view message, const char* file, int line);
void trace(const char* colorCode, const char* format, ...);

}  // namespace C2Core::Log

#endif  // C2_LOG_HPP
