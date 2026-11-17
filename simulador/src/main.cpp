#include "car.h"
#include "render.h"
#include "semaforo.h"
#include "input.h"
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <vector>
using namespace std;

static void onExit(int) {
    restoreInput();
    restoreScreen();
    exit(0);
}

int main() {
    signal(SIGINT, onExit);
    signal(SIGTERM, onExit);

    vector<Car> cars = createCars(25);
    vector<TrafficLight> lights = createTrafficLights();

    initScreen();
    initInput();

    const int TICK_MS = 150;
    SimMode mode = SimMode::FLUID;
    long tick = 0;
    bool running = true;

    while (running) {
        draw(cars, lights, mode, tick);

        int timeout = (mode == SimMode::FLUID) ? TICK_MS : -1;
        Key key = readKey(timeout);

        bool advance = false;
        switch (key) {
            case Key::QUIT:
                running = false;
                break;
            case Key::SPACE:
                mode = (mode == SimMode::FLUID) ? SimMode::TICK : SimMode::FLUID;
                break;
            case Key::UP:
            case Key::DOWN:
            case Key::LEFT:
            case Key::RIGHT:
                mode = SimMode::TICK; // las flechas ponen la simulacion en modo manual
                advance = true;
                break;
            case Key::NONE:
                if (mode == SimMode::FLUID) advance = true; // se agoto el tiempo: sigue solo
                break;
        }

        if (advance) {
            cars.erase(remove_if(cars.begin(), cars.end(), [](Car& car) {
                return !moveCar(car);
            }), cars.end());
            updateTrafficLights(lights);
            ++tick;
        }
    }

    restoreInput();
    restoreScreen();
    return 0;
}
