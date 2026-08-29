#ifndef C2_HASH_HPP
#define C2_HASH_HPP

#include <cstdint>
#include <string_view>


namespace C2Core::Hash {

    constexpr uint32_t fnvA1Hash(std::string_view s) {
        uint32_t fnvOffsetBasis = 0x811c9dc5;
        uint32_t fnvPrime = 0x01000193;
        uint32_t hash = fnvOffsetBasis;

        for (const auto& symbol : s) {
            hash ^= static_cast<uint8_t>(symbol);
            hash *= fnvPrime;
        }

        return hash;
    }

} // C2Core::Hash


#endif // C2_HASH_HPP
