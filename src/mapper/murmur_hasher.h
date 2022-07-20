#pragma once
#include <string_view>

namespace trde::mapper {

namespace functions {
uint32_t MurMurHashV3_32(std::string_view bytes, uint32_t seed);
}

enum class MurMurVersion { V1, V2, V3 };

template<typename T, MurMurVersion version = MurMurVersion::V3, int BitSize = 32>
struct MurMurHash;

template<typename T>
struct MurMurHash<T, MurMurVersion::V3, 32> {
  using result_type = uint32_t;

  explicit MurMurHash(uint32_t seed) : seed_(seed) {}
  uint32_t operator()(const T &val) const {
    return functions::MurMurHashV3_32(std::string_view(
        reinterpret_cast<const char *>(&val), sizeof(val)), seed_);
  }
  uint32_t seed_;
};

}