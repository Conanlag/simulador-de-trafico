#pragma once
#include "config.h"
#include <vector>

enum class LightColor { GREEN, YELLOW, RED };
struct TrafficLight {
    int row, col;
    LightColor state;
    int timer;
};
int lightTicks(LightColor state, const Config& config);
std::vector<TrafficLight> createTrafficLights(const Config& config);
void advanceLight(TrafficLight& light, const Config& config);
