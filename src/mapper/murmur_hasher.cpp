#include "murmur_hasher.h"
#include "MurmurHash3.h"
namespace trde::mapper::functions {
uint32_t MurMurHashV3_32(std::string_view bytes, uint32_t seed) {
  uint32_t result;
  MurmurHash3_x86_32(bytes.data(), static_cast<int>(bytes.size()), seed, &result);
  return result;
}
}