#include "DirectoryResource.hpp"
#include <sys/types.h>
#include <gtest/gtest.h>
#include <dirent.h>

static void getDirectoryContent(const std::string &dirpath, std::set<std::string> &set) {
	DIR *dir = opendir(dirpath.c_str());

	struct dirent *dirFile;
	while ((dirFile = readdir(dir)) != NULL)
		set.insert(dirFile->d_name);
	closedir(dir);
}

static void checkEach(const std::string &content, std::set<std::string> &set) {
	for (std::set<std::string>::const_iterator it = set.begin(), ite = set.end(); it != ite; it++) {
		if (*it != "." && *it != "..") {
			EXPECT_TRUE(content.find(*it) != std::string::npos);
		}
	}
}

TEST(DirectoryResource, DefaultValues) {
	DirectoryResource resource("files");

	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
}

TEST(DirectoryResource, InvalidDirpath) {
	DirectoryResource resource("files/nosuchdir");

	resource.generate();
	EXPECT_FALSE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_TRUE(resource.failed());
}

TEST(DirectoryResource, ClassicDirectory) {
	DirectoryResource resource("files");
	std::set<std::string> files;
	getDirectoryContent("files", files);

	resource.generate();
	std::stringstream oss;
	oss << resource.stream().rdbuf();
	EXPECT_TRUE(resource.done());
	EXPECT_FALSE(resource.inProgress());
	EXPECT_FALSE(resource.failed());
	checkEach(oss.str(), files);
}

TEST(DirectoryResource, AsInterfacePtr) {
	Resource *resource = new DirectoryResource("files");
	std::set<std::string> files;
	getDirectoryContent("files", files);

	resource->generate();
	std::stringstream oss;
	oss << resource->stream().rdbuf();
	EXPECT_TRUE(resource->done());
	EXPECT_FALSE(resource->inProgress());
	EXPECT_FALSE(resource->failed());
	checkEach(oss.str(), files);
}

TEST(DirectoryResource, MultipleGenerate) {
	DirectoryResource resource("files");

	resource.generate();
	EXPECT_THROW(resource.generate(), std::logic_error);
}

TEST(DirectoryResource, AccessBeforeGenerate) {
	DirectoryResource resource("files");

	EXPECT_THROW(resource.stream(), std::logic_error);
}

TEST(DirectoryResource, AccessAfterFail) {
	DirectoryResource resource("files/nosuchdir");

	resource.generate();
	EXPECT_THROW(resource.stream(), std::logic_error);
}
