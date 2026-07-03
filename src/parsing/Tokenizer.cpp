/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 14:15:36 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 12:54:30 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

#define SPACE		' '
#define TAB			'\t'
#define CRLF		'\r'
#define NEWLINE		'\n'
#define HASHTAG		'#'
#define LBRACE		'{'
#define RBRACE		'}'
#define COLON		':'
#define COMMA		','
#define EOFILE		"EOF"

static bool	isSpace(char c)
{
	if (c == SPACE || c == TAB || c == CRLF)
		return (true);
	return (false);
}

static bool isPunctuation(char c)
{
	if (c == LBRACE || c == RBRACE || c == COLON || c == COMMA)
		return (true);
	return (false);
}

static bool isSeparator(char c)
{
	if (isSpace(c) || isPunctuation(c) || c == NEWLINE || c == HASHTAG)
		return (true);
	return (false);
}

void	ConfigParser::tokenize(const std::string& content)
{
	size_t i = 0;
	size_t line = 1;

	while (i < content.length())
	{
		char c = content[i];

		// SPACES
		if (isSpace(c))
		{
			i++;
			continue;
		}

		// NEWLINE
		if (c == NEWLINE)
		{
			Token	t;
			t.type = TOKEN_NEWLINE;
			t.value = "\\n";
			t.line = line;
			_tokens.push_back(t);
			line++;
			i++;
			continue;
		}

		// COMMENTS
		if (c == HASHTAG)
		{
			while (i < content.length() && content[i] != NEWLINE)
				i++;
			continue;
		}

		// PONCTUATIONS, CARAC
		if (isPunctuation(c))
		{
			Token	t;
			t.line = line;
			if (c == LBRACE)
				t.type = TOKEN_LBRACE;
			else if (c == RBRACE)
				t.type = TOKEN_RBRACE;
			else if (c == COLON)
				t.type = TOKEN_COLON;
			else if (c == COMMA)
				t.type = TOKEN_COMMA;
			t.value = c;
			_tokens.push_back(t);
			i++;
			continue;
		}

		// WORDS
		std::string word = "";
		while (i < content.length() && !isSeparator(content[i]))
		{
			word += content[i];
			i++;
		}

		if (!word.empty())
		{
			Token	t;
			t.type = TOKEN_WORD;
			t.value = word;
			t.line = line;
			_tokens.push_back(t);
		}
	}
	Token	eof;
	eof.type = TOKEN_EOF;
	eof.value = EOFILE;
	eof.line = line;
	_tokens.push_back(eof);
}
