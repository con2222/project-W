#include <C2Core/c2_log.hpp>
#include <C2Core/Color.hpp>


#include <iostream>
#include <cstdio>
#include <cstdarg>


#define C2_ERROR(message) C2Core::Log::ErrorDetailed(message, __FILE__, __LINE__)



namespace C2Core::Log {

constexpr size_t BUFFER_SIZE = 1024;

void info(std::string_view message) {
    std::cout << C2Core::ConsoleColor::FG_GREEN << "[INFO] " << C2Core::ConsoleColor::RESET << message << "\n";
}

void info(const char* format, ...) {
    va_list argList;
    char buffer[BUFFER_SIZE];
    
    va_start(argList, format);
    vsnprintf(buffer, BUFFER_SIZE, format, argList);
    va_end(argList);

    std::cout << C2Core::ConsoleColor::FG_GREEN << "[INFO] " << C2Core::ConsoleColor::RESET << buffer << "\n";
}

void warning(std::string_view message) {
    std::cout << C2Core::ConsoleColor::FG_YELLOW << "[WARN] " << C2Core::ConsoleColor::RESET << message << "\n";
}

void warning(const char* format, ...) {
    va_list argList;
    char buffer[BUFFER_SIZE];
    
    va_start(argList, format);
    vsnprintf(buffer, BUFFER_SIZE, format, argList);
    va_end(argList);

    std::cout << C2Core::ConsoleColor::FG_YELLOW << "[WARN] " << C2Core::ConsoleColor::RESET << buffer << "\n";
}


void error(std::string_view message) {
    std::cerr << C2Core::ConsoleColor::FG_RED << "[ERROR] " << C2Core::ConsoleColor::RESET << message << "\n";
}

void error(const char* format, ...) {
    va_list argList;
    char buffer[BUFFER_SIZE];

    va_start(argList, format);
    vsnprintf(buffer, BUFFER_SIZE, format, argList);
    va_end(argList);

    std::cerr << C2Core::ConsoleColor::FG_RED << "[ERROR] " << C2Core::ConsoleColor::RESET << buffer << "\n";
}

void errorDetailed(std::string_view message, const char* file, int line) {
    std::cerr << C2Core::ConsoleColor::FG_RED << "[ERROR] " << C2Core::ConsoleColor::RESET
              << message 
              << C2Core::ConsoleColor::FG_CYAN << " (" << file << ":" << line << ")" 
              << C2Core::ConsoleColor::RESET << "\n";
}

void trace(const char* colorCode, const char* format, ...) {
    va_list argList;
    char buffer[BUFFER_SIZE];

    va_start(argList, format);
    vsnprintf(buffer, BUFFER_SIZE, format, argList);
    va_end(argList);

    std::cout << colorCode << buffer << "\n";
}

} // C2Core::Log
