#pragma once

#include "framework.h"
#include <profileapi.h>

class Timer {
public:
	Timer();

	double get_time(int seconds);
private:
	LARGE_INTEGER frequency;
};

