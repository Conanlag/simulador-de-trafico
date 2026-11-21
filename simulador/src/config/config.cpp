#include "config.h"
#include "mapa.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string trim(std::string s) {
    auto begin = s.find_first_not_of(" \t\r\n");
    return begin == std::string::npos ? "" : s.substr(begin, s.find_last_not_of(" \t\r\n") - begin + 1);
}

static std::vector<Point> points(const std::string& value) {
    std::vector<Point> result;
    std::istringstream list(value);
    std::string item;
    while (std::getline(list, item, ';')) {
        std::istringstream pair(item);
        Point p;
        char comma;
        if (!(pair >> p.r >> comma >> p.c) || comma != ',' || (pair >> std::ws && !pair.eof()))
            throw std::runtime_error("Coordenadas invalidas: " + item);
        result.push_back(p);
    }
    if (result.empty()) throw std::runtime_error("Lista de coordenadas vacia");
    return result;
}

Config loadConfig(const std::string& path) {
    Config c;
    std::ifstream file(path);
    if (!file) throw std::runtime_error("No se pudo abrir " + path);
    std::string line;
    int number = 0;
    while (std::getline(file, line)) {
        ++number;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) throw std::runtime_error("Falta = en linea " + std::to_string(number));
        std::string key = trim(line.substr(0, eq)), value = trim(line.substr(eq + 1));
        if (key == "starts") { c.starts = points(value); continue; }
        if (key == "destinations") { c.destinations = points(value); continue; }
        std::size_t used;
        double n = std::stod(value, &used);
        if (used != value.size() || !std::isfinite(n) || n <= 0 || n > 10000000)
            throw std::runtime_error("Valor invalido para " + key);
        if (key == "green_seconds") c.green = n;
        else if (key == "yellow_seconds") c.yellow = n;
        else if (key == "red_seconds") c.red = n;
        else {
            if (n != std::floor(n)) throw std::runtime_error("Se requiere entero para " + key);
            if (key == "vehicles") c.vehicles = static_cast<int>(n);
            else if (key == "lights") c.lights = static_cast<int>(n);
            else if (key == "step_ms") c.stepMs = static_cast<int>(n);
            else if (key == "frame_ms") c.frameMs = static_cast<int>(n);
            else if (key == "max_ticks") c.maxTicks = static_cast<int>(n);
            else if (key == "planner_threads") c.plannerThreads = static_cast<int>(n);
            else if (key == "seed") c.seed = static_cast<unsigned>(n);
            else throw std::runtime_error("Parametro desconocido: " + key);
        }
    }
    validateConfig(c);
    return c;
}

void validateConfig(const Config& c) {
    if (c.vehicles < 20 || c.vehicles > 200) throw std::runtime_error("vehicles debe estar entre 20 y 200");
    if (c.lights < 10 || c.lights > 20) throw std::runtime_error("lights debe estar entre 10 y 20");
    if (!std::isfinite(c.green) || !std::isfinite(c.yellow) || !std::isfinite(c.red) ||
        c.green <= 0 || c.yellow <= 0 || c.red <= 0 || c.green > 86400 || c.yellow > 86400 || c.red > 86400 ||
        c.stepMs <= 0 || c.frameMs <= 0 ||
        c.maxTicks <= 0 || c.plannerThreads < 1 || c.plannerThreads > 64)
        throw std::runtime_error("Tiempos positivos y planner_threads entre 1 y 64 requeridos");
    if ((!c.starts.empty() && c.starts.size() != static_cast<std::size_t>(c.vehicles)) ||
        (!c.destinations.empty() && c.destinations.size() != static_cast<std::size_t>(c.vehicles)))
        throw std::runtime_error("Cada lista manual debe tener vehicles coordenadas");
}

void askVehicleCount(Config& config, std::istream& input, std::ostream& output) {
    output << "\nCONFIGURACION INICIAL\n"
           << "Cada vehiculo tendra su propio hilo, origen en el borde, destino dentro de la ciudad y ruta automatica.\n";
    for (;;) {
        output << "Cantidad de vehiculos (20 a 200) [" << config.vehicles << "]: " << std::flush;
        std::string line;
        if (!std::getline(input, line)) throw std::runtime_error("No se recibio la cantidad de vehiculos");
        line = trim(line);
        int count = config.vehicles;
        if (!line.empty()) {
            std::istringstream value(line);
            if (!(value >> count) || (value >> std::ws && !value.eof())) {
                output << "Escriba un numero entero entre 20 y 200.\n";
                continue;
            }
        }
        if (count < 20 || count > 200) {
            output << "La cantidad debe estar entre 20 y 200.\n";
            continue;
        }
        if ((!config.starts.empty() && config.starts.size() != static_cast<std::size_t>(count)) ||
            (!config.destinations.empty() && config.destinations.size() != static_cast<std::size_t>(count))) {
            output << "Las listas manuales deben tener una coordenada por vehiculo. "
                   << "Use " << config.vehicles << " o modifique el archivo de configuracion.\n";
            continue;
        }
        config.vehicles = count;
        output << "Se iniciaran " << count << " hilos de vehiculos y " << config.lights << " hilos de semaforos.\n";
        return;
    }
}
