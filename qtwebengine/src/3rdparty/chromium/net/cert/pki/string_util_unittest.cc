// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/pki/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

TEST(StringUtilTest, IsAscii) {
  EXPECT_TRUE(net::string_util::IsAscii(""));
  EXPECT_TRUE(net::string_util::IsAscii("mail.google.com"));
  EXPECT_TRUE(net::string_util::IsAscii("mail.google.com\x7F"));
  EXPECT_FALSE(net::string_util::IsAscii("mail.google.com\x80"));
  EXPECT_FALSE(net::string_util::IsAscii("mail.google.com\xFF"));
}

TEST(StringUtilTest, IsEqualNoCase) {
  EXPECT_TRUE(net::string_util::IsEqualNoCase("", ""));
  EXPECT_TRUE(
      net::string_util::IsEqualNoCase("mail.google.com", "maIL.GOoGlE.cOm"));
  EXPECT_TRUE(net::string_util::IsEqualNoCase("MAil~-.google.cOm",
                                              "maIL~-.gOoGlE.CoM"));
  EXPECT_TRUE(net::string_util::IsEqualNoCase("mail\x80.google.com",
                                              "maIL\x80.GOoGlE.cOm"));
  EXPECT_TRUE(net::string_util::IsEqualNoCase("mail\xFF.google.com",
                                              "maIL\xFF.GOoGlE.cOm"));
  EXPECT_FALSE(
      net::string_util::IsEqualNoCase("mail.google.co", "maIL.GOoGlE.cOm"));
  EXPECT_FALSE(
      net::string_util::IsEqualNoCase("mail.google.com", "maIL.GOoGlE.cO"));
}

TEST(StringUtilTest, EndsWithNoCase) {
  EXPECT_TRUE(net::string_util::EndsWithNoCase("", ""));
  EXPECT_TRUE(net::string_util::EndsWithNoCase("mail.google.com", ""));
  EXPECT_TRUE(
      net::string_util::EndsWithNoCase("mail.google.com", "maIL.GOoGlE.cOm"));
  EXPECT_TRUE(
      net::string_util::EndsWithNoCase("mail.google.com", ".gOoGlE.cOm"));
  EXPECT_TRUE(
      net::string_util::EndsWithNoCase("MAil~-.google.cOm", "-.gOoGlE.CoM"));
  EXPECT_TRUE(net::string_util::EndsWithNoCase("mail\x80.google.com",
                                               "\x80.GOoGlE.cOm"));
  EXPECT_FALSE(
      net::string_util::EndsWithNoCase("mail.google.com", "pOoGlE.com"));
  EXPECT_FALSE(net::string_util::EndsWithNoCase("mail\x80.google.com",
                                                "\x81.GOoGlE.cOm"));
  EXPECT_FALSE(
      net::string_util::EndsWithNoCase("mail.google.co", ".GOoGlE.cOm"));
  EXPECT_FALSE(
      net::string_util::EndsWithNoCase("mail.google.com", ".GOoGlE.cO"));
  EXPECT_FALSE(
      net::string_util::EndsWithNoCase("mail.google.com", "mail.google.com1"));
  EXPECT_FALSE(
      net::string_util::EndsWithNoCase("mail.google.com", "1mail.google.com"));
}

TEST(StringUtilTest, FindAndReplace) {
  std::string tester = "hoobla derp hoobla derp porkrind";
  tester = net::string_util::FindAndReplace(tester, "blah", "woof");
  EXPECT_EQ(tester, "hoobla derp hoobla derp porkrind");
  tester = net::string_util::FindAndReplace(tester, "", "yeet");
  EXPECT_EQ(tester, "hoobla derp hoobla derp porkrind");
  tester = net::string_util::FindAndReplace(tester, "hoobla", "derp");
  EXPECT_EQ(tester, "derp derp derp derp porkrind");
  tester = net::string_util::FindAndReplace(tester, "derp", "a");
  EXPECT_EQ(tester, "a a a a porkrind");
  tester = net::string_util::FindAndReplace(tester, "a ", "");
  EXPECT_EQ(tester, "porkrind");
  tester = net::string_util::FindAndReplace(tester, "porkrind", "");
  EXPECT_EQ(tester, "");
}

TEST(StringUtilTest, StartsWithNoCase) {
  EXPECT_TRUE(net::string_util::StartsWithNoCase("", ""));
  EXPECT_TRUE(net::string_util::StartsWithNoCase("mail.google.com", ""));
  EXPECT_TRUE(
      net::string_util::StartsWithNoCase("mail.google.com", "maIL.GOoGlE.cOm"));
  EXPECT_TRUE(net::string_util::StartsWithNoCase("mail.google.com", "MaIL."));
  EXPECT_TRUE(
      net::string_util::StartsWithNoCase("MAil~-.google.cOm", "maiL~-.Goo"));
  EXPECT_TRUE(
      net::string_util::StartsWithNoCase("mail\x80.google.com", "MAIL\x80."));
  EXPECT_FALSE(
      net::string_util::StartsWithNoCase("mail.google.com", "maIl.MoO"));
  EXPECT_FALSE(
      net::string_util::StartsWithNoCase("mail\x80.google.com", "Mail\x81"));
  EXPECT_FALSE(
      net::string_util::StartsWithNoCase("mai.google.co", "MAiL.GoogLE"));
  EXPECT_FALSE(
      net::string_util::StartsWithNoCase("mail.google.com", "MaI.GooGLE"));
  EXPECT_FALSE(net::string_util::StartsWithNoCase("mail.google.com",
                                                  "mail.google.com1"));
  EXPECT_FALSE(net::string_util::StartsWithNoCase("mail.google.com",
                                                  "1mail.google.com"));
}

}  // namespace

}  // namespace net
