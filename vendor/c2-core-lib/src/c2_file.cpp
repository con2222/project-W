#include "../include/C2Core/c2_file.hpp"
#include "../include/C2Core//c2_log.hpp"
#include <fstream>
#include <optional>


namespace C2Core::File {
    std::optional<std::string> readText(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::ate | std::ios::binary | std::ios::in);
        if (!file.is_open()) {
            C2Core::Log::error("Failed to open file: %s", filepath.c_str());
            return std::nullopt;
        }

        size_t size = file.tellg();
        file.seekg(0);

        std::string result;
        result.resize(size);
        file.read(result.data(), size);

        return result;
    }
}
