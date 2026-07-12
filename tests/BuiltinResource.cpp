#include "BuiltinResource.hpp"
#include <gtest/gtest.h>

#define DEFAULT_CODE 301

TEST(BuiltinResource, DefaultValues) {
	BuiltinResource resource(DEFAULT_CODE);

	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
}

TEST(BuiltinResource, InvalidCode) {
	BuiltinResource resource(1000);

	resource.generate();
	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_TRUE(resource.failed());
}

TEST(BuiltinResource, UnhandledCode) {
	BuiltinResource resource(400);

	resource.generate();
	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_TRUE(resource.failed());
}

TEST(BuiltinResource, BasicCode) {
	BuiltinResource resource(DEFAULT_CODE);

	resource.generate();
	EXPECT_TRUE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
	EXPECT_NO_THROW(resource.stream());
}

TEST(BuiltinResource, AsInterfacePtr) {
	Resource *resource = new BuiltinResource(DEFAULT_CODE);

	resource->generate();
	EXPECT_TRUE(resource->done());
	EXPECT_FALSE(resource->inProgress());
	EXPECT_FALSE(resource->failed());
	EXPECT_NO_THROW(resource->stream());
}

TEST(BuiltinResource, MultipleGenerate) {
	BuiltinResource resource(DEFAULT_CODE);

	resource.generate();
	EXPECT_THROW(resource.generate(), std::logic_error);
}

TEST(BuiltinResource, AccessBeforeGenerate) {
	BuiltinResource resource(DEFAULT_CODE);

	EXPECT_THROW(resource.stream(), std::logic_error);
}

TEST(BuiltinResource, AccessAfterFail) {
	BuiltinResource resource(400);

	resource.generate();
	EXPECT_THROW(resource.stream(), std::logic_error);
}
