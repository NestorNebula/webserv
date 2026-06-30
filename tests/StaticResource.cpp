#include "StaticResource.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(StaticResource, DefaultValues) {
	StaticResource resource("files/small.txt");

	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
}

TEST(StaticResource, InvalidFilepath) {
	StaticResource resource("files/nosuchpath.txt");

	resource.generate();
	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_TRUE(resource.failed());
}

TEST(StaticResource, BasicFile) {
	StaticResource resource("files/small.txt");
	std::fstream fstream("files/small.txt");
	std::ostringstream oss1, oss2;
	oss1 << fstream.rdbuf();

	resource.generate();
	oss2 << resource.stream().rdbuf();

	EXPECT_TRUE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
	EXPECT_EQ(oss1.str(), oss2.str());
}

TEST(StaticResource, AsInterfacePtr) {
	Resource *resource = new StaticResource("files/small.txt");
	std::fstream fstream("files/small.txt");
	std::ostringstream oss1, oss2;
	oss1 << fstream.rdbuf();

	resource->generate();
	oss2 << resource->stream().rdbuf();

	EXPECT_TRUE(resource->done());
	EXPECT_FALSE(resource->inProgress());
	EXPECT_FALSE(resource->failed());
	EXPECT_EQ(oss1.str(), oss2.str());
	delete resource;
}

TEST(StaticResource, EmptyFile) {
	StaticResource resource("files/empty.txt");
	std::fstream fstream("files/empty.txt");
	std::ostringstream oss1, oss2;
	oss1 << fstream.rdbuf();

	resource.generate();
	oss2 << resource.stream().rdbuf();

	EXPECT_TRUE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
	EXPECT_EQ(oss1.str(), oss2.str());
}

TEST(StaticResource, LargeFile) {
	StaticResource resource("files/large.txt");
	std::fstream fstream("files/large.txt");
	std::ostringstream oss1, oss2;
	oss1 << fstream.rdbuf();

	resource.generate();
	oss2 << resource.stream().rdbuf();

	EXPECT_TRUE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
	EXPECT_EQ(oss1.str(), oss2.str());
}

TEST(StaticResource, MultipleGenerate) {
	StaticResource resource("files/small.txt");

	resource.generate();
	EXPECT_THROW(resource.generate(), std::logic_error);
}

TEST(StaticResource, AccessBeforeGenerate) {
	StaticResource resource("files/small.txt");
	EXPECT_THROW(resource.stream(), std::logic_error);
}

TEST(StaticResource, AccessAfterFail) {
	StaticResource resource("files/nosuchfile.txt");

	resource.generate();
	EXPECT_THROW(resource.stream(), std::logic_error);
}
