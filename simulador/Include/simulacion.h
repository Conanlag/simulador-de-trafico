#pragma once
#include "car.h"
#include "rutas.h"
#include "semaforo.h"
#include "mapa.h"
#include <array>
#include <condition_variable>
#include <mutex>
#include <ostream>
#include <thread>
#include <vector>

struct VehicleState {
    std::size_t position = 0;
    bool active = false, arrived = false;
    int crossing = -1;
    long arrivalTick = 0, waitTicks = 0, lightWaitTicks = 0;
    std::vector<int> reservations;
};
struct Snapshot {
    std::vector<Car> cars;
    std::vector<TrafficLight> lights;
    long tick = 0;
    int arrived = 0, waiting = 0;
};

class Simulation {
public:
    Simulation(const Config& config, const Planning& planning);
    ~Simulation();
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    bool step();
    void stop();
    Snapshot snapshot();
    void report(std::ostream& out);
    void writeCsv(const std::string& prefix);
    bool complete();
private:
    static constexpr int CROSSINGS = (BLOCKS + 1) * (BLOCKS + 1);
    Config config;
    Planning planning;
    std::vector<VehicleState> vehicles;
    std::vector<TrafficLight> lights;
    std::array<std::mutex, CROSSINGS> crossingLocks;
    std::array<int, CROSSINGS> crossingOwner, lightIndex, queues{}, peakQueues{};
    std::array<int, H * W> occupancy, reserved;
    std::mutex monitor;
    std::condition_variable changed;
    std::vector<std::thread> workers;
    bool stopping = false;
    long tick = 0, carEpoch = 0, lightEpoch = 0;
    int carsPending = 0, lightsPending = 0, arrived = 0;
    long checks = 0, conflicts = 0, collisions = 0;
    std::vector<int> arrivalOrder;
    static int cell(Point p) { return p.r * W + p.c; }
    static int crossing(Point p);
    void worker(int id, bool light);
    void move(int id);
    void release(int id);
    void wait(int id, int cross, bool red);
    void verify();
};
