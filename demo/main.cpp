#include "ConfigParser.hpp"
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
	ConfigParser parser;
	parser.parseFile("config.conf");
	config = parser.getServers()[0];

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
