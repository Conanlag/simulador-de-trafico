#include "config.h"
#include "mapa.h"
#include "rutas.h"
#include "semaforo.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

int main() {
    Config config;
    for (int b = 0; b <= BLOCKS; ++b) {
        int center = b * PERIOD + STREET_WIDTH/2;
        int dir = b % 2 ? -1 : 1;
        assert(streetDirection(center) == dir);
        auto route = shortestRoute({center, dir > 0 ? 0 : W-1}, {center, dir > 0 ? W-1 : 0});
        assert(route.size() == static_cast<std::size_t>(W));
        for (std::size_t i = 1; i < route.size(); ++i) {
            assert(route[i].r == center);
            assert(route[i].c - route[i-1].c == dir);
        }
        route = shortestRoute({dir > 0 ? 0 : H-1, center}, {dir > 0 ? H-1 : 0, center});
        assert(route.size() == static_cast<std::size_t>(H));
    }
    assert(shortestRoute({5,50}, {1,50}).size() == 15); // desvio minimo: 5 oeste + 4 norte + 5 este
    assert(shortestRoute({1,0}, {50,1}).size() == 51); // giro hacia el sur
    assert(shortestRoute({-1,0}, {1,50}).empty());
    TrafficLight light{1,1,LightColor::GREEN,lightTicks(LightColor::GREEN,config)};
    for (LightColor state : {LightColor::GREEN, LightColor::YELLOW, LightColor::RED}) {
        assert(light.state == state);
        int ticks = lightTicks(state, config);
        for (int i = 1; i < ticks; ++i) { advanceLight(light, config); assert(light.state == state); }
        advanceLight(light, config);
    }
    assert(light.state == LightColor::GREEN);
    config.vehicles = 19;
    bool rejected = false;
    try { validateConfig(config); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    Config interactive;
    std::ostringstream prompt;
    std::istringstream answers("abc\n19\n201\n20.5\n20 basura\n20\n");
    askVehicleCount(interactive, answers, prompt);
    assert(interactive.vehicles == 20);
    assert(prompt.str().find("numero entero") != std::string::npos);
    assert(prompt.str().find("entre 20 y 200") != std::string::npos);
    std::istringstream maximum("200\n");
    askVehicleCount(interactive, maximum, prompt);
    assert(interactive.vehicles == 200);
    std::istringstream defaultAnswer("\n");
    askVehicleCount(interactive, defaultAnswer, prompt);
    assert(interactive.vehicles == 200);
    interactive.vehicles = 20;
    interactive.starts.assign(20, {1, 0});
    std::istringstream manual("30\n20\n");
    askVehicleCount(interactive, manual, prompt);
    assert(interactive.vehicles == 20 && interactive.starts.size() == 20);
    std::istringstream empty;
    rejected = false;
    try { askVehicleCount(interactive, empty, prompt); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    config.vehicles = 20;
    config.starts.assign(20, {1, 50}); // salida, no entrada
    rejected = false;
    try { planRoutes(config); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    config.starts.assign(20, {49, 0});
    config.destinations.assign(20, {45, 3});
    auto turning = planRoutes(config);
    for (const auto& route : turning.routes) {
        assert(route.size() == 12);
        assert((route[5] == Point{49, 5})); // gira al norte antes del extremo este
        assert((route[9] == Point{45, 5})); // gira al oeste hacia el destino
        assert((route.back() == Point{45, 3}));
    }
    config.destinations.assign(20, {49, 50});
    rejected = false;
    try { planRoutes(config); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected); // llegar al borde no constituye un destino
    config.destinations.assign(20, {3, 3});
    rejected = false;
    try { planRoutes(config); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected); // tampoco se puede terminar dentro de una manzana
    config.starts.clear();
    config.destinations.clear();
    for (unsigned seed : {1u, 17u, 42u, 123u}) {
        config.seed = seed;
        auto planned = planRoutes(config);
        for (std::size_t id = 0; id < planned.routes.size(); ++id) {
            const auto& route = planned.routes[id];
            assert(route.front() == planned.trips[id].start);
            assert(route.back() == planned.trips[id].destination);
            for (std::size_t i = 1; i < route.size(); ++i) {
                Point p = route[i];
                assert(p.r > 0 && p.r < H-1 && p.c > 0 && p.c < W-1);
                auto next = successors(route[i-1]);
                bool legal = false;
                for (Point q : next) if (q == p) legal = true;
                assert(legal);
            }
        }
    }
    std::cout << "OK: sentidos, caminos minimos, bordes, ciclo de luces y configuracion invalida\n";
}