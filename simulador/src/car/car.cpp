#include "car.h"
#include "mapa.h"
#include <random>
#include <utility>
using namespace std;

static mt19937 rng(random_device{}());

// Cuantas bandas de calle hay por eje: una al inicio de cada manzana,
// mas una extra al final para cerrar la ciudad.
static const int NUM_BANDS = BLOCKS + 1;

static int randomBandStart() {
    return (rng() % NUM_BANDS) * PERIOD;
}

vector<Car> createCars(int n) {
    const int colors[] = {91, 93, 94, 95, 96, 97};
    vector<Car> cars;

    for (int i = 0; i < n; ++i) {
        Car car{};
        bool horizontal = rng() % 2;
        int start = randomBandStart();
        int dir = streetDirection(start);

        if (horizontal) {
            car.dr = 0;
            car.dc = dir;
            car.r  = start + laneOffset(dir);
            car.c  = rng() % W;
        } else {
            car.dr = dir;
            car.dc = 0;
            car.c  = start + laneOffset(dir);
            car.r  = rng() % H;
        }

        car.color = colors[i % 6];
        cars.push_back(car);
    }
    return cars;
}

bool moveCar(Car& car) {
    // Punto de decision: el auto cruza el centro de la calle perpendicular
    bool decision = (car.dc != 0 && isBandCenter(car.c)) ||
                    (car.dr != 0 && isBandCenter(car.r));

    if (decision) {
        const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        vector<pair<int, int>> options;

        for (auto& d : dirs) {
            bool reverse = (d[0] == -car.dr && d[1] == -car.dc);
            if (reverse) continue;
            // Cada calle permite un unico sentido, incluso al girar.
            if (d[0] != 0 && d[0] != streetDirection(car.c)) continue;
            if (d[1] != 0 && d[1] != streetDirection(car.r)) continue;
            if (inBounds(car.r + d[0], car.c + d[1]))
                options.push_back({d[0], d[1]});
        }

        if (!options.empty()) {
            auto choice = options[rng() % options.size()];
            int newDr = choice.first;
            int newDc = choice.second;

            if (newDr != 0) {
                // Ahora avanza vertical: fija el carril en la calle vertical
                car.c = bandStart(car.c) + laneOffset(newDr);
            } else {
                // Ahora avanza horizontal: fija el carril en la calle horizontal
                car.r = bandStart(car.r) + laneOffset(newDc);
            }
            car.dr = newDr;
            car.dc = newDc;
        }
    }

    // Al salir del mapa, el auto termina su recorrido.
    car.r += car.dr;
    car.c += car.dc;
    return inBounds(car.r, car.c);
}
