#include "catch2/catch_all.hpp"
#include "id_transformer.h"
#include "murmur_hasher.h"
#include <iostream>
#include <algorithm>

namespace trde::mapper {

TEST_CASE("trde_mapper_id_transformer") {
//  SECTION("normal") {
//    MurMurHash<int64_t> value_hash(71);
//    MurMurHash<int64_t> check_hash(7);
//
//    IDTransformer transformer(300000000, std::move(value_hash), std::move(check_hash));
//
//    auto result = transformer(std::vector<int64_t>{1, 2, 3, 4});
//    REQUIRE(result[0] != result[1]);
//    REQUIRE(result[1] != result[2]);
//    REQUIRE(result[2] != result[3]);
//  }

  BENCHMARK_ADVANCED("simple")(Catch::Benchmark::Chronometer meter) {
      std::vector<std::vector<int64_t>> global_id_batches;
      constexpr size_t num_iter = 10;
      constexpr size_t total_size = 1024;
      constexpr float hot_percentage = 0.5;
      constexpr size_t num_embeddings = 300000000;
      constexpr size_t hot_size = total_size * hot_percentage;
      constexpr int64_t hot_limit = 1000000;
      constexpr int64_t total_limit = 10000000000;
      constexpr int64_t batch_size = 1024;
      std::random_device dev;
      std::default_random_engine engine(dev());
      std::uniform_int_distribution<int64_t> hot_dist(0, hot_limit);
      std::uniform_int_distribution<int64_t> normal_dist(0, total_limit);

      global_id_batches.reserve(num_iter);
      for (size_t i = 0; i < num_iter; ++i) {
        std::vector<int64_t> global_ids;
        global_ids.reserve(total_size * batch_size);
        for (int64_t j = 0; j < batch_size; ++j) {
          for (int64_t k = 0; k < total_size; ++k) {
            if (k < hot_size) {
              global_ids.emplace_back(hot_dist(engine));
            } else {
              global_ids.emplace_back(normal_dist(engine));
            }
          }
        }
        std::shuffle(global_ids.begin(), global_ids.end(), engine);
        global_id_batches.emplace_back(global_ids);
      }

      meter.measure([&] {
        MurMurHash<int64_t> value_hash(71);
        MurMurHash<int64_t> check_hash(7);
        IDTransformer transformer(num_embeddings, std::move(value_hash), std::move(check_hash));
        int64_t sum = 0;
        for (auto &batch : global_id_batches) {
          auto result = transformer(batch);
          sum = std::accumulate(&result[0], &result[batch.size()], sum, std::plus<>());
        }
        return sum;
      });
    };

}

}