#pragma once
#include "config.h"

// Copia de un vehiculo para dibujar; el estado concurrente vive en Simulation.
struct Car {
    int r, c;
    int dr, dc;
    int color;
};
Car carView(Point current, Point next, int id);
