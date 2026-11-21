#pragma once
#include <iosfwd>
#include <string>
#include <vector>

struct Point {
    int r = 0, c = 0;
    bool operator==(const Point& p) const { return r == p.r && c == p.c; }
    bool operator!=(const Point& p) const { return !(*this == p); }
};

struct Config {
    int vehicles = 25;
    int lights = 20;
    double green = 5, yellow = 2, red = 6;
    int stepMs = 150;
    int frameMs = 150;
    int maxTicks = 20000;
    int plannerThreads = 4;
    unsigned seed = 42;
    std::vector<Point> starts, destinations;
};

Config loadConfig(const std::string& path);
void validateConfig(const Config& config);

// Pregunta antes de activar el modo de terminal de la simulacion.
void askVehicleCount(Config& config, std::istream& input, std::ostream& output);
