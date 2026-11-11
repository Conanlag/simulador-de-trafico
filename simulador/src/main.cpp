#include "car.h"
#include "render.h"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <thread>
using namespace std;

void onExit(int) {
    restoreScreen();
    exit(0);
}

int main() {
    signal(SIGINT, onExit);

    vector<Car> cars = createCars(25);

    initScreen();
    while (true) {
        draw(cars);
        for (auto& car : cars) moveCar(car);
        this_thread::sleep_for(chrono::milliseconds(150));
    }
}