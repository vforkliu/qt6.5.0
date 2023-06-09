// Copyright 2022 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/tint/transform/builtin_polyfill.h"

#include <utility>

#include "src/tint/transform/test_helper.h"

namespace tint::transform {
namespace {

using Level = BuiltinPolyfill::Level;

using BuiltinPolyfillTest = TransformTest;

TEST_F(BuiltinPolyfillTest, ShouldRunEmptyModule) {
    auto* src = R"()";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
}

TEST_F(BuiltinPolyfillTest, EmptyModule) {
    auto* src = R"()";

    auto* expect = src;

    auto got = Run<BuiltinPolyfill>(src);

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// acosh
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillAcosh(Level level) {
    BuiltinPolyfill::Builtins builtins;
    builtins.acosh = level;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunAcosh) {
    auto* src = R"(
fn f() {
  acosh(1.0);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src, polyfillAcosh(Level::kNone)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillAcosh(Level::kClampParameters)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillAcosh(Level::kFull)));
}

TEST_F(BuiltinPolyfillTest, Acosh_Full_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = acosh(1234);
}
)";

    auto* expect = R"(
fn tint_acosh(x : f32) -> f32 {
  return log((x + sqrt(((x * x) - 1))));
}

fn f() {
  let r : f32 = tint_acosh(1234);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAcosh(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Acosh_Full_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = acosh(vec3<f32>(1234));
}
)";

    auto* expect = R"(
fn tint_acosh(x : vec3<f32>) -> vec3<f32> {
  return log((x + sqrt(((x * x) - 1))));
}

fn f() {
  let r : vec3<f32> = tint_acosh(vec3<f32>(1234));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAcosh(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Acosh_Range_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = acosh(1234);
}
)";

    auto* expect = R"(
fn tint_acosh(x : f32) -> f32 {
  return select(acosh(x), 0.0, (x < 1.0));
}

fn f() {
  let r : f32 = tint_acosh(1234);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAcosh(Level::kRangeCheck));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Acosh_Range_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = acosh(vec3<f32>(1234));
}
)";

    auto* expect = R"(
fn tint_acosh(x : vec3<f32>) -> vec3<f32> {
  return select(acosh(x), vec3<f32>(0.0), (x < vec3<f32>(1.0)));
}

fn f() {
  let r : vec3<f32> = tint_acosh(vec3<f32>(1234));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAcosh(Level::kRangeCheck));

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// asinh
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillSinh() {
    BuiltinPolyfill::Builtins builtins;
    builtins.asinh = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunAsinh) {
    auto* src = R"(
fn f() {
  asinh(1.0);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillSinh()));
}

TEST_F(BuiltinPolyfillTest, Asinh_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = asinh(1234);
}
)";

    auto* expect = R"(
fn tint_sinh(x : f32) -> f32 {
  return log((x + sqrt(((x * x) + 1))));
}

fn f() {
  let r : f32 = tint_sinh(1234);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSinh());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Asinh_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = asinh(vec3<f32>(1234));
}
)";

    auto* expect = R"(
fn tint_sinh(x : vec3<f32>) -> vec3<f32> {
  return log((x + sqrt(((x * x) + 1))));
}

fn f() {
  let r : vec3<f32> = tint_sinh(vec3<f32>(1234));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSinh());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// atanh
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillAtanh(Level level) {
    BuiltinPolyfill::Builtins builtins;
    builtins.atanh = level;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunAtanh) {
    auto* src = R"(
fn f() {
  atanh(1.0);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src, polyfillAtanh(Level::kNone)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillAtanh(Level::kClampParameters)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillAtanh(Level::kFull)));
}

TEST_F(BuiltinPolyfillTest, Atanh_Full_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = atanh(1234);
}
)";

    auto* expect = R"(
fn tint_atanh(x : f32) -> f32 {
  return (log(((1 + x) / (1 - x))) * 0.5);
}

fn f() {
  let r : f32 = tint_atanh(1234);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAtanh(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Atanh_Full_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = atanh(vec3<f32>(1234));
}
)";

    auto* expect = R"(
fn tint_atanh(x : vec3<f32>) -> vec3<f32> {
  return (log(((1 + x) / (1 - x))) * 0.5);
}

fn f() {
  let r : vec3<f32> = tint_atanh(vec3<f32>(1234));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAtanh(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Atanh_Range_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = atanh(1234);
}
)";

    auto* expect = R"(
fn tint_atanh(x : f32) -> f32 {
  return select(atanh(x), 0.0, (x >= 1.0));
}

fn f() {
  let r : f32 = tint_atanh(1234);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAtanh(Level::kRangeCheck));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Atanh_Range_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = atanh(vec3<f32>(1234));
}
)";

    auto* expect = R"(
fn f() {
  let r : vec3<f32> = atanh(vec3<f32>(1234));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillAcosh(Level::kRangeCheck));

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// countLeadingZeros
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillCountLeadingZeros() {
    BuiltinPolyfill::Builtins builtins;
    builtins.count_leading_zeros = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunCountLeadingZeros) {
    auto* src = R"(
fn f() {
  countLeadingZeros(0xf);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillCountLeadingZeros()));
}

TEST_F(BuiltinPolyfillTest, CountLeadingZeros_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = countLeadingZeros(15);
}
)";

    auto* expect = R"(
fn tint_count_leading_zeros(v : i32) -> i32 {
  var x = u32(v);
  let b16 = select(0u, 16u, (x <= 65535u));
  x = (x << b16);
  let b8 = select(0u, 8u, (x <= 16777215u));
  x = (x << b8);
  let b4 = select(0u, 4u, (x <= 268435455u));
  x = (x << b4);
  let b2 = select(0u, 2u, (x <= 1073741823u));
  x = (x << b2);
  let b1 = select(0u, 1u, (x <= 2147483647u));
  let is_zero = select(0u, 1u, (x == 0u));
  return i32((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : i32 = tint_count_leading_zeros(15);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountLeadingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountLeadingZeros_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = countLeadingZeros(15u);
}
)";

    auto* expect = R"(
fn tint_count_leading_zeros(v : u32) -> u32 {
  var x = u32(v);
  let b16 = select(0u, 16u, (x <= 65535u));
  x = (x << b16);
  let b8 = select(0u, 8u, (x <= 16777215u));
  x = (x << b8);
  let b4 = select(0u, 4u, (x <= 268435455u));
  x = (x << b4);
  let b2 = select(0u, 2u, (x <= 1073741823u));
  x = (x << b2);
  let b1 = select(0u, 1u, (x <= 2147483647u));
  let is_zero = select(0u, 1u, (x == 0u));
  return u32((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : u32 = tint_count_leading_zeros(15u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountLeadingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountLeadingZeros_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = countLeadingZeros(vec3<i32>(15));
}
)";

    auto* expect = R"(
fn tint_count_leading_zeros(v : vec3<i32>) -> vec3<i32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(0u), vec3<u32>(16u), (x <= vec3<u32>(65535u)));
  x = (x << b16);
  let b8 = select(vec3<u32>(0u), vec3<u32>(8u), (x <= vec3<u32>(16777215u)));
  x = (x << b8);
  let b4 = select(vec3<u32>(0u), vec3<u32>(4u), (x <= vec3<u32>(268435455u)));
  x = (x << b4);
  let b2 = select(vec3<u32>(0u), vec3<u32>(2u), (x <= vec3<u32>(1073741823u)));
  x = (x << b2);
  let b1 = select(vec3<u32>(0u), vec3<u32>(1u), (x <= vec3<u32>(2147483647u)));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(1u), (x == vec3<u32>(0u)));
  return vec3<i32>((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : vec3<i32> = tint_count_leading_zeros(vec3<i32>(15));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountLeadingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountLeadingZeros_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = countLeadingZeros(vec3<u32>(15u));
}
)";

    auto* expect = R"(
fn tint_count_leading_zeros(v : vec3<u32>) -> vec3<u32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(0u), vec3<u32>(16u), (x <= vec3<u32>(65535u)));
  x = (x << b16);
  let b8 = select(vec3<u32>(0u), vec3<u32>(8u), (x <= vec3<u32>(16777215u)));
  x = (x << b8);
  let b4 = select(vec3<u32>(0u), vec3<u32>(4u), (x <= vec3<u32>(268435455u)));
  x = (x << b4);
  let b2 = select(vec3<u32>(0u), vec3<u32>(2u), (x <= vec3<u32>(1073741823u)));
  x = (x << b2);
  let b1 = select(vec3<u32>(0u), vec3<u32>(1u), (x <= vec3<u32>(2147483647u)));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(1u), (x == vec3<u32>(0u)));
  return vec3<u32>((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : vec3<u32> = tint_count_leading_zeros(vec3<u32>(15u));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountLeadingZeros());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// countTrailingZeros
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillCountTrailingZeros() {
    BuiltinPolyfill::Builtins builtins;
    builtins.count_trailing_zeros = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunCountTrailingZeros) {
    auto* src = R"(
fn f() {
  countTrailingZeros(0xf);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillCountTrailingZeros()));
}

TEST_F(BuiltinPolyfillTest, CountTrailingZeros_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = countTrailingZeros(15);
}
)";

    auto* expect = R"(
fn tint_count_trailing_zeros(v : i32) -> i32 {
  var x = u32(v);
  let b16 = select(16u, 0u, bool((x & 65535u)));
  x = (x >> b16);
  let b8 = select(8u, 0u, bool((x & 255u)));
  x = (x >> b8);
  let b4 = select(4u, 0u, bool((x & 15u)));
  x = (x >> b4);
  let b2 = select(2u, 0u, bool((x & 3u)));
  x = (x >> b2);
  let b1 = select(1u, 0u, bool((x & 1u)));
  let is_zero = select(0u, 1u, (x == 0u));
  return i32((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : i32 = tint_count_trailing_zeros(15);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountTrailingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountTrailingZeros_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = countTrailingZeros(15u);
}
)";

    auto* expect = R"(
fn tint_count_trailing_zeros(v : u32) -> u32 {
  var x = u32(v);
  let b16 = select(16u, 0u, bool((x & 65535u)));
  x = (x >> b16);
  let b8 = select(8u, 0u, bool((x & 255u)));
  x = (x >> b8);
  let b4 = select(4u, 0u, bool((x & 15u)));
  x = (x >> b4);
  let b2 = select(2u, 0u, bool((x & 3u)));
  x = (x >> b2);
  let b1 = select(1u, 0u, bool((x & 1u)));
  let is_zero = select(0u, 1u, (x == 0u));
  return u32((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : u32 = tint_count_trailing_zeros(15u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountTrailingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountTrailingZeros_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = countTrailingZeros(vec3<i32>(15));
}
)";

    auto* expect = R"(
fn tint_count_trailing_zeros(v : vec3<i32>) -> vec3<i32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(16u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(65535u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(8u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(255u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(4u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(15u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(2u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(3u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(1u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(1u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(1u), (x == vec3<u32>(0u)));
  return vec3<i32>((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : vec3<i32> = tint_count_trailing_zeros(vec3<i32>(15));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountTrailingZeros());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, CountTrailingZeros_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = countTrailingZeros(vec3<u32>(15u));
}
)";

    auto* expect = R"(
fn tint_count_trailing_zeros(v : vec3<u32>) -> vec3<u32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(16u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(65535u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(8u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(255u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(4u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(15u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(2u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(3u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(1u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(1u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(1u), (x == vec3<u32>(0u)));
  return vec3<u32>((((((b16 | b8) | b4) | b2) | b1) + is_zero));
}

fn f() {
  let r : vec3<u32> = tint_count_trailing_zeros(vec3<u32>(15u));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillCountTrailingZeros());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// extractBits
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillExtractBits(Level level) {
    BuiltinPolyfill::Builtins builtins;
    builtins.extract_bits = level;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunExtractBits) {
    auto* src = R"(
fn f() {
  extractBits(1234, 5u, 6u);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src, polyfillExtractBits(Level::kNone)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillExtractBits(Level::kClampParameters)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillExtractBits(Level::kFull)));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Full_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = extractBits(1234, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : i32, offset : u32, count : u32) -> i32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let shl = (32u - e);
  let shr = (shl + s);
  return ((v << shl) >> shr);
}

fn f() {
  let r : i32 = tint_extract_bits(1234, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Full_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = extractBits(1234u, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : u32, offset : u32, count : u32) -> u32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let shl = (32u - e);
  let shr = (shl + s);
  return ((v << shl) >> shr);
}

fn f() {
  let r : u32 = tint_extract_bits(1234u, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Full_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = extractBits(vec3<i32>(1234), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : vec3<i32>, offset : u32, count : u32) -> vec3<i32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let shl = (32u - e);
  let shr = (shl + s);
  return ((v << vec3<u32>(shl)) >> vec3<u32>(shr));
}

fn f() {
  let r : vec3<i32> = tint_extract_bits(vec3<i32>(1234), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Full_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = extractBits(vec3<u32>(1234u), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : vec3<u32>, offset : u32, count : u32) -> vec3<u32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let shl = (32u - e);
  let shr = (shl + s);
  return ((v << vec3<u32>(shl)) >> vec3<u32>(shr));
}

fn f() {
  let r : vec3<u32> = tint_extract_bits(vec3<u32>(1234u), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Clamp_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = extractBits(1234, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : i32, offset : u32, count : u32) -> i32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return extractBits(v, s, (e - s));
}

fn f() {
  let r : i32 = tint_extract_bits(1234, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Clamp_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = extractBits(1234u, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : u32, offset : u32, count : u32) -> u32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return extractBits(v, s, (e - s));
}

fn f() {
  let r : u32 = tint_extract_bits(1234u, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Clamp_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = extractBits(vec3<i32>(1234), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : vec3<i32>, offset : u32, count : u32) -> vec3<i32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return extractBits(v, s, (e - s));
}

fn f() {
  let r : vec3<i32> = tint_extract_bits(vec3<i32>(1234), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, ExtractBits_Clamp_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = extractBits(vec3<u32>(1234u), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_extract_bits(v : vec3<u32>, offset : u32, count : u32) -> vec3<u32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return extractBits(v, s, (e - s));
}

fn f() {
  let r : vec3<u32> = tint_extract_bits(vec3<u32>(1234u), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillExtractBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// firstLeadingBit
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillFirstLeadingBit() {
    BuiltinPolyfill::Builtins builtins;
    builtins.first_leading_bit = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunFirstLeadingBit) {
    auto* src = R"(
fn f() {
  firstLeadingBit(0xf);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillFirstLeadingBit()));
}

TEST_F(BuiltinPolyfillTest, FirstLeadingBit_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = firstLeadingBit(15);
}
)";

    auto* expect = R"(
fn tint_first_leading_bit(v : i32) -> i32 {
  var x = select(u32(v), u32(~(v)), (v < 0i));
  let b16 = select(0u, 16u, bool((x & 4294901760u)));
  x = (x >> b16);
  let b8 = select(0u, 8u, bool((x & 65280u)));
  x = (x >> b8);
  let b4 = select(0u, 4u, bool((x & 240u)));
  x = (x >> b4);
  let b2 = select(0u, 2u, bool((x & 12u)));
  x = (x >> b2);
  let b1 = select(0u, 1u, bool((x & 2u)));
  let is_zero = select(0u, 4294967295u, (x == 0u));
  return i32((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : i32 = tint_first_leading_bit(15);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstLeadingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstLeadingBit_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = firstLeadingBit(15u);
}
)";

    auto* expect = R"(
fn tint_first_leading_bit(v : u32) -> u32 {
  var x = v;
  let b16 = select(0u, 16u, bool((x & 4294901760u)));
  x = (x >> b16);
  let b8 = select(0u, 8u, bool((x & 65280u)));
  x = (x >> b8);
  let b4 = select(0u, 4u, bool((x & 240u)));
  x = (x >> b4);
  let b2 = select(0u, 2u, bool((x & 12u)));
  x = (x >> b2);
  let b1 = select(0u, 1u, bool((x & 2u)));
  let is_zero = select(0u, 4294967295u, (x == 0u));
  return u32((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : u32 = tint_first_leading_bit(15u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstLeadingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstLeadingBit_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = firstLeadingBit(vec3<i32>(15));
}
)";

    auto* expect = R"(
fn tint_first_leading_bit(v : vec3<i32>) -> vec3<i32> {
  var x = select(vec3<u32>(v), vec3<u32>(~(v)), (v < vec3<i32>(0i)));
  let b16 = select(vec3<u32>(0u), vec3<u32>(16u), vec3<bool>((x & vec3<u32>(4294901760u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(0u), vec3<u32>(8u), vec3<bool>((x & vec3<u32>(65280u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(0u), vec3<u32>(4u), vec3<bool>((x & vec3<u32>(240u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(0u), vec3<u32>(2u), vec3<bool>((x & vec3<u32>(12u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(0u), vec3<u32>(1u), vec3<bool>((x & vec3<u32>(2u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(4294967295u), (x == vec3<u32>(0u)));
  return vec3<i32>((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : vec3<i32> = tint_first_leading_bit(vec3<i32>(15));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstLeadingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstLeadingBit_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = firstLeadingBit(vec3<u32>(15u));
}
)";

    auto* expect = R"(
fn tint_first_leading_bit(v : vec3<u32>) -> vec3<u32> {
  var x = v;
  let b16 = select(vec3<u32>(0u), vec3<u32>(16u), vec3<bool>((x & vec3<u32>(4294901760u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(0u), vec3<u32>(8u), vec3<bool>((x & vec3<u32>(65280u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(0u), vec3<u32>(4u), vec3<bool>((x & vec3<u32>(240u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(0u), vec3<u32>(2u), vec3<bool>((x & vec3<u32>(12u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(0u), vec3<u32>(1u), vec3<bool>((x & vec3<u32>(2u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(4294967295u), (x == vec3<u32>(0u)));
  return vec3<u32>((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : vec3<u32> = tint_first_leading_bit(vec3<u32>(15u));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstLeadingBit());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// firstTrailingBit
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillFirstTrailingBit() {
    BuiltinPolyfill::Builtins builtins;
    builtins.first_trailing_bit = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunFirstTrailingBit) {
    auto* src = R"(
fn f() {
  firstTrailingBit(0xf);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillFirstTrailingBit()));
}

TEST_F(BuiltinPolyfillTest, FirstTrailingBit_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = firstTrailingBit(15);
}
)";

    auto* expect = R"(
fn tint_first_trailing_bit(v : i32) -> i32 {
  var x = u32(v);
  let b16 = select(16u, 0u, bool((x & 65535u)));
  x = (x >> b16);
  let b8 = select(8u, 0u, bool((x & 255u)));
  x = (x >> b8);
  let b4 = select(4u, 0u, bool((x & 15u)));
  x = (x >> b4);
  let b2 = select(2u, 0u, bool((x & 3u)));
  x = (x >> b2);
  let b1 = select(1u, 0u, bool((x & 1u)));
  let is_zero = select(0u, 4294967295u, (x == 0u));
  return i32((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : i32 = tint_first_trailing_bit(15);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstTrailingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstTrailingBit_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = firstTrailingBit(15u);
}
)";

    auto* expect = R"(
fn tint_first_trailing_bit(v : u32) -> u32 {
  var x = u32(v);
  let b16 = select(16u, 0u, bool((x & 65535u)));
  x = (x >> b16);
  let b8 = select(8u, 0u, bool((x & 255u)));
  x = (x >> b8);
  let b4 = select(4u, 0u, bool((x & 15u)));
  x = (x >> b4);
  let b2 = select(2u, 0u, bool((x & 3u)));
  x = (x >> b2);
  let b1 = select(1u, 0u, bool((x & 1u)));
  let is_zero = select(0u, 4294967295u, (x == 0u));
  return u32((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : u32 = tint_first_trailing_bit(15u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstTrailingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstTrailingBit_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = firstTrailingBit(vec3<i32>(15));
}
)";

    auto* expect = R"(
fn tint_first_trailing_bit(v : vec3<i32>) -> vec3<i32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(16u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(65535u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(8u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(255u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(4u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(15u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(2u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(3u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(1u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(1u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(4294967295u), (x == vec3<u32>(0u)));
  return vec3<i32>((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : vec3<i32> = tint_first_trailing_bit(vec3<i32>(15));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstTrailingBit());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, FirstTrailingBit_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = firstTrailingBit(vec3<u32>(15u));
}
)";

    auto* expect = R"(
fn tint_first_trailing_bit(v : vec3<u32>) -> vec3<u32> {
  var x = vec3<u32>(v);
  let b16 = select(vec3<u32>(16u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(65535u))));
  x = (x >> b16);
  let b8 = select(vec3<u32>(8u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(255u))));
  x = (x >> b8);
  let b4 = select(vec3<u32>(4u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(15u))));
  x = (x >> b4);
  let b2 = select(vec3<u32>(2u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(3u))));
  x = (x >> b2);
  let b1 = select(vec3<u32>(1u), vec3<u32>(0u), vec3<bool>((x & vec3<u32>(1u))));
  let is_zero = select(vec3<u32>(0u), vec3<u32>(4294967295u), (x == vec3<u32>(0u)));
  return vec3<u32>((((((b16 | b8) | b4) | b2) | b1) | is_zero));
}

fn f() {
  let r : vec3<u32> = tint_first_trailing_bit(vec3<u32>(15u));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillFirstTrailingBit());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// insertBits
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillInsertBits(Level level) {
    BuiltinPolyfill::Builtins builtins;
    builtins.insert_bits = level;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunInsertBits) {
    auto* src = R"(
fn f() {
  insertBits(1234, 5678, 5u, 6u);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src, polyfillInsertBits(Level::kNone)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillInsertBits(Level::kClampParameters)));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillInsertBits(Level::kFull)));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Full_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = insertBits(1234, 5678, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : i32, n : i32, offset : u32, count : u32) -> i32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let mask = (((1u << s) - 1u) ^ ((1u << e) - 1u));
  return (((n << s) & i32(mask)) | (v & i32(~(mask))));
}

fn f() {
  let r : i32 = tint_insert_bits(1234, 5678, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Full_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = insertBits(1234u, 5678u, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : u32, n : u32, offset : u32, count : u32) -> u32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let mask = (((1u << s) - 1u) ^ ((1u << e) - 1u));
  return (((n << s) & mask) | (v & ~(mask)));
}

fn f() {
  let r : u32 = tint_insert_bits(1234u, 5678u, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Full_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = insertBits(vec3<i32>(1234), vec3<i32>(5678), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : vec3<i32>, n : vec3<i32>, offset : u32, count : u32) -> vec3<i32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let mask = (((1u << s) - 1u) ^ ((1u << e) - 1u));
  return (((n << vec3<u32>(s)) & vec3<i32>(i32(mask))) | (v & vec3<i32>(i32(~(mask)))));
}

fn f() {
  let r : vec3<i32> = tint_insert_bits(vec3<i32>(1234), vec3<i32>(5678), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Full_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = insertBits(vec3<u32>(1234u), vec3<u32>(5678u), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : vec3<u32>, n : vec3<u32>, offset : u32, count : u32) -> vec3<u32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  let mask = (((1u << s) - 1u) ^ ((1u << e) - 1u));
  return (((n << vec3<u32>(s)) & vec3<u32>(mask)) | (v & vec3<u32>(~(mask))));
}

fn f() {
  let r : vec3<u32> = tint_insert_bits(vec3<u32>(1234u), vec3<u32>(5678u), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kFull));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Clamp_i32) {
    auto* src = R"(
fn f() {
  let r : i32 = insertBits(1234, 5678, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : i32, n : i32, offset : u32, count : u32) -> i32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return insertBits(v, n, s, (e - s));
}

fn f() {
  let r : i32 = tint_insert_bits(1234, 5678, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Clamp_u32) {
    auto* src = R"(
fn f() {
  let r : u32 = insertBits(1234u, 5678u, 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : u32, n : u32, offset : u32, count : u32) -> u32 {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return insertBits(v, n, s, (e - s));
}

fn f() {
  let r : u32 = tint_insert_bits(1234u, 5678u, 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Clamp_vec3_i32) {
    auto* src = R"(
fn f() {
  let r : vec3<i32> = insertBits(vec3<i32>(1234), vec3<i32>(5678), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : vec3<i32>, n : vec3<i32>, offset : u32, count : u32) -> vec3<i32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return insertBits(v, n, s, (e - s));
}

fn f() {
  let r : vec3<i32> = tint_insert_bits(vec3<i32>(1234), vec3<i32>(5678), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, InsertBits_Clamp_vec3_u32) {
    auto* src = R"(
fn f() {
  let r : vec3<u32> = insertBits(vec3<u32>(1234u), vec3<u32>(5678u), 5u, 6u);
}
)";

    auto* expect = R"(
fn tint_insert_bits(v : vec3<u32>, n : vec3<u32>, offset : u32, count : u32) -> vec3<u32> {
  let s = min(offset, 32u);
  let e = min(32u, (s + count));
  return insertBits(v, n, s, (e - s));
}

fn f() {
  let r : vec3<u32> = tint_insert_bits(vec3<u32>(1234u), vec3<u32>(5678u), 5u, 6u);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillInsertBits(Level::kClampParameters));

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// saturate
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillSaturate() {
    BuiltinPolyfill::Builtins builtins;
    builtins.saturate = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunSaturate) {
    auto* src = R"(
fn f() {
  saturate(0.5);
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillSaturate()));
}

TEST_F(BuiltinPolyfillTest, Saturate_f32) {
    auto* src = R"(
fn f() {
  let r : f32 = saturate(0.5f);
}
)";

    auto* expect = R"(
fn tint_saturate(v : f32) -> f32 {
  return clamp(v, f32(0), f32(1));
}

fn f() {
  let r : f32 = tint_saturate(0.5f);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Saturate_f32_from_abstract_float) {
    auto* src = R"(
fn f() {
  let r : f32 = saturate(0.5);
}
)";

    auto* expect = R"(
fn tint_saturate(v : f32) -> f32 {
  return clamp(v, f32(0), f32(1));
}

fn f() {
  let r : f32 = tint_saturate(0.5);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Saturate_f16) {
    auto* src = R"(
enable f16;

fn f() {
  let r : f16 = saturate(0.5h);
}
)";

    auto* expect = R"(
enable f16;

fn tint_saturate(v : f16) -> f16 {
  return clamp(v, f16(0), f16(1));
}

fn f() {
  let r : f16 = tint_saturate(0.5h);
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Saturate_vec3_f32) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = saturate(vec3<f32>(0.5f));
}
)";

    auto* expect = R"(
fn tint_saturate(v : vec3<f32>) -> vec3<f32> {
  return clamp(v, vec3<f32>(0), vec3<f32>(1));
}

fn f() {
  let r : vec3<f32> = tint_saturate(vec3<f32>(0.5f));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Saturate_vec3_f32_from_abstract_float) {
    auto* src = R"(
fn f() {
  let r : vec3<f32> = saturate(vec3(0.5));
}
)";

    auto* expect = R"(
fn tint_saturate(v : vec3<f32>) -> vec3<f32> {
  return clamp(v, vec3<f32>(0), vec3<f32>(1));
}

fn f() {
  let r : vec3<f32> = tint_saturate(vec3(0.5));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

TEST_F(BuiltinPolyfillTest, Saturate_vec3_f16) {
    auto* src = R"(
enable f16;

fn f() {
  let r : vec3<f16> = saturate(vec3<f16>(0.5h));
}
)";

    auto* expect = R"(
enable f16;

fn tint_saturate(v : vec3<f16>) -> vec3<f16> {
  return clamp(v, vec3<f16>(0), vec3<f16>(1));
}

fn f() {
  let r : vec3<f16> = tint_saturate(vec3<f16>(0.5h));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillSaturate());

    EXPECT_EQ(expect, str(got));
}

////////////////////////////////////////////////////////////////////////////////
// textureSampleBaseClampToEdge
////////////////////////////////////////////////////////////////////////////////
DataMap polyfillTextureSampleBaseClampToEdge_2d_f32() {
    BuiltinPolyfill::Builtins builtins;
    builtins.texture_sample_base_clamp_to_edge_2d_f32 = true;
    DataMap data;
    data.Add<BuiltinPolyfill::Config>(builtins);
    return data;
}

TEST_F(BuiltinPolyfillTest, ShouldRunTextureSampleBaseClampToEdge_2d_f32) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;
@group(0) @binding(1) var s : sampler;

fn f() {
  textureSampleBaseClampToEdge(t, s, vec2<f32>(0.5));
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_TRUE(ShouldRun<BuiltinPolyfill>(src, polyfillTextureSampleBaseClampToEdge_2d_f32()));
}

TEST_F(BuiltinPolyfillTest, ShouldRunTextureSampleBaseClampToEdge_external) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_external;
@group(0) @binding(1) var s : sampler;

fn f() {
  textureSampleBaseClampToEdge(t, s, vec2<f32>(0.5));
}
)";

    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src));
    EXPECT_FALSE(ShouldRun<BuiltinPolyfill>(src, polyfillTextureSampleBaseClampToEdge_2d_f32()));
}

TEST_F(BuiltinPolyfillTest, TextureSampleBaseClampToEdge_2d_f32_f32) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;
@group(0) @binding(1) var s : sampler;

fn f() {
  let r = textureSampleBaseClampToEdge(t, s, vec2<f32>(0.5));
}
)";

    auto* expect = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn tint_textureSampleBaseClampToEdge(t : texture_2d<f32>, s : sampler, coord : vec2<f32>) -> vec4<f32> {
  let dims = vec2<f32>(textureDimensions(t, 0));
  let half_texel = (vec2<f32>(0.5) / dims);
  let clamped = clamp(coord, half_texel, (1 - half_texel));
  return textureSampleLevel(t, s, clamped, 0);
}

fn f() {
  let r = tint_textureSampleBaseClampToEdge(t, s, vec2<f32>(0.5));
}
)";

    auto got = Run<BuiltinPolyfill>(src, polyfillTextureSampleBaseClampToEdge_2d_f32());

    EXPECT_EQ(expect, str(got));
}

}  // namespace
}  // namespace tint::transform
