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

IDTransformerThin::IDTransformerThin(size_t num_cache_ids) : checks_(reinterpret_cast<uint16_t *>(
                                                                         AlignMalloc(num_cache_ids * sizeof(uint16_t),
                                                                                     alignment())),
                                                                     free),
                                                             num_cache_ids_(num_cache_ids) {
  if (num_cache_ids_ % (alignment() / sizeof(uint16_t)) != 0) [[unlikely]] {
    throw std::runtime_error("must be div by 64");
  }
  std::fill(&checks_[0], &checks_[num_cache_ids_], std::numeric_limits<uint16_t>::max());
}

std::span<const int64_t> IDTransformerThin::Apply() {
  result_caches_.clear();
  result_caches_.reserve(hash_values_.size());
  for (size_t offset = 0; offset < hash_values_.size(); ++offset) {
    auto &hash_value = hash_values_[offset];
    constexpr size_t n_elems_per_slot = alignment() / sizeof(uint16_t);
    uint32_t offset_in_slot_ = hash_value.check_index_ % n_elems_per_slot;
    uint32_t slot_id = hash_value.check_index_ / n_elems_per_slot;
    uint16_t *check_begin = &checks_[slot_id * n_elems_per_slot];
    uint32_t i = 0;
    for (; i < n_elems_per_slot; ++i, ++offset_in_slot_) {  // TODO: Make it simd
      offset_in_slot_ %= n_elems_per_slot;
      if (check_begin[offset_in_slot_] == std::numeric_limits<uint16_t>::max()) { // empty slot, fill it
        // TODO: fetch params?
        check_begin[offset_in_slot_] = hash_value.check_;
        result_caches_.emplace_back(offset_in_slot_ + slot_id * n_elems_per_slot);
        break;
      }
      if (check_begin[offset_in_slot_] == hash_value.check_) {  // found
        result_caches_.emplace_back(offset_in_slot_ + slot_id * n_elems_per_slot);
        break;
      }
      // not match, continue
    }

    if (i == n_elems_per_slot) {
      // full, need evict
      throw std::runtime_error("need evict");
    }
  }

  return result_caches_;
}
}