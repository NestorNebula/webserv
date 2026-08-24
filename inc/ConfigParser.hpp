/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 12:54:00 by mamarti           #+#    #+#             */
/*   Updated: 2026/08/24 18:03:33 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include "HttpMethod.hpp"
# include "ServerConfig.hpp"
# include "RouteConfig.hpp"

# include <exception>

enum TokenType {
	TOKEN_WORD,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_COLON,
	TOKEN_COMMA,
	TOKEN_NEWLINE,
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
// #kd - conf_file_root
		const std::string			_conf_file_root;

		/* Parser Tools */
		void	tokenize(const std::string& content);
		Token	peek();
		Token	consume();
		Token	expect(TokenType type);
		void	skipNewlines();

		/* Logical Work */
		void	parseServer();
		void	parseRoute(ServerConfig& current_server);
		void	parseDirective(std::map<std::string, std::string>& directives_map,
								std::set<std::string>& seen_directives);
		size_t	parseSize(const std::string& sizeStr);
		int		parsePort(const std::string& portStr);

		/* Validation */
		void	validateServerConfig(const ServerConfig& server);
		void	validateRouteConfig(const RouteConfig& route);
		void	validateCGIExecutables(const RouteConfig& route);
		void	validateDirExists(const std::string& path, const std::string& context);
		void	validateFileExists(const std::string& path, const std::string& context);

		
	public:
// #kd - conf_file_root
		ConfigParser(const std::string &conf_file_root);
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
