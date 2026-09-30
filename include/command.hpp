#include <miniaudio.h>

#include <C2Core/c2_log.hpp>
#include <cstdint>

namespace c2::audio {
enum class CommandType : uint8_t {
    Select,
    Next,
    Prev,
    Play,
    Pause,
    Seek,
    SetVolume,
};

struct Command {
    CommandType command;

    union {
        int index;
        float volume;
        ma_uint64 frame;
    };
};

}  // namespace c2::audio
