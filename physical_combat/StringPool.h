#pragma once

// 5F0
class StringPool {
public:
	static constexpr auto kLeft = 0xBA;

	const char *strings[190];

	static StringPool *GetInstance();
};
