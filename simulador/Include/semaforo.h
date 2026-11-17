#pragma once
#include <vector>

enum class LightColor { GREEN, YELLOW, RED };

struct TrafficLight {
    int row, col;   // celda central de la interseccion que controla
    LightColor state;
    int timer;      // ticks que le quedan al estado actual
};

// Crea un semaforo en el centro de cada interseccion de la ciudad.
// Arrancan repartidos entre los 3 estados para que no cambien todos a la vez.
std::vector<TrafficLight> createTrafficLights();

// Avanza un tick el reloj de cada semaforo (aun no controla a los autos,
// solo cambia de color para que se vean funcionando).
void updateTrafficLights(std::vector<TrafficLight>& lights);