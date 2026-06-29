/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserTools.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 14:17:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/29 14:24:10 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <sstream>

Token	ConfigParser::peek()
{
	if (_pos < _tokens.size())
		return _tokens[_pos];
	return _tokens.back(); // EOF on overflow
}

Token	ConfigParser::consume()
{
	Token	t = peek();
	if (_pos < _tokens.size())
		_pos++;
	return t;
}

Token	ConfigParser::expect(TokenType type)
{
	Token	t = consume();
	if (t.type != type)
	{
		std::stringstream	ss;
		ss << "Unexpected token '" << t.value << "' at line " << t.line << ".";
		throw ConfigException(ss.str());
	}
	return t;
}

void	ConfigParser::skipNewlines()
{
	while (peek().type == TOKEN_NEWLINE)
		consume();
}
