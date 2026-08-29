#pragma once

#include <cstdint>

namespace C2Core {

namespace ConsoleColor {
    constexpr const char* RESET = "\033[0m";

    constexpr const char* STYLE_BOLD = "\033[1m";
    constexpr const char* STYLE_DIM = "\033[2m";
    constexpr const char* STYLE_ITALIC = "\033[3m";
    constexpr const char* STYLE_UNDERLINE = "\033[4m";
    constexpr const char* STYLE_BLINK = "\033[5m";
    constexpr const char* STYLE_FAST_BLINK = "\033[6m";
    constexpr const char* STYLE_REVERSE = "\033[7m";
    constexpr const char* STYLE_HIDDEN = "\033[8m";
    constexpr const char* STYLE_STRIKE = "\033[9m";

    constexpr const char* FG_BLACK = "\033[30m";
    constexpr const char* FG_RED = "\033[31m";
    constexpr const char* FG_GREEN = "\033[32m";
    constexpr const char* FG_YELLOW = "\033[33m";
    constexpr const char* FG_BLUE = "\033[34m";
    constexpr const char* FG_MAGENTA = "\033[35m";
    constexpr const char* FG_CYAN = "\033[36m";
    constexpr const char* FG_WHITE = "\033[37m";

    constexpr const char* FG_BRIGHT_BLACK = "\033[90m";
    constexpr const char* FG_BRIGHT_RED = "\033[91m";
    constexpr const char* FG_BRIGHT_GREEN = "\033[92m";
    constexpr const char* FG_BRIGHT_YELLOW = "\033[93m";
    constexpr const char* FG_BRIGHT_BLUE = "\033[94m";
    constexpr const char* FG_BRIGHT_MAGENTA = "\033[95m";
    constexpr const char* FG_BRIGHT_CYAN = "\033[96m";
    constexpr const char* FG_BRIGHT_WHITE = "\033[97m";

    constexpr const char* BG_BLACK = "\033[40m";
    constexpr const char* BG_RED = "\033[41m";
    constexpr const char* BG_GREEN = "\033[42m";
    constexpr const char* BG_YELLOW = "\033[43m";
    constexpr const char* BG_BLUE = "\033[44m";
    constexpr const char* BG_MAGENTA = "\033[45m";
    constexpr const char* BG_CYAN = "\033[46m";
    constexpr const char* BG_WHITE = "\033[47m";

    constexpr const char* BG_BRIGHT_BLACK = "\033[100m";
    constexpr const char* BG_BRIGHT_RED = "\033[101m";
    constexpr const char* BG_BRIGHT_GREEN = "\033[102m";
    constexpr const char* BG_BRIGHT_YELLOW = "\033[103m";
    constexpr const char* BG_BRIGHT_BLUE = "\033[104m";
    constexpr const char* BG_BRIGHT_MAGENTA = "\033[105m";
    constexpr const char* BG_BRIGHT_CYAN = "\033[106m";
    constexpr const char* BG_BRIGHT_WHITE = "\033[107m";
}

enum class Color : uint32_t {
    Transparent = 0x00000000,
    Black       = 0xFF000000,
    White       = 0xFFFFFFFF,
    Red         = 0xFFFF0000,
    Green       = 0xFF00FF00,
    Blue        = 0xFF0000FF,
    Yellow      = 0xFFFFFF00,
    Cyan        = 0xFF00FFFF,
    Magenta     = 0xFFFF00FF,
    Gray        = 0xFF808080,
    Silver      = 0xFFC0C0C0,
    Maroon      = 0xFF800000,
    Olive       = 0xFF808000,
    DarkGreen   = 0xFF008000,
    Purple      = 0xFF800080,
    Teal        = 0xFF008080,
    Navy        = 0xFF000080,
    Orange      = 0xFFFFA500,
    Pink        = 0xFFFFC0CB,
    Brown       = 0xFFA52A2A
};

} // C2Core

