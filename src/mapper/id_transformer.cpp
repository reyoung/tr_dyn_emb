#include "id_transformer.h"

namespace trde::mapper {

static void *AlignMalloc(size_t num_cache_ids, size_t align) {
  void *ptr;
  int ok = posix_memalign(&ptr, align, num_cache_ids);
  if (ok != 0) [[unlikely]] {
    throw std::runtime_error("malloc failed");
  }
  return ptr;
}

IDTransformerThin::IDTransformerThin(size_t num_cache_ids) : checks_(reinterpret_cast<uint8_t *>(
                                                                         AlignMalloc(num_cache_ids, alignment())),
                                                                     free),
                                                             num_cache_ids_(num_cache_ids) {
  if (num_cache_ids_ % alignment() != 0) [[unlikely]] {
    throw std::runtime_error("must be div by 64");
  }
  std::fill(&checks_[0], &checks_[num_cache_ids_], std::numeric_limits<uint8_t>::max());
}

std::unique_ptr<int64_t[]> IDTransformerThin::Apply() {
  // does sort make faster?
  std::sort(hash_values_.begin(), hash_values_.end(), [](const HashValue &a, const HashValue &b) {
    return a.check_index_ < b.check_index_;
  });

  std::unique_ptr<int64_t[]> result(new int64_t[hash_values_.size()]);

  for (auto &hash_value : hash_values_) {
    uint32_t offset = hash_value.check_index_ % alignment();
    uint32_t slot_id = hash_value.check_index_ / alignment();
    uint8_t *check_begin = &checks_[slot_id * alignment()];
    uint32_t i = 0;
    for (; i < alignment(); ++i, ++offset) {  // TODO: Make it simd
      offset %= alignment();
      if (check_begin[offset] == std::numeric_limits<uint8_t>::max()) { // empty slot, fill it
        // TODO: fetch params?
        check_begin[offset] = hash_value.check_;
        result[hash_value.offset_] = i + hash_value.check_index_ * alignment();
        break;
      }
      if (check_begin[offset] == hash_value.check_) {  // found
        result[hash_value.offset_] = i + hash_value.check_index_ * alignment();
        break;
      }
      // not match, continue
    }

    if (i == alignment()) {
      // full, need evict
      throw std::runtime_error("need evict");
    }
  }

  return result;
}
}