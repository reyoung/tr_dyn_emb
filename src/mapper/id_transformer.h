#pragma once
#include <memory>
#include <span>
#include <string.h>
#include <vector>
#include <iostream>

namespace trde::mapper {

class IDTransformerThin {
public:
  explicit IDTransformerThin(size_t num_cache_ids);
protected:
  static constexpr uint32_t alignment() {
    return 64;  // cache line size
  }

  std::unique_ptr<int64_t[]> Apply();
  std::unique_ptr<uint8_t[], decltype(&free)> checks_;
  size_t num_cache_ids_;
  struct HashValue {
    uint8_t check_;
    uint32_t offset_: 24;
    uint32_t check_index_;
  };

  static_assert(sizeof(HashValue) == sizeof(uint64_t));
  std::vector<HashValue> hash_values_;
};

template<typename SlotHash, typename CheckHash>
class IDTransformer : public IDTransformerThin {
public:
  IDTransformer(size_t num_cache_ids, SlotHash slot_hash, CheckHash check_hash)
      : IDTransformerThin(num_cache_ids), slot_hash_(std::move(slot_hash)),
        check_hash_(std::move(check_hash)) {}

  std::unique_ptr<int64_t[]> operator()(std::span<const int64_t> global_ids) {
    hash_values_.clear();
    hash_values_.reserve(global_ids.size());

    for (uint32_t i = 0; i < global_ids.size(); ++i) {
      auto id = global_ids[i];
      HashValue value{};
      value.check_index_ = static_cast<uint32_t>(slot_hash_(id) % num_cache_ids_);
      // use UINT8_MAX to identify empty slot, so check hash range is max-1
      value.check_ = static_cast<uint8_t>(check_hash_(id) % (std::numeric_limits<uint8_t>::max() - 1));
      value.offset_ = i;
      hash_values_.emplace_back(std::move(value));
    }
    return Apply();
  }

private:
  SlotHash slot_hash_;
  CheckHash check_hash_;
};
}