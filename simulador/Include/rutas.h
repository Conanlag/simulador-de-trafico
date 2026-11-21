#pragma once
#include "config.h"
#include <vector>

struct Trip { Point start, destination; };
struct Planning {
    std::vector<Trip> trips;
    std::vector<std::vector<Point>> routes;
    double sequentialMs = 0, parallelMs = 0;
};

// Grafo dirigido de los carriles centrales; cada arista cuesta un paso.
std::vector<Point> successors(Point p);
std::vector<Point> shortestRoute(Point start, Point destination);
Planning planRoutes(const Config& config);
