#include "semaforo.h"
#include "mapa.h"

// Duracion de cada estado, en ticks de simulacion
constexpr int GREEN_TICKS  = 18;
constexpr int YELLOW_TICKS = 4;
constexpr int RED_TICKS    = 18;

static int ticksFor(LightColor state) {
    switch (state) {
        case LightColor::GREEN:  return GREEN_TICKS;
        case LightColor::YELLOW: return YELLOW_TICKS;
        case LightColor::RED:    return RED_TICKS;
    }
    return GREEN_TICKS;
}

static LightColor nextState(LightColor state) {
    switch (state) {
        case LightColor::GREEN:  return LightColor::YELLOW;
        case LightColor::YELLOW: return LightColor::RED;
        case LightColor::RED:    return LightColor::GREEN;
    }
    return LightColor::GREEN;
}

std::vector<TrafficLight> createTrafficLights() {
    std::vector<TrafficLight> lights;
    int idx = 0;

    for (int r = 0; r < H; ++r) {
        if (!isBandCenter(r)) continue;
        for (int c = 0; c < W; ++c) {
            if (!isBandCenter(c)) continue;

            TrafficLight light;
            light.row = r;
            light.col = c;

            // Se reparten en las 3 fases para que no cambien todos a la vez
            int phase = idx % 3;
            light.state = (phase == 0) ? LightColor::GREEN
                         : (phase == 1) ? LightColor::YELLOW
                                        : LightColor::RED;
            light.timer = ticksFor(light.state);

            lights.push_back(light);
            ++idx;
        }
    }
    return lights;
}

void updateTrafficLights(std::vector<TrafficLight>& lights) {
    for (auto& light : lights) {
        if (--light.timer <= 0) {
            light.state = nextState(light.state);
            light.timer = ticksFor(light.state);
        }
    }
}