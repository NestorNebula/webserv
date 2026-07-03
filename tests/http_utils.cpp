#include "http_utils.hpp"
#include <gtest/gtest.h>

TEST(isAllowedMethod, AllowedMethod) {
	RouteConfig config;

	config.methods.insert(METHOD_GET);
	EXPECT_TRUE(isAllowedMethod(METHOD_GET, config));
}

TEST(isAllowedMethod, NotAllowedMethod) {
	RouteConfig config;

	config.methods.insert(METHOD_POST);
	EXPECT_TRUE(isAllowedMethod(METHOD_GET, config));
}

TEST(isValidVersion, ValidVersion) {
	EXPECT_TRUE(isValidVersion("HTTP/1.1"));
	EXPECT_TRUE(isValidVersion("HTTP/1.0"));
}

TEST(isValidVersion, InvalidVersion) {
	EXPECT_FALSE(isValidVersion("Valid Version"));
	EXPECT_FALSE(isValidVersion("HTTP/1.2"));
	EXPECT_FALSE(isValidVersion("HTTTP/1.2"));
	EXPECT_FALSE(isValidVersion("HTTP/1.x"));
	EXPECT_FALSE(isValidVersion("HTTP/x.1"));
	EXPECT_FALSE(isValidVersion("HTTP 1.1"));
}

TEST(findBestRoute, OneRoute) {
	ServerConfig server;
	RouteConfig route;

	route.path = "/route";
	server.routes.push_back(route);
	EXPECT_EQ(findBestRoute("/route", server), &route);
	EXPECT_EQ(findBestRoute("/route/index.html", server), &route);
	EXPECT_EQ(findBestRoute("/route/subroute/index.html", server), &route);
}

TEST(findBestRoute, MultipleRoutes) {
	ServerConfig server;
	RouteConfig r1, r2, r3;

	r1.path = "/route";
	r2.path = "/route/subroute";
	r3.path = "/another/subroute";
	server.routes.push_back(r1);
	server.routes.push_back(r2);
	server.routes.push_back(r3);
	EXPECT_EQ(findBestRoute("/route", server), &r1);
	EXPECT_EQ(findBestRoute("/route/subroute", server), &r2);
	EXPECT_EQ(findBestRoute("/another/subroute", server), &r3);
	EXPECT_EQ(findBestRoute("/route/index.html", server), &r1);
	EXPECT_EQ(findBestRoute("/route/subroute/index.html", server), &r2);
	EXPECT_EQ(findBestRoute("/another/subroute/index.html", server), &r3);
}

TEST(findBestRoute, InvalidRoutes) {
	ServerConfig server;
	RouteConfig r1, r2, r3;

	r1.path = "/route";
	r2.path = "/route/subroute";
	r3.path = "/another/subroute";
	EXPECT_EQ(findBestRoute("/not-my-route", server), NULL);
	EXPECT_EQ(findBestRoute("/rout", server), NULL);
	EXPECT_EQ(findBestRoute("/routes", server), NULL);
}

TEST(resolvePath, SameRoute) {
	RouteConfig route;

	route.path = "/route";
	route.root = "/route";
	EXPECT_EQ(resolvePath("/route", route), "/route");
	EXPECT_EQ(resolvePath("/route/subroute", route), "/route/subroute");
	EXPECT_EQ(resolvePath("/route/index.html", route), "/route/index.html");
}

TEST(resolvePath, DifferentRoute) {
	RouteConfig route;

	route.path = "/route";
	route.root = "/another";
	EXPECT_EQ(resolvePath("/route", route), "/another");
	EXPECT_EQ(resolvePath("/route/subroute", route), "/another/subroute");
	EXPECT_EQ(resolvePath("/route/index.html", route), "/another/index.html");
}

TEST(isExistingFile, ExistingFile) {
	EXPECT_TRUE(isExistingFile("files/empty.txt"));
	EXPECT_TRUE(isExistingFile("files/small.txt"));
	EXPECT_TRUE(isExistingFile("files/large.txt"));
}

TEST(isExistingFile, NonExistingFile) {
	EXPECT_FALSE(isExistingFile("empty.txt"));
	EXPECT_FALSE(isExistingFile("small.txt"));
	EXPECT_FALSE(isExistingFile("large.txt"));
}

TEST(isDirectory, Directory) {
	EXPECT_TRUE(isDirectory("files"));
	EXPECT_TRUE(isDirectory("."));
	EXPECT_TRUE(isDirectory(".."));
}

TEST(isDirectory, NonDirectory) {
	EXPECT_FALSE(isDirectory("non-existing"));
	EXPECT_FALSE(isDirectory("files/empty.txt"));
}

// TODO: isCgi tests

TEST(isAccessibleFile, AccessibleFile) {
	EXPECT_TRUE(isAccessibleFile("files/empty.txt"));
	EXPECT_TRUE(isAccessibleFile("files/small.txt"));
	EXPECT_TRUE(isAccessibleFile("files/large.txt"));
}

TEST(isAccessibleFile, NonAccessibleFile) {
	EXPECT_FALSE(isAccessibleFile("non-existing"));
}
