/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 12:54:00 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/06 14:52:32 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <exception>

struct RouteConfig {
	std::string	path;
	std::string	root;
	bool		autoindex;
	bool		upload;
	size_t		max_body_size;

	RouteConfig();
};

struct ServerConfig {
	std::string	host;
	int			port;
	size_t		max_body_size;
	std::string	root;
	bool		upload;
	std::string	upload_dir;

	std::map<std::string, std::string>	error_pages;
	std::vector<RouteConfig>			routes;

	ServerConfig();
};

enum TokenType {
	TOKEN_WORD,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_COLON,
	TOKEN_COMMA,
	TOKKEN_NEWLINE,
	TOKEN_EOF
};

struct Token {
	TokenType	type;
	std::string	value;
	size_t		line;
};

class ConfigParser {
	private:
		std::vector<Token>			_tokens;
		size_t						_pos;
		std::vector<ServerConfig>	_servers;

		/* Parser Tools */
		void	tokenize(const std::string& content);
		Token	peek();
		Token	consume();
		Token	expect(TokenType type);
		void	skipNewlines();

		/* Work Logical */
		void	parseServer();
		void	parseRoute();
		void	parseDirective(std::map<std::string, std::string>& directives_map,
								std::set<std::string>& seen_directives);
		size_t	parseSize(const std::string& sizeStr);

		/* Validation */
		void	validateServerConfig(const ServerConfig& server);
		void	validateRouteConfig(const RouteConfig& route);

	public:
		ConfigParser();
		~ConfigParser();

		void	parseFile(const std::string& filename);

		const std::vector<ServerConfig>& getServers() const { return _servers; }

		class ConfigException : public std::exception {
			private:
				std::string _msg;
			public:
				ConfigException(const std::string& msg);
				virtual ~ConfigException() throw();
				virtual const char* what() const throw();
		};
	};

#endif
