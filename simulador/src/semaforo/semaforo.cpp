#include "semaforo.h"
#include "mapa.h"
#include <cmath>

int lightTicks(LightColor state, const Config& c) {
    double seconds = state == LightColor::GREEN ? c.green : state == LightColor::YELLOW ? c.yellow : c.red;
    return static_cast<int>(std::ceil(seconds * 1000.0 / c.stepMs));
}

std::vector<TrafficLight> createTrafficLights(const Config& c) {
    std::vector<TrafficLight> lights;
    constexpr int bands = BLOCKS + 1;
    for (int i = 0; i < c.lights; ++i) {
        int cross = (2 * i + 1) * bands * bands / (2 * c.lights);
        auto state = static_cast<LightColor>(i % 3);
        lights.push_back({cross / bands * PERIOD + STREET_WIDTH / 2,
                          cross % bands * PERIOD + STREET_WIDTH / 2,
                          state, lightTicks(state, c)});
    }
    return lights;
}

void advanceLight(TrafficLight& light, const Config& config) {
    if (--light.timer <= 0) {
        light.state = static_cast<LightColor>((static_cast<int>(light.state) + 1) % 3);
        light.timer = lightTicks(light.state, config);
    }
}
