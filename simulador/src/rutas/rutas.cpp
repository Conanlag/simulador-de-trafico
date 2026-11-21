#include "rutas.h"
#include "mapa.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <queue>
#include <random>
#include <stdexcept>
#include <omp.h>

std::vector<Point> successors(Point p) {
    std::vector<Point> next;
    if (isBandCenter(p.r)) {
        Point q{p.r, p.c + streetDirection(p.r)};
        if (inBounds(q.r, q.c)) next.push_back(q);
    }
    if (isBandCenter(p.c)) {
        Point q{p.r + streetDirection(p.c), p.c};
        if (inBounds(q.r, q.c)) next.push_back(q);
    }
    return next;
}

std::vector<Point> shortestRoute(Point start, Point destination) {
    if (!inBounds(start.r, start.c) || !inBounds(destination.r, destination.c)) return {};
    std::array<int, H * W> parent;
    parent.fill(-1);
    auto index = [](Point p) { return p.r * W + p.c; };
    int origin = index(start), target = index(destination);
    std::queue<Point> pending;
    pending.push(start);
    parent[origin] = origin;
    while (!pending.empty() && parent[target] == -1) {
        Point p = pending.front();
        pending.pop();
        for (Point q : successors(p)) {
            int id = index(q);
            if (parent[id] != -1) continue;
            parent[id] = index(p);
            pending.push(q);
        }
    }
    if (parent[target] == -1) return {};
    std::vector<Point> result;
    for (int id = target;; id = parent[id]) {
        result.push_back({id / W, id % W});
        if (id == origin) break;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

static bool entry(Point p) {
    if (!inBounds(p.r, p.c)) return false;
    return (isBandCenter(p.r) && p.c == (streetDirection(p.r) > 0 ? 0 : W-1)) ||
           (isBandCenter(p.c) && p.r == (streetDirection(p.c) > 0 ? 0 : H-1));
}

static bool internalDestination(Point p) {
    return p.r > 0 && p.r < H-1 && p.c > 0 && p.c < W-1 &&
           (isBandCenter(p.r) || isBandCenter(p.c));
}

Planning planRoutes(const Config& config) {
    Planning result;
    std::vector<Point> entries, destinations;
    for (int b = 0; b <= BLOCKS; ++b) {
        int x = b * PERIOD + STREET_WIDTH / 2;
        int dir = streetDirection(x);
        entries.push_back({x, dir > 0 ? 0 : W-1});
        entries.push_back({dir > 0 ? 0 : H-1, x});
    }
    // El nucleo de calles interiores es accesible desde todas las entradas.
    // Las bandas exteriores tienen tramos de un solo sentido sin acceso de regreso.
    for (int r = STREET_WIDTH; r < H-STREET_WIDTH; ++r)
        for (int c = STREET_WIDTH; c < W-STREET_WIDTH; ++c)
            if (internalDestination({r, c}) && !isIntersection(r, c))
                destinations.push_back({r, c});
    std::mt19937 random(config.seed);
    for (int i = 0; i < config.vehicles; ++i) {
        Trip trip{config.starts.empty() ? entries[random() % entries.size()] : config.starts[i],
                  config.destinations.empty() ? destinations[random() % destinations.size()] : config.destinations[i]};
        if (!entry(trip.start) || !internalDestination(trip.destination) || trip.start == trip.destination)
            throw std::runtime_error("Origen/destino invalido del vehiculo " + std::to_string(i + 1) + ": use una entrada del borde y un destino en un carril dentro del mapa");
        result.trips.push_back(trip);
    }
    std::vector<std::vector<Point>> sequential(config.vehicles);
    result.routes.resize(config.vehicles);
    using Clock = std::chrono::steady_clock;
    // Precalienta el equipo de OpenMP; ambos calculos usan exactamente los mismos viajes.
    #pragma omp parallel num_threads(config.plannerThreads)
    { (void)omp_get_thread_num(); }
    auto begin = Clock::now();
    for (int i = 0; i < config.vehicles; ++i)
        sequential[i] = shortestRoute(result.trips[i].start, result.trips[i].destination);
    result.sequentialMs = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    begin = Clock::now();
    #pragma omp parallel for num_threads(config.plannerThreads) schedule(static)
    for (int i = 0; i < config.vehicles; ++i)
        result.routes[i] = shortestRoute(result.trips[i].start, result.trips[i].destination);
    result.parallelMs = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
    for (int i = 0; i < config.vehicles; ++i) {
        if (result.routes[i].empty()) throw std::runtime_error("Destino inalcanzable del vehiculo " + std::to_string(i + 1));
        if (result.routes[i] != sequential[i]) throw std::runtime_error("Las rutas secuenciales y paralelas difieren");
    }
    return result;
}
