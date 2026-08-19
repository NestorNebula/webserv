#include "TemporaryFileStream.hpp"
#include <gtest/gtest.h>

TEST(TemporaryFileStream, DefaultTemporaryFile) {
  TemporaryFileStream stream;

  EXPECT_TRUE(stream);
  EXPECT_EQ(stream.size(), 0);
}

TEST(TemporaryFileStream, WriteInStream) {
  TemporaryFileStream stream;
  std::string str("Hello world!");

  stream.write(str.c_str(), str.size());
  EXPECT_EQ(static_cast<unsigned long>(stream.size()), str.size());
}

TEST(TemporaryFileStream, WriteThenReadInStream) {
  TemporaryFileStream stream;
  std::string str("Hello world!");
  char buf[20];

  stream.write(str.c_str(), str.size());
  stream.read(buf, str.size());
  EXPECT_EQ(std::string(buf, str.size()), str);
}

TEST(TemporaryFileStream, WriteThenReadWithShiftOperatorsInStream) {
  TemporaryFileStream stream;
  std::string str("Hello_world!");
  char buf[20];

  stream << str;
  stream >> buf;
  EXPECT_EQ(std::string(buf, str.size()), str);
}

TEST(TemporaryFileStream, AlternateWriteAndRead) {
  TemporaryFileStream stream;
  std::string str;
  char buf[20];

  str = "Hello";
  stream.write(str.c_str(), str.size());
  stream.read(buf, str.size() / 2);
  EXPECT_EQ(std::string(buf, str.size() / 2), str.substr(0, str.size() / 2));
  stream.read(buf, str.size() - str.size() / 2);
  EXPECT_EQ(std::string(buf, str.size() - str.size() / 2), str.substr(str.size() / 2));
  str = " World!";
  stream.write(str.c_str(), str.size() / 2);
  stream.write(str.c_str() + str.size() / 2, str.size() - str.size() / 2);
  stream.read(buf, str.size());
  EXPECT_EQ(std::string(buf, str.size()), str);
}
