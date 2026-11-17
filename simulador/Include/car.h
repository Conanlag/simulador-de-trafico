#pragma once
#include <vector>

struct Car {
    int r, c;    // posicion actual
    int dr, dc;  // direccion de avance (uno de los dos siempre es 0)
    int color;   // codigo de color ANSI para dibujarlo
};

// Crea n autos, cada uno en un carril valido de alguna calle,
// respetando el sentido de esa calle.
std::vector<Car> createCars(int n);

// Mueve un auto un paso. En el centro de cada interseccion que cruza,
// decide al azar si sigue derecho o gira en el sentido de la calle destino.
// Devuelve false si salio del mapa y debe retirarse de la simulacion.
bool moveCar(Car& car);
