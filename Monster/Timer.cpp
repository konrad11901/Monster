#include "Timer.h"

Timer::Timer() {
    QueryPerformanceFrequency(&frequency);
}

double Timer::get_time(int seconds) {
    LARGE_INTEGER curr_counter;
    QueryPerformanceCounter(&curr_counter);

    auto difference = curr_counter.QuadPart % (frequency.QuadPart * seconds);

    return (double)difference / (double)frequency.QuadPart;
}