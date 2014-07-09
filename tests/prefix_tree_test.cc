#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "common/prefix_tree.h"
#include "test_util.h"

using std::string;
using std::vector;

namespace sicxe {
namespace tests {

TEST(PrefixTreeTest, Insert) {
  PrefixTree tree;
  EXPECT_FALSE(tree.Insert(""));
  EXPECT_TRUE(tree.Insert("abc"));
  EXPECT_FALSE(tree.Insert("abc"));
  EXPECT_TRUE(tree.Insert("abcd"));
  EXPECT_TRUE(tree.Insert("abcde"));
  EXPECT_TRUE(tree.Insert("abd"));
  EXPECT_TRUE(tree.Insert("abef"));
  EXPECT_FALSE(tree.Insert("abcd"));
}

TEST(PrefixTreeTest, Find) {
  PrefixTree tree;
  vector<string> r1, r2, r3, r4, r5, r6, r7, r8;

  EXPECT_TRUE(tree.Insert("aabc"));
  EXPECT_TRUE(tree.Insert("aabd"));
  EXPECT_TRUE(tree.Insert("aacde"));
  EXPECT_TRUE(tree.Insert("aacefg"));

  tree.Find("a", &r1);
  EXPECT_EQ("aabc|aabd|aacde|aacefg", TestUtil::ConcatStrVector(r1, "|"));
  tree.Find("aab", &r2);
  EXPECT_EQ("aabc|aabd", TestUtil::ConcatStrVector(r2, "|"));
  tree.Find("aac", &r3);
  EXPECT_EQ("aacde|aacefg", TestUtil::ConcatStrVector(r3, "|"));
  tree.Find("aace", &r4);
  EXPECT_EQ("aacefg", TestUtil::ConcatStrVector(r4, "|"));
  tree.Find("aabcd", &r5);
  EXPECT_EQ("", TestUtil::ConcatStrVector(r5, "|"));
  tree.Find("", &r6);
  EXPECT_EQ("", TestUtil::ConcatStrVector(r6, "|"));

  EXPECT_TRUE(tree.Insert("aa"));
  tree.Find("a", &r7);
  EXPECT_EQ("aa|aabc|aabd|aacde|aacefg", TestUtil::ConcatStrVector(r7, "|"));
  tree.Find("aa", &r8);
  EXPECT_EQ("aa", TestUtil::ConcatStrVector(r8, "|"));  // exact match only
}

}  // namespace tests
}  // namespace sicxe
