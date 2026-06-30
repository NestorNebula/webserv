#include "FakeResource.hpp"
#include "Response.hpp"
#include <gtest/gtest.h>

#define BUFSIZE 4096 // DO NOT DECREASE

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

/*
TEST(Response, Basic200) {
	FakeResource resource("");
	Response response(resource);

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"\r\n");
	EXPECT_FALSE(response.hasBody());
}
*/

TEST(Response, Basic200WithBody) {
	FakeResource resource;
	Response response(resource);
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"\r\n");
	EXPECT_TRUE(response.hasBody());
	EXPECT_EQ(response.readBody(buf, BUFSIZE), static_cast<long>(std::string("Content").size()));
	EXPECT_STREQ(buf, "Content");
}

TEST(Response, Basic404WithBody) {
	FakeResource resource;
	Response response(resource);
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(404);
	response.setReason("Not Found");
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 404 Not Found\r\n"
			"\r\n");
	EXPECT_TRUE(response.hasBody());
	EXPECT_EQ(response.readBody(buf, BUFSIZE), static_cast<long>(std::string("Content").size()));
	EXPECT_STREQ(buf, "Content");
}

TEST(Response, OneHeaderWithBody) {
	FakeResource resource;
	Response response(resource);
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	response.addHeader(Header("Content-Type", "text/html"));
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"\r\n");
	EXPECT_TRUE(response.hasBody());
	EXPECT_EQ(response.readBody(buf, BUFSIZE), static_cast<long>(std::string("Content").size()));
	EXPECT_STREQ(buf, "Content");
}

TEST(Response, MultipleHeadersWithBody) {
	FakeResource resource;
	Response response(resource);
	Headers headers;
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	headers.insert("Content-Type", "text/html");
	headers.insert("Connection", "close");
	headers.insert("Server", "webserv");
	response.addHeaders(headers.begin(), headers.end());
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"Connection: close\r\n"
			"Content-Type: text/html\r\n"
			"Server: webserv\r\n"
			"\r\n");
	EXPECT_TRUE(response.hasBody());
	EXPECT_EQ(response.readBody(buf, BUFSIZE), static_cast<long>(std::string("Content").size()));
	EXPECT_STREQ(buf, "Content");
}

TEST(Response, MultilineBody) {
	FakeResource resource("Hello\nThere!\n");
	Response response(resource);
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"\r\n");
	EXPECT_EQ(response.readBody(buf, BUFSIZE), static_cast<long>(std::string("Content").size()));
	EXPECT_STREQ(buf, "Hello\nThere!\n");
	EXPECT_TRUE(response.hasBody());
}

TEST(Response, AccessNonReadyResponse) {
	FakeResource resource;
	Response response(resource);

	EXPECT_FALSE(response.isReady());
	EXPECT_THROW(response.getHead(), std::logic_error);
}

/*
TEST(Response, ReadEmptyBody) {
	FakeResource resource("");
	Response response(resource);
	char buf[BUFSIZE];

	response.setVersion("HTTP/1.1");
	response.setCode(200);
	response.setReason("OK");
	EXPECT_TRUE(response.isReady());
	EXPECT_EQ(response.getHead(), "HTTP/1.1 200 OK\r\n"
			"\r\n");
	EXPECT_FALSE(response.hasBody());
	EXPECT_THROW(response.readBody(buf, BUFSIZE), std::logic_error);
}
*/
