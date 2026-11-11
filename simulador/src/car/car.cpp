#include "car.h"
#include "mapa.h"
#include <random>
using namespace std;

static mt19937 rng(random_device{}());

vector<Car> createCars(int n) {
    const int colors[] = {91, 93, 94, 95, 96, 97};
    vector<Car> cars;

    for (int i = 0; i < n; ++i) {
        Car car;
        // Colocar el auto en una calle horizontal o vertical aleatoria
        if (rng() % 2) {
            car.r = (rng() % ((H - 1) / STREET_ROW + 1)) * STREET_ROW;
            car.c = rng() % W;
            car.dr = 0;
            car.dc = (rng() % 2) ? 1 : -1;
        } else {
            car.c = (rng() % ((W - 1) / STREET_COL + 1)) * STREET_COL;
            car.r = rng() % H;
            car.dc = 0;
            car.dr = (rng() % 2) ? 1 : -1;
        }
        car.color = colors[i % 6];
        cars.push_back(car);
    }
    return cars;
}

void moveCar(Car& car) {
    bool intersection = isHStreet(car.r) && isVStreet(car.c);

    if (intersection) {
        // Elegir una dirección válida al azar (sin dar reversa si hay otra opción)
        const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        vector<pair<int, int>> options;
        for (auto& d : dirs) {
            int nr = car.r + d[0], nc = car.c + d[1];
            bool reverse = (d[0] == -car.dr && d[1] == -car.dc);
            if (inBounds(nr, nc) && isStreet(nr, nc) && !reverse)
                options.push_back({d[0], d[1]});
        }
        if (!options.empty()) {
            auto choice = options[rng() % options.size()];
            car.dr = choice.first;
            car.dc = choice.second;
        } else {
            car.dr = -car.dr;
            car.dc = -car.dc;
        }
    }

    // Si la calle termina (borde o fin de calle), dar la vuelta
    int nr = car.r + car.dr, nc = car.c + car.dc;
    if (!inBounds(nr, nc) || !isStreet(nr, nc)) {
        car.dr = -car.dr;
        car.dc = -car.dc;
        nr = car.r + car.dr;
        nc = car.c + car.dc;
    }
    car.r = nr;
    car.c = nc;
}