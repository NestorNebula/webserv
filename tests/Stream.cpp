#include "Stream.hpp"
#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

void testStream(Stream *stream) {
  EXPECT_TRUE(stream);
  EXPECT_EQ(stream->size(), 0);
}

void testWriteInStream(Stream *stream) {
  std::string str("Hello world!");

  stream->write(str.c_str(), str.size());
  EXPECT_EQ(static_cast<unsigned long>(stream->size()), str.size());
}

void testWriteThenReadInStream(Stream *stream) {
  std::string str("Hello world!");
  char buf[20];

  stream->write(str.c_str(), str.size());
  stream->read(buf, str.size());
  EXPECT_EQ(std::string(buf, str.size()), str);
}

void testShiftOperatorsStream(Stream *stream) {
  std::string str("Hello_world!");
  char buf[20];

  *stream << str;
  *stream >> buf;
  EXPECT_EQ(std::string(buf, str.size()), str);
}

void testWriteReadAlternate(Stream *stream) {
  std::string str;
  char buf[20];

  str = "Hello";
  stream->write(str.c_str(), str.size());
  stream->read(buf, str.size() / 2);
  EXPECT_EQ(std::string(buf, str.size() / 2), str.substr(0, str.size() / 2));
  stream->read(buf, str.size() - str.size() / 2);
  EXPECT_EQ(std::string(buf, str.size() - str.size() / 2), str.substr(str.size() / 2));
  str = " World!";
  stream->write(str.c_str(), str.size() / 2);
  stream->write(str.c_str() + str.size() / 2, str.size() - str.size() / 2);
  stream->read(buf, str.size());
  EXPECT_EQ(std::string(buf, str.size()), str);
}

TEST(Stream, StringStream) {
  Stream ss(new std::stringstream);
  testStream(&ss);
  ss.adoptStream(new std::stringstream);
  testWriteInStream(&ss);
  ss.adoptStream(new std::stringstream);
  testWriteThenReadInStream(&ss);
  ss.adoptStream(new std::stringstream);
  testShiftOperatorsStream(&ss);
  ss.adoptStream(new std::stringstream);
  testWriteReadAlternate(&ss);
}

TEST(Stream, FileStream) {
  Stream fs(new std::fstream(".a", std::ios::in | std::ios::out | std::ios::trunc));
  testStream(&fs);
  fs.adoptStream(new std::fstream(".b", std::ios::in | std::ios::out | std::ios::trunc));
  testWriteInStream(&fs);
  fs.adoptStream(new std::fstream(".c", std::ios::in | std::ios::out | std::ios::trunc));
  testWriteThenReadInStream(&fs);
  fs.adoptStream(new std::fstream(".d", std::ios::in | std::ios::out | std::ios::trunc));
  testShiftOperatorsStream(&fs);
  fs.adoptStream(new std::fstream(".e", std::ios::in | std::ios::out | std::ios::trunc));
  testWriteReadAlternate(&fs);
  std::remove(".a");
  std::remove(".b");
  std::remove(".c");
  std::remove(".d");
  std::remove(".e");
}
