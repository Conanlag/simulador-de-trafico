#pragma once
#include "car.h"
#include "semaforo.h"
#include <vector>

enum class SimMode { FLUID, TICK };

void initScreen();
void restoreScreen();

// Dibuja la grilla completa, los semaforos, los autos y la barra de
// controles de abajo.
void draw(const std::vector<Car>& cars,
          const std::vector<TrafficLight>& lights,
          SimMode mode,
          long tick);