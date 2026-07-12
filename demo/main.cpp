#include "Session.hpp"
#include <fstream>
#include <iostream>
#include "unistd.h"

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

#define COLOR_DEF "\x1b[0m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"

ServerConfig config;

static bool setWorkingDirectory(const std::string &path);

static void setupConfig();

static void routeFromServer(RouteConfig &route);

static void demoRequest(char *requestFile);

int main (int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "usage: http-demo <request> [requests...]\n";
		return 0;
	}

	if (!setWorkingDirectory("./config.conf")) { // Replace with config path in real main
		std::cerr << "couldn't setup working directory.\n";
		return 0;
	}
	setupConfig();

	for (int i = 1; i < argc; i++) {
		demoRequest(argv[i]);
		if (i + 1 != argc)
			std::cout << "\n";
	}
	return 0;
}

static bool setWorkingDirectory(const std::string &path) {
	std::string::size_type lastSlash = path.find_last_of('/');
	if (lastSlash == std::string::npos)
		return true;
	std::string directory = path.substr(0, lastSlash);
	return chdir(directory.c_str()) == 0;
}

static void setupConfig() {
	config.host = "127.0.0.1";
	config.port = 8080;
	config.error_pages["default"] = "./errors/default.html";
	config.error_pages["404"] = "./errors/404.html";
	config.max_body_size = 1 * (1024 * 1024);
	config.root = "./";
	config.methods.insert(METHOD_GET);

	RouteConfig route;
	routeFromServer(route);
	route.path = "/";
	route.root = "./www/html";
	route.index.push_back("index.html");
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/css";
	route.root = "./www/css";
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/js";
	route.root = "./www/js";
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/images";
	route.root = "./www/images";
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/old";
	route.redirect = "/";
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/uploads";
	route.root = "./uploads";
	route.methods.clear();
	route.methods.insert(METHOD_GET);
	route.methods.insert(METHOD_POST);
	route.max_body_size = 100 * 1024 * 1024;
	route.upload = true;
	route.upload_dir = ".";
	route.autoindex = true;
	config.routes.push_back(route);

	route = RouteConfig();
	routeFromServer(route);
	route.path = "/cgi";
	route.root = "./cgi";
	route.cgi[".php"] = "/usr/bin/php-cgi";
	route.cgi[".py"] = "/usr/bin/python3";
	config.routes.push_back(route);
}

static void routeFromServer(RouteConfig &route) {
	route.root = config.root;
	route.upload = config.upload;
	route.upload_dir = config.upload_dir;
	route.max_body_size = config.max_body_size;
	route.methods = config.methods;
	route.index = config.index;
	route.error_pages = config.error_pages;

	route.autoindex = false;
}

static void demoRequest(char *requestFile) {
	static char buf[BUFSIZE];

	std::fstream fs(requestFile);
	if (!fs.is_open()) {
		std::cerr << "Can't open " << requestFile << "\n";
		return;
	}
	Session session(config);
	std::cout << "Request: " << requestFile << "\n\n";
	std::cout << BLUE;
	for (std::streamsize size = 0; session.nextAction() == Session::RDSOCK && (fs.read(buf, BUFSIZE) || fs.eof()) && (size = fs.gcount()); size = 0) {
		session.write(buf, size);
		std::cout << std::string(buf, size);
	}
	std::cout << COLOR_DEF << "\nResponse\n\n" << GREEN;
	if (session.nextAction() != Session::WRSOCK)
		std::cerr << RED << "No response available for the given Request\n";
	for (std::streamsize size = 0; session.nextAction() == Session::WRSOCK && (size = session.read(buf, BUFSIZE)); size = 0)
		std::cout << std::string(buf, size);
	std::cout << COLOR_DEF;
}
