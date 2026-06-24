#include "Resource.hpp"

class FakeResource: public Resource {
public:
  FakeResource(std::string content = "Content") {
	  _content = content;
  }
  virtual ~FakeResource() {}
  virtual const std::string &getContent() const { return _content; }
  
private:
};
