// Copyright (c) 2022 Alexey Ivanchukov (lewa_j)
#pragma once

#include <string>

class Parser
{
public:
	Parser(const char *src, size_t length);
	// returns false on end of file
	bool getToken(std::string &out);

	bool skipWhite();

	const char *dataStart = nullptr;
	const char *data = nullptr;
	size_t size = 0;
};
