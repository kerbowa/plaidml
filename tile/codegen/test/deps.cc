// Copyright 2018 Intel Corporation.

#include <gtest/gtest.h>

#include "testing/matchers.h"
#include "tile/codegen/deps.h"
#include "tile/stripe/stripe.h"
#include "tile/stripe/stripe.pb.h"

namespace gp = google::protobuf;

using ::testing::EqualsProtoText;

namespace vertexai {
namespace tile {
namespace codegen {

TEST(DepsTest, SmallDepMix) {
  auto input_text = R"(
    location: { unit { } }
    stmts {
      tags: "main"
      block {
        location: { unit { } }
        refs {
          into: "b1"
          location: { unit { } }
          shape { type: FLOAT32 dimensions: {size:1 stride:1} } 
        }
        refs {
          into: "b2"
          location: { unit { } }
          shape { type: FLOAT32 dimensions: {size:1 stride:1} }
        }
        stmts { load { from:"b1" into:"$1" } }
        stmts { store { from:"$1" into:"b2" } }
        stmts { load { from:"b2" into:"$2" } deps: 0}
        stmts { constant { name:"$3" iconst: 0 } }
        stmts { intrinsic { name:"ADD" type:FLOAT32 inputs:"$2" inputs:"$3" outputs:"$4"} }
        stmts { special { name:"COPY" inputs:"b1" outputs:"b2"} }
        stmts { store { from:"$4" into:"b2"} }
      }
    }
  )";
  stripe::proto::Block input_proto;
  gp::TextFormat::ParseFromString(input_text, &input_proto);

  auto block = stripe::FromProto(input_proto);
  proto::GenericPass options;
  options.add_reqs("main");
  ComputeDepsPass(block.get(), options);

  const char* expected = R"(
    location: { unit { } }
    stmts {
      tags: "main"
      block {
        location: { unit { } }
        refs {
          into: "b1"
          location: { unit { } }
          shape { type: FLOAT32 dimensions: {size:1 stride:1} } 
        }
        refs {
          into: "b2"
          location: { unit { } }
          shape { type: FLOAT32 dimensions: {size:1 stride:1} }
        }
        stmts { load { from:"b1" into:"$1" } }
        stmts { store { from:"$1" into:"b2" } deps: 0 }
        stmts { load { from:"b2" into:"$2" } deps: 1}
        stmts { constant { name:"$3" iconst: 0 } }
        stmts { intrinsic { name:"ADD" type:FLOAT32 inputs:"$2" inputs:"$3" outputs:"$4"} deps: 2 deps: 3}
        stmts { special { name:"COPY" inputs:"b1" outputs:"b2"} deps: 2}
        stmts { store { from:"$4" into:"b2"} deps: 4 deps: 5}
      }
    }
  )";

  auto output_proto = IntoProto(*block);
  EXPECT_THAT(output_proto, EqualsProtoText(expected));
}

}  // namespace codegen
}  // namespace tile
}  // namespace vertexai
