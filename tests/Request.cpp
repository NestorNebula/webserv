#include "Request.hpp"
#include <gtest/gtest.h>

static void expectHeader(const Headers &headers, std::string key,
                         std::string value) {
  ASSERT_TRUE(headers.has(key));
  EXPECT_EQ(headers.find(key)->second, value);
}

TEST(Request, BasicGet) {
  Request req;

  EXPECT_FALSE(req.isComplete());
  EXPECT_FALSE(req.isInvalid());
  req.append("GET / HTTP/1.1 \r\n"
             "Host: localhost\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getMethod(), "GET");
  EXPECT_EQ(req.getURL(), "/");
  EXPECT_EQ(req.getVersion(), "HTTP/1.1");
  EXPECT_EQ((int) req.getHeaders().size(), 1);
  expectHeader(req.getHeaders(), "Host", "localhost");
  EXPECT_EQ(req.getRaw(), "GET / HTTP/1.1 \r\n"
                          "Host: localhost\r\n"
                          "\r\n");
}

TEST(Request, MultipleHeaders) {
  Request req;

  req.append("GET /index.html HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "User-Agent: Firefox\r\n"
             "Accept: text/html\r\n"
             "Connection: close\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getURL(), "/index.html");
  EXPECT_EQ((int) req.getHeaders().size(), 4);
  expectHeader(req.getHeaders(), "Host", "localhost");
  expectHeader(req.getHeaders(), "User-Agent", "Firefox");
  expectHeader(req.getHeaders(), "Accept", "text/html");
  expectHeader(req.getHeaders(), "Connection", "close");
}

TEST(Request, QueryString) {
  Request req;

  req.append("GET /search?q=webserv&page=2 HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getURL(), "/search");
  EXPECT_EQ(req.getQuery(), "q=webserv&page=2");
}

TEST(Request, BasicPost) {
  Request req;

  req.append("POST /login HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Content-Length: 29\r\n"
             "\r\n"
             "username=nhoussie&password=42");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getMethod(), "POST");
  EXPECT_EQ((int) req.getHeaders().size(), 2);
  expectHeader(req.getHeaders(), "Host", "localhost");
  expectHeader(req.getHeaders(), "Content-Length", "29");
  EXPECT_EQ((int) req.getBody().size(), 29);
  EXPECT_EQ(req.getBody(), "username=nhoussie&password=42");
}

TEST(Request, EmptyPost) {
  Request req;

  req.append("POST /upload HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Content-Length: 0\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_TRUE(req.getBody().empty());
}

TEST(Request, BasicDelete) {
  Request req;

  req.append("DELETE /uploads/test.txt HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getMethod(), "DELETE");
  EXPECT_EQ((int) req.getHeaders().size(), 1);
  expectHeader(req.getHeaders(), "Host", "localhost");
}

TEST(Request, CutHeader) {
  Request req;

  req.append("GET / HTTP/1.1\r\n"
             "Host: loca");
  EXPECT_TRUE(req.hasMethod());
  EXPECT_TRUE(req.hasURL());
  EXPECT_FALSE(req.hasQuery());
  EXPECT_TRUE(req.hasVersion());
  EXPECT_FALSE(req.hasHeaders());
  req.append("lhost\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ((int) req.getHeaders().size(), 1);
  expectHeader(req.getHeaders(), "Host", "localhost");
}

TEST(Request, CutStartLine) {
  Request req;

  req.append("GET /inde");
  req.append("x.html HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "\r\n");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ(req.getURL(), "/index.html");
}

TEST(Request, CutBody) {
  Request req;

  req.append("POST /upload HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Content-Length: 5\r\n"
             "\r\n"
             "Hel");
  req.append("lo");
  EXPECT_TRUE(req.hasBody());
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ((int) req.getBody().size(), 5);
  EXPECT_EQ(req.getBody(), "Hello");
}

TEST(Request, InvalidHeader) {
  Request req;

  req.append("GET / HTTP/1.1\r\n"
             "Host localhost\r\n"
             "\r\n");
  EXPECT_FALSE(req.isComplete());
  EXPECT_TRUE(req.isInvalid());
}

TEST(Request, NoMethod) {
  Request req;

  req.append("/ HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "\r\n");
  EXPECT_FALSE(req.isComplete());
  EXPECT_TRUE(req.isInvalid());
}

TEST(Request, MultipleHeadersPost) {
  Request req;

  req.append("POST /form HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "User-Agent: Chrome\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: 29\r\n"
             "\r\n"
             "username=nhoussie&password=42");
  EXPECT_TRUE(req.isComplete());
  EXPECT_EQ((int) req.getHeaders().size(), 4);
  expectHeader(req.getHeaders(), "Host", "localhost");
  expectHeader(req.getHeaders(), "User-Agent", "Chrome");
  expectHeader(req.getHeaders(), "Content-Type",
               "application/x-www-form-urlencoded");
  expectHeader(req.getHeaders(), "Content-Length", "29");
  EXPECT_EQ((int) req.getBody().size(), 29);
  EXPECT_EQ(req.getBody(), "username=nhoussie&password=42");
}
