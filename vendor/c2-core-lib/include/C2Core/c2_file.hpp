#pragma once

#include <string>
#include <string_view>
#include <optional>


namespace C2Core::File {
    std::optional<std::string> readText(std::string_view filepath);
}
