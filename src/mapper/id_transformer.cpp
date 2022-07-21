#include "id_transformer.h"
#include <string>

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
                                                             num_cache_ids_(num_cache_ids),
                                                             flag_cache_(reinterpret_cast<int8_t *>(AlignMalloc(
                                                                 alignment(),
                                                                 alignment()))) {
  if (num_cache_ids_ % (alignment() / sizeof(uint16_t)) != 0) [[unlikely]] {
    throw std::runtime_error("must be div by 64");
  }
  std::fill(&checks_[0], &checks_[num_cache_ids_], std::numeric_limits<uint16_t>::max());
  std::fill(&flag_cache_[0], &flag_cache_[alignment()], 0);
}

static uint32_t SearchInSlot(uint16_t *slot_begin, uint16_t check, uint32_t n_elems_per_slot, uint32_t offset_hint) {
  for (uint32_t i = 0; i < n_elems_per_slot; ++i) {
    if (slot_begin[offset_hint] == std::numeric_limits<uint16_t>::max()) {
      slot_begin[offset_hint] = check;
      return offset_hint;
    }
    if (slot_begin[offset_hint] == check) {
      return offset_hint;
    }
    ++offset_hint;
    offset_hint %= n_elems_per_slot;
  }
  return n_elems_per_slot;
}

std::span<const int64_t> IDTransformerThin::Apply() {
  result_caches_.clear();
  result_caches_.reserve(hash_values_.size());
  for (size_t offset = 0; offset < hash_values_.size(); ++offset) {
    auto &hash_value = hash_values_[offset];
    constexpr size_t n_elems_per_slot = alignment() / sizeof(uint16_t);
    uint32_t offset_in_slot_ = hash_value.check_index_ % n_elems_per_slot;
    uint32_t slot_id = hash_value.check_index_ / n_elems_per_slot;
    uint16_t *slot_begin = &checks_[slot_id * n_elems_per_slot];

    offset_in_slot_ = SearchInSlot(slot_begin, hash_value.check_, n_elems_per_slot, offset_in_slot_);
    if (offset_in_slot_ == n_elems_per_slot) [[unlikely]] {
      throw std::runtime_error("need evict");
    } else {
      result_caches_.emplace_back(offset_in_slot_ + slot_id * n_elems_per_slot);
    }
  }

  return result_caches_;
}
}