#pragma once
#include <vector>

struct Car {
    int r, c;    // posición
    int dr, dc;  // dirección
    int color;   // código de color ANSI
};

std::vector<Car> createCars(int n);
void moveCar(Car& car);