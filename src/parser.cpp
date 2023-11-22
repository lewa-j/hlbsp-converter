// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#include "parser.h"
#include <cstdint>

Parser::Parser(const char *src, size_t length)
{
	dataStart = src;
	data = dataStart;
	size = length;
}

bool Parser::getToken(std::string &out)
{
	if (!data[0])
		return false;

	if (!skipWhite())
		return false;

	out.clear();
	uint8_t c = (uint8_t)data[0];

	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = (uint8_t)*data;

			// unexpected line end
			if (!c)
			{
				return false;
			}
			data++;

			if (c == '\\' && *data == '"')
			{
				out += *data;

				data++;
				continue;
			}

			if (c == '\"')
			{
				return true;
			}

			out += c;
		}
	}

	if (c == '{' || c == '}')
	{
		data++;
		out = c;
		return true;
	}

	// parse a regular word
	do
	{
		out += c;

		data++;
		c = (uint8_t)*data;

		if (c == '{' || c == '}')
			break;
	} while (c > ' ');

	return true;
}

bool Parser::skipWhite()
{
	while(true)
	{
		uint8_t c;
		while ((c = ((uint8_t)*data)) <= ' ')
		{
			if (c == 0)
			{
				return false;
			}
			data++;
		}

		// skip // comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
		}
		else
		{
			return true;
		}
	}

	return true;
}
