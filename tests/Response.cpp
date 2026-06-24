#include "FakeResource.hpp"
#include "Response.hpp"
#include <gtest/gtest.h>

TEST(Response, EmptyResponse) {
	FakeResource resource;
	Response response(resource);

	EXPECT_FALSE(response.isReady());
}

TEST(Response, MissingVersion) {
	FakeResource resource;
	Response response(resource);

	response.setCode(200);
	response.setReason("OK");
	EXPECT_FALSE(response.isReady());
}

TEST(Response, MissingCode) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setReason("OK");
	EXPECT_FALSE(response.isReady());
}

TEST(Response, MissingReason) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	EXPECT_FALSE(response.isReady());
}

TEST(Response, Generate200) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"\r\n"
			"Content");
}

TEST(Response, Generate404) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(404);
	response.setReason("Not Found");
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 404 Not Found\r\n"
			"\r\n"
			"Content");
}

TEST(Response, GenerateOneHeader) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	response.addHeader(Header("Content-Type", "text/html"));
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"\r\n"
			"Content");
}

TEST(Response, GenerateMultipleHeaders) {
	FakeResource resource;
	Response response(resource);
	Headers headers;

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	headers.insert("Content-Type", "text/html");
	headers.insert("Connection", "close");
	headers.insert("Server", "webserv");
	response.addHeaders(headers.begin(), headers.end());
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"Content-Type: text/html\r\n"
			"Server: webserv\r\n"
			"\r\n"
			"Content");
}

TEST(Response, GenerateEmptyBody) {
	FakeResource resource("");
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"\r\n");
}

TEST(Response, GenerateMultilineBody) {
	FakeResource resource("Hello\nThere!\n");
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"\r\n"
			"Hello\n"
			"There!\n");
}

TEST(Response, ActionAfterGenerate) {
	FakeResource resource;
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	response.generate();
	EXPECT_EQ(response.getRaw(), "HTTP/1.1 200 OK\r\n"
			"\r\n"
			"Content");
	EXPECT_FALSE(response.isReady());
	EXPECT_THROW(response.setVersion("HTTP/1.0"), std::logic_error);
	EXPECT_THROW(response.setCode(200), std::logic_error);
	EXPECT_THROW(response.setReason("OK"), std::logic_error);
	EXPECT_THROW(response.generate(), std::logic_error);
}
