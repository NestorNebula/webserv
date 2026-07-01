#include "Session.hpp"
#include <gtest/gtest.h>

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

static void writeSimpleRequest(Session &session) {
	std::string data("GET / HTTP/1.1 \r\n"
             "Host: localhost\r\n"
             "\r\n");

	EXPECT_EQ(session.write(data.c_str(), data.size()), data.size());
	EXPECT_EQ(session.nextAction(), Session::WRSOCK);
}

TEST(Session, StartAction) {
	Session session;

	EXPECT_EQ(session.nextAction(), Session::RDSOCK);
}

TEST(Session, WriteData) {
	Session session;
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
	Session session;
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
	Session session;
	char buf[BUFSIZ];

	writeSimpleRequest(session);
	Stream::streamsize size = session.read(buf, BUFSIZ);
	EXPECT_LT(size, BUFSIZ);
	EXPECT_GT(size, 0);
	EXPECT_EQ(size, std::string(buf).size());
	EXPECT_EQ(session.nextAction(), Session::CLOSE);
}

TEST(Session, ReadUntilEndOfResponse) {
	Session s1;
	char buf[BUFSIZ];

	writeSimpleRequest(s1);
	Stream::streamsize size = s1.read(buf, BUFSIZ);
	EXPECT_LT(size, BUFSIZ);
	EXPECT_GT(size, 0);
	EXPECT_EQ(size, std::string(buf).size());
	EXPECT_EQ(s1.nextAction(), Session::CLOSE);

	Session s2;
	writeSimpleRequest(s2);
	Stream ::streamsize size2 = 0;
	while (s2.nextAction() == Session::WRSOCK)
		size2 += s2.read(buf, 5);
	EXPECT_EQ(size, size2);
	EXPECT_EQ(s2.nextAction(), Session::CLOSE);
}

TEST(Session, WrongCalls) {
	Session session;
	char buf[BUFSIZE];

	EXPECT_EQ(session.nextAction(), Session::RDSOCK);
	EXPECT_THROW(session.read(buf, BUFSIZE), std::logic_error);

	writeSimpleRequest(session);
	EXPECT_THROW(session.write("Hello world!\n", 13), std::logic_error);

	session.read(buf, BUFSIZE);
	EXPECT_EQ(session.nextAction(), Session::CLOSE);
	EXPECT_THROW(session.write("Hello world!\n", 13), std::logic_error);
	EXPECT_THROW(session.read(buf, BUFSIZE), std::logic_error);
}
