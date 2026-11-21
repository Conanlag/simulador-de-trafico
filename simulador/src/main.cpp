#include "config.h"
#include "input.h"
#include "render.h"
#include "rutas.h"
#include "simulacion.h"
#include <chrono>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

static volatile std::sig_atomic_t interrupted = 0;
static void onSignal(int) { interrupted = 1; }

// RAII restaura la terminal incluso si falla la simulacion.
struct Terminal {
    bool enabled;
    explicit Terminal(bool enable) : enabled(enable) {
        if (enabled) { initScreen(); initInput(); }
    }
    ~Terminal() { if (enabled) { restoreInput(); restoreScreen(); } }
};

int main(int argc, char** argv) {
    try {
        Config config;
        bool headless = !isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO);
        bool askAtStart = true;
        std::string output;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--headless") { headless = true; askAtStart = false; }
            else if (arg == "--config" && i + 1 < argc) config = loadConfig(argv[++i]);
            else if (arg == "--output" && i + 1 < argc) output = argv[++i];
            else if (arg == "--help") {
                std::cout << "Uso: calles [--config archivo] [--headless] [--output prefijo_csv]\n"
                          << "Al iniciar se pregunta la cantidad de vehiculos (20 a 200).\n"
                          << "--headless omite la pregunta y usa la cantidad del archivo o el valor por defecto.\n"
                          << "ESPACIO pausa/reanuda; flechas avanzan un paso; Q termina.\n";
                return 0;
            } else throw std::runtime_error("Argumento invalido: " + arg);
        }
        if (askAtStart) askVehicleCount(config, std::cin, std::cout);
        validateConfig(config);
        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);
        Planning planning = planRoutes(config);
        Simulation sim(config, planning);
        bool running = true, paused = false;
        long ticks = 0;
        auto begin = std::chrono::steady_clock::now();
        {
            Terminal terminal(!headless);
            while (running && !interrupted && ticks < config.maxTicks && !sim.complete()) {
                bool advance = headless;
                if (!headless) {
                    Snapshot s = sim.snapshot();
                    draw(s.cars, s.lights, paused ? SimMode::TICK : SimMode::FLUID, s.tick);
                    std::cout << "\033[K" << "Llegaron: " << s.arrived << '/' << config.vehicles
                              << "   Esperando cruce: " << s.waiting << "   Tiempo simulado: "
                              << s.tick * config.stepMs / 1000.0 << " s\n" << std::flush;
                    Key key = readKey(paused ? 100 : config.frameMs);
                    if (key == Key::QUIT) running = false;
                    else if (key == Key::SPACE) paused = !paused;
                    else if (key == Key::UP || key == Key::DOWN || key == Key::LEFT || key == Key::RIGHT) {
                        paused = true;
                        advance = true;
                    } else if (key == Key::NONE && !paused) advance = true;
                }
                if (running && advance) { sim.step(); ++ticks; }
            }
        }
        sim.stop();
        sim.report(std::cout);
        std::cout << "Duracion real de la ejecucion: "
                  << std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count() << " s.\n";
        if (!output.empty()) sim.writeCsv(output);
        if (!sim.complete()) {
            std::cerr << "Simulacion incompleta: interrupcion, salida manual o limite de ticks.\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
