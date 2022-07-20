#include "catch2/catch_all.hpp"
#include "id_transformer.h"
#include "murmur_hasher.h"
#include <iostream>

namespace trde::mapper {

TEST_CASE("trde_mapper_id_transformer") {
  MurMurHash<int64_t> value_hash(71);
  MurMurHash<int64_t> check_hash(7);

  IDTransformer transformer(300000000, std::move(value_hash), std::move(check_hash));

  auto result = transformer(std::vector<int64_t>{1, 2, 3, 4});
  REQUIRE(result[0] != result[1]);
  REQUIRE(result[1] != result[2]);
  REQUIRE(result[2] != result[3]);

}

}