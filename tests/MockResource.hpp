#include "Resource.hpp"
#include <gmock/gmock.h>

class MockResource : public Resource {
public:
	MockResource();
	MockResource(const MockResource &);
	MockResource &operator=(const MockResource &);
	virtual ~MockResource();
        MOCK_CONST_METHOD0(getContent, std::string &());

      private:
};
