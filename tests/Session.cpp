#include "Session.hpp"
#include <gtest/gtest.h>

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

static void setConfig(ServerConfig &config) {
	RouteConfig route;

	route.autoindex = true;
	route.methods.insert(METHOD_GET);
	route.path = "/";
	route.index.push_back("./");
	config.routes.push_back(route);
}

static void writeSimpleRequest(Session &session) {
	std::string data("GET / HTTP/1.1 \r\n"
             "Host: localhost\r\n"
             "\r\n");

	EXPECT_EQ(session.write(data.c_str(), data.size()), data.size());
	EXPECT_EQ(session.nextAction(), Session::WRSOCK);
}

TEST(Session, StartAction) {
	ServerConfig config;
	setConfig(config);
	Session session(config);

	EXPECT_EQ(session.nextAction(), Session::RDSOCK);
}

TEST(Session, WriteData) {
	ServerConfig config;
	setConfig(config);
	Session session(config);
	std::string data("GET /index.html HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "User-Agent: Firefox\r\n"
             "Accept: text/html\r\n"
             "Connection: close\r\n"
             "\r\n");

	EXPECT_EQ(session.write(data.c_str(), data.size()), data.size());
	EXPECT_EQ(session.nextAction(), Session::WRSOCK);
}

TEST(Session, WriteUntilEndOfRequest) {
	ServerConfig config;
	setConfig(config);
	Session session(config);
	std::vector<std::string> data;
	data.push_back("GET /index.html HTTP/1.1\r\n");
    data.push_back("Host: localhost\r\n");
    data.push_back("User-Agent: Firefox\r\n");
    data.push_back("Accept: text/html\r\n");
    data.push_back("Connection: close\r\n");
    data.push_back("\r\n");

	for (std::vector<std::string>::const_iterator it = data.begin(), ite = data.end(); it != ite; it++) {
		EXPECT_EQ(session.nextAction(), Session::RDSOCK);
		EXPECT_EQ(session.write(it->c_str(), it->size()), it->size());
	}
	EXPECT_EQ(session.nextAction(), Session::WRSOCK);
}

TEST(Session, ReadData) {
	ServerConfig config;
	setConfig(config);
	Session session(config);
	char buf[BUFSIZE];

	writeSimpleRequest(session);
	Stream::streamsize size = session.read(buf, BUFSIZE);
	EXPECT_LT(size, BUFSIZE);
	EXPECT_GT(size, 0);
	EXPECT_EQ(size, std::string(buf).size());
	EXPECT_EQ(session.nextAction(), Session::KPALIVE);
}

TEST(Session, ReadUntilEndOfResponse) {
	ServerConfig config;
	setConfig(config);
	Session s1(config);
	char buf[BUFSIZE];

	writeSimpleRequest(s1);
	Stream::streamsize size = s1.read(buf, BUFSIZE);
	EXPECT_LT(size, BUFSIZE);
	EXPECT_GT(size, 0);
	EXPECT_EQ(size, std::string(buf).size());
	EXPECT_EQ(s1.nextAction(), Session::KPALIVE);

	Session s2(config);
	writeSimpleRequest(s2);
	Stream ::streamsize size2 = 0;
	while (s2.nextAction() == Session::WRSOCK)
		size2 += s2.read(buf, 5);
	EXPECT_EQ(size, size2);
	EXPECT_EQ(s2.nextAction(), Session::KPALIVE);
}

TEST(Session, ResetSession) {
	ServerConfig config;
	setConfig(config);
	Session session(config);
	char buf[BUFSIZE];

	writeSimpleRequest(session);
	session.read(buf, BUFSIZE);
	EXPECT_EQ(session.nextAction(), Session::KPALIVE);
	session.reset();
	EXPECT_EQ(session.nextAction(), Session::RDSOCK);
}

TEST(Session, WrongCalls) {
	ServerConfig config;
	setConfig(config);
	Session session(config);
	char buf[BUFSIZE];

	EXPECT_EQ(session.nextAction(), Session::RDSOCK);
	EXPECT_THROW(session.read(buf, BUFSIZE), std::logic_error);
	EXPECT_THROW(session.reset(), std::logic_error);

	writeSimpleRequest(session);
	EXPECT_THROW(session.write("Hello world!\n", 13), std::logic_error);
	EXPECT_THROW(session.reset(), std::logic_error);

	session.read(buf, BUFSIZE);
	EXPECT_EQ(session.nextAction(), Session::KPALIVE);
	EXPECT_THROW(session.write("Hello world!\n", 13), std::logic_error);
	EXPECT_THROW(session.read(buf, BUFSIZE), std::logic_error);
	session.reset();
	EXPECT_THROW(session.reset(), std::logic_error);
}
