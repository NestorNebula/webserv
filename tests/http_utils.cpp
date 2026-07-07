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
	EXPECT_FALSE(isAllowedMethod(METHOD_GET, config));
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
	EXPECT_EQ(findBestRoute("/route", server), &server.routes[0]);
	EXPECT_EQ(findBestRoute("/route/index.html", server), &server.routes[0]);
	EXPECT_EQ(findBestRoute("/route/subroute/index.html", server), &server.routes[0]);
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
	EXPECT_EQ(findBestRoute("/route", server), &server.routes[0]);
	EXPECT_EQ(findBestRoute("/route/subroute", server), &server.routes[1]);
	EXPECT_EQ(findBestRoute("/another/subroute", server), &server.routes[2]);
	EXPECT_EQ(findBestRoute("/route/index.html", server), &server.routes[0]);
	EXPECT_EQ(findBestRoute("/route/subroute/index.html", server), &server.routes[1]);
	EXPECT_EQ(findBestRoute("/another/subroute/index.html", server), &server.routes[2]);
}

TEST(findBestRoute, InvalidRoutes) {
	ServerConfig server;
	RouteConfig r1, r2, r3;

	r1.path = "/route";
	r2.path = "/route/subroute";
	r3.path = "/another/subroute";
	server.routes.push_back(r1);
	server.routes.push_back(r2);
	server.routes.push_back(r3);
	EXPECT_EQ(findBestRoute("/not-my-route", server), (void *) NULL);
	EXPECT_EQ(findBestRoute("/rout", server), (void *) NULL);
	EXPECT_EQ(findBestRoute("/routes", server), (void *) NULL);
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

TEST(joinPaths, BasicPaths) {
	EXPECT_EQ(joinPaths("prefix/", "suffix"), "prefix/suffix");
	EXPECT_EQ(joinPaths("prefix", "/suffix"), "prefix/suffix");
	EXPECT_EQ(joinPaths("/", "suffix"), "/suffix");
	EXPECT_EQ(joinPaths("prefix", "/"), "prefix/");
}

TEST(joinPaths, SpecialPaths) {
	EXPECT_EQ(joinPaths("/", "/"), "/");
	EXPECT_EQ(joinPaths("prefix", ""), "prefix");
	EXPECT_EQ(joinPaths("", "suffix"), "suffix");
	EXPECT_EQ(joinPaths("", ""), "");
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

TEST(isCgi, CGIFiles) {
	RouteConfig config;

	config.cgi.insert(std::pair<std::string, std::string>(".php", "php-cgi"));
	config.cgi.insert(std::pair<std::string, std::string>(".py", "python3"));
	EXPECT_TRUE(isCgi("cgi.php", config));
	EXPECT_TRUE(isCgi("cgi.py", config));
}

TEST(isCgi, NonCGIFiles) {
	RouteConfig config;

	config.cgi.insert(std::pair<std::string, std::string>(".php", "php-cgi"));
	config.cgi.insert(std::pair<std::string, std::string>(".py", "python3"));
	EXPECT_FALSE(isCgi("noncgi.cgi", config));
	config.cgi.clear();
	EXPECT_FALSE(isCgi("cgi.php", config));
	EXPECT_FALSE(isCgi("cgi.py", config));
}

TEST(isAccessibleFile, AccessibleFile) {
	EXPECT_TRUE(isAccessibleFile("files/empty.txt"));
	EXPECT_TRUE(isAccessibleFile("files/small.txt"));
	EXPECT_TRUE(isAccessibleFile("files/large.txt"));
}

TEST(isAccessibleFile, NonAccessibleFile) {
	EXPECT_FALSE(isAccessibleFile("non-existing"));
}
