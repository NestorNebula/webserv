#pragma once

#include "Resource.hpp"
#include "gmock/gmock.h"

class FakeResource: public Resource {
public:
  FakeResource(const std::string &content = "Content") {
	  _stream.adoptStream(new std::stringstream());
	  _stream.write(content.c_str(), content.size());
  }
  virtual ~FakeResource() {}
  MOCK_METHOD0(generate, void());
  MOCK_CONST_METHOD0(done, bool());
  MOCK_CONST_METHOD0(inProgress, bool());
  MOCK_CONST_METHOD0(failed, bool());
  virtual Stream &stream() { return _stream; }
  
private:
  Stream _stream;
};
