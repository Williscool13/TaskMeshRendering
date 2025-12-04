//
// Created by William on 2025-10-30.
//

#ifndef WILLENGINETESTBED_TIME_H
#define WILLENGINETESTBED_TIME_H
#include <chrono>
#include <cstdint>

class Time
{
public:
    static Time& Get()
    {
        static Time instance{};
        return instance;
    }

    Time();

    void Reset();

    void Update();

    [[nodiscard]] float GetDeltaTime() const;

    [[nodiscard]] float GetTime() const;

private:
    uint64_t deltaTime{};
    std::chrono::time_point<std::chrono::steady_clock> lastTime;
};

#endif //WILLENGINETESTBED_TIME_H
