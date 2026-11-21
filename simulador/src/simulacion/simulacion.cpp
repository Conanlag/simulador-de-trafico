#include "simulacion.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

int Simulation::crossing(Point p) {
    return isIntersection(p.r, p.c) ? (p.r / PERIOD) * (BLOCKS + 1) + p.c / PERIOD : -1;
}

Simulation::Simulation(const Config& c, const Planning& p)
    : config(c), planning(p), vehicles(c.vehicles), lights(createTrafficLights(c)) {
    occupancy.fill(-1);
    reserved.fill(-1);
    crossingOwner.fill(-1);
    lightIndex.fill(-1);
    for (int i = 0; i < config.lights; ++i)
        lightIndex[crossing({lights[i].row, lights[i].col})] = i;
    try {
        for (int i = 0; i < config.lights; ++i) workers.emplace_back(&Simulation::worker, this, i, true);
        for (int i = 0; i < config.vehicles; ++i) workers.emplace_back(&Simulation::worker, this, i, false);
    } catch (...) {
        stop();
        throw;
    }
}

Simulation::~Simulation() { stop(); }

void Simulation::stop() {
    {
        std::lock_guard<std::mutex> lock(monitor);
        stopping = true;
    }
    changed.notify_all();
    for (auto& worker : workers) if (worker.joinable()) worker.join();
    workers.clear();
}

void Simulation::worker(int id, bool light) {
    long seen = 0;
    std::unique_lock<std::mutex> lock(monitor);
    while (true) {
        changed.wait(lock, [&] { return stopping || (light ? lightEpoch : carEpoch) != seen; });
        if (stopping) break;
        seen = light ? lightEpoch : carEpoch;
        if (light) {
            advanceLight(lights[id], config);
            --lightsPending;
        } else {
            move(id);
            --carsPending;
        }
        if ((light ? lightsPending : carsPending) == 0) changed.notify_all();
    }
    // Un mutex siempre se libera desde el mismo hilo que lo adquirio.
    if (!light) release(id);
}

bool Simulation::step() {
    std::unique_lock<std::mutex> lock(monitor);
    if (stopping || arrived == config.vehicles) return false;
    ++tick;
    queues.fill(0);
    lightsPending = config.lights;
    ++lightEpoch;
    changed.notify_all();
    changed.wait(lock, [&] { return lightsPending == 0 || stopping; });
    if (stopping) return false;
    carsPending = config.vehicles;
    ++carEpoch;
    changed.notify_all();
    changed.wait(lock, [&] { return carsPending == 0 || stopping; });
    verify();
    return arrived < config.vehicles;
}

void Simulation::release(int id) {
    auto& v = vehicles[id];
    for (int cell : v.reservations) reserved[cell] = -1;
    v.reservations.clear();
    if (v.crossing >= 0) {
        crossingOwner[v.crossing] = -1;
        crossingLocks[v.crossing].unlock();
        v.crossing = -1;
    }
}

void Simulation::wait(int id, int cross, bool red) {
    auto& v = vehicles[id];
    ++v.waitTicks;
    if (red) ++v.lightWaitTicks;
    else ++conflicts;
    if (cross >= 0) {
        ++queues[cross];
        peakQueues[cross] = std::max(peakQueues[cross], queues[cross]);
    }
}

void Simulation::move(int id) {
    auto& v = vehicles[id];
    if (v.arrived) return;
    const auto& route = planning.routes[id];
    // Los vehiculos con el mismo origen esperan fuera del mapa hasta poder entrar.
    if (!v.active) {
        Point p = route.front();
        int key = crossing(p);
        int light = key >= 0 ? lightIndex[key] : -1;
        if (light >= 0 && lights[light].state != LightColor::GREEN) { wait(id, key, true); return; }
        if (occupancy[cell(p)] != -1 || reserved[cell(p)] != -1) { wait(id, key, false); return; }
        // Todas las entradas del borde pertenecen a una interseccion exterior.
        // Se reserva el cruce y su salida antes de admitir el auto.
        if (key >= 0) {
            std::vector<int> cells;
            for (std::size_t i = 0; i < route.size(); ++i) {
                int k = cell(route[i]);
                if (occupancy[k] != -1 || reserved[k] != -1) { wait(id, key, false); return; }
                cells.push_back(k);
                if (crossing(route[i]) != key) break;
            }
            if (!crossingLocks[key].try_lock()) { wait(id, key, false); return; }
            v.crossing = key;
            crossingOwner[key] = id;
            v.reservations = cells;
            for (int k : cells) reserved[k] = id;
        }
        v.active = true;
        occupancy[cell(p)] = id;
        return;
    }
    Point current = route[v.position];
    Point next = route[v.position + 1];
    int key = crossing(next);
    if (key >= 0 && v.crossing != key) {
        int light = lightIndex[key];
        if (light >= 0 && lights[light].state != LightColor::GREEN) { wait(id, key, true); return; }
        // No se entra a un cruce si no hay sitio para completar la maniobra.
        // La reserva incluye la primera celda de salida: evita bloquear la caja.
        std::vector<int> cells;
        for (std::size_t i = v.position + 1; i < route.size(); ++i) {
            int k = cell(route[i]);
            if ((occupancy[k] != -1 && occupancy[k] != id) || (reserved[k] != -1 && reserved[k] != id)) {
                wait(id, key, false); return;
            }
            cells.push_back(k);
            if (crossing(route[i]) != key) break;
        }
        if (!crossingLocks[key].try_lock()) { wait(id, key, false); return; }
        v.crossing = key;
        crossingOwner[key] = id;
        v.reservations = cells;
        for (int k : cells) reserved[k] = id;
    }
    int target = cell(next);
    if ((occupancy[target] != -1 && occupancy[target] != id) || (reserved[target] != -1 && reserved[target] != id)) {
        wait(id, key, false); return;
    }
    occupancy[cell(current)] = -1;
    occupancy[target] = id;
    ++v.position;
    if (key < 0 && v.crossing >= 0) release(id);
    if (v.position + 1 == route.size()) {
        occupancy[target] = -1;
        release(id);
        v.active = false;
        v.arrived = true;
        v.arrivalTick = tick;
        arrivalOrder.push_back(id);
        ++arrived;
    }
}

void Simulation::verify() {
    std::array<int, CROSSINGS> count{};
    std::array<int, H * W> seen;
    seen.fill(-1);
    for (int id = 0; id < config.vehicles; ++id) {
        const auto& v = vehicles[id];
        if (!v.active) continue;
        Point p = planning.routes[id][v.position];
        int k = cell(p), key = crossing(p);
        ++checks;
        if (seen[k] != -1 || occupancy[k] != id || (key >= 0 && (++count[key] > 1 || crossingOwner[key] != id)))
            ++collisions;
        seen[k] = id;
    }
    if (collisions) throw std::runtime_error("Violacion de exclusion mutua detectada");
}

Snapshot Simulation::snapshot() {
    std::lock_guard<std::mutex> lock(monitor);
    Snapshot s;
    s.tick = tick;
    s.lights = lights;
    s.arrived = arrived;
    for (int q : queues) s.waiting += q;
    for (int id = 0; id < config.vehicles; ++id) {
        auto& v = vehicles[id];
        if (v.active) s.cars.push_back(carView(planning.routes[id][v.position], planning.routes[id][v.position + 1], id));
    }
    return s;
}

bool Simulation::complete() {
    std::lock_guard<std::mutex> lock(monitor);
    return arrived == config.vehicles;
}

namespace {
using Rows = std::vector<std::vector<std::string>>;

std::string decimal(double value) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(3) << value;
    return text.str();
}

std::string coordinate(Point p) {
    return "(" + std::to_string(p.r) + "," + std::to_string(p.c) + ")";
}

// Calcula anchos para mantener alineadas incluso las tablas de 200 vehiculos.
void table(std::ostream& out, const std::vector<std::string>& headers, const Rows& rows) {
    std::vector<std::size_t> widths;
    for (const auto& header : headers) widths.push_back(header.size());
    for (const auto& row : rows)
        for (std::size_t i = 0; i < headers.size(); ++i)
            widths[i] = std::max(widths[i], row[i].size());
    auto border = [&] {
        out << '+';
        for (auto width : widths) out << std::string(width + 2, '-') << '+';
        out << '\n';
    };
    auto line = [&](const std::vector<std::string>& row) {
        out << '|';
        for (std::size_t i = 0; i < row.size(); ++i)
            out << ' ' << row[i] << std::string(widths[i] - row[i].size() + 1, ' ') << '|';
        out << '\n';
    };
    border();
    line(headers);
    border();
    for (const auto& row : rows) line(row);
    border();
}
}

void Simulation::report(std::ostream& out) {
    std::lock_guard<std::mutex> lock(monitor);
    double seconds = config.stepMs / 1000.0;
    double total = 0;
    for (const auto& v : vehicles) if (v.arrived) total += v.arrivalTick * seconds;
    std::string first = "Ninguno todavia";
    if (!arrivalOrder.empty()) {
        int id = arrivalOrder.front();
        first = "Vehiculo " + std::to_string(id + 1) + " en " + decimal(vehicles[id].arrivalTick * seconds) + " s";
    }
    out << "\nRESULTADOS DE LA SIMULACION\n";
    table(out, {"Resumen", "Resultado"}, {
        {"Estado", arrived == config.vehicles ? "Completa" : "Incompleta"},
        {"Vehiculos que llegaron", std::to_string(arrived) + " / " + std::to_string(config.vehicles)},
        {"Hilos de vehiculos", std::to_string(config.vehicles)},
        {"Semaforos (un hilo por semaforo)", std::to_string(config.lights)},
        {"Primer vehiculo en llegar", first},
        {"Viaje promedio (s)", arrived ? decimal(total / arrived) : "Sin llegadas"},
        {"Duracion simulada (s)", decimal(tick * seconds)},
        {"Pasos de simulacion", std::to_string(tick)},
        {"Colisiones detectadas", std::to_string(collisions)},
        {"Verificaciones de seguridad", std::to_string(checks)},
        {"Esperas por trafico (intentos)", std::to_string(conflicts)}
    });
    out << "\nTIEMPOS POR VEHICULO\n"
        << "Tiempos en segundos simulados, desde el inicio comun; incluyen la cola de entrada.\n";
    Rows trips;
    for (int id = 0; id < config.vehicles; ++id) {
        const auto& v = vehicles[id];
        const auto& trip = planning.trips[id];
        long duration = v.arrived ? v.arrivalTick : tick;
        auto order = std::find(arrivalOrder.begin(), arrivalOrder.end(), id);
        trips.push_back({std::to_string(id + 1),
            v.arrived ? std::to_string(order - arrivalOrder.begin() + 1) : "Pendiente",
            coordinate(trip.start), coordinate(trip.destination),
            decimal(duration * seconds) + (v.arrived ? "" : "*"),
            decimal(v.waitTicks * seconds), decimal(v.lightWaitTicks * seconds),
            decimal(duration ? 100.0 * v.lightWaitTicks / duration : 0),
            decimal(duration ? 100.0 * v.waitTicks / duration : 0)});
    }
    table(out, {"Vehiculo", "Orden", "Origen", "Destino", "Viaje (s)", "Espera (s)",
                "Luz (s)", "Luz (%)", "Parado (%)"}, trips);
    out << "Orden: 1 = primero en llegar. Los empates de paso se ordenan por registro.\n"
        << "Espera: tiempo total detenido por trafico o luces.\n"
        << "Luz: espera solo por semaforos. Luz (%) = 100 x Luz (s) / Viaje (s).\n"
        << "Parado (%): porcentaje total del viaje sin avanzar.\n";
    if (arrived < config.vehicles)
        out << "* Viaje en curso: tiempo transcurrido, no una llegada. El promedio incluye solo llegados.\n";

    out << "\nCONGESTION POR INTERSECCION\n";
    Rows congestion;
    for (int k = 0; k < CROSSINGS; ++k) {
        Point p{k / (BLOCKS + 1) * PERIOD + STREET_WIDTH / 2,
                k % (BLOCKS + 1) * PERIOD + STREET_WIDTH / 2};
        congestion.push_back({coordinate(p), lightIndex[k] >= 0 ? "Si" : "No",
            std::to_string(queues[k]), std::to_string(peakQueues[k])});
    }
    table(out, {"Cruce (fila,col)", "Semaforo", "En espera al final", "Maximo en espera"}, congestion);
    out << "Las colas son cantidades de vehiculos por cruce, muestreadas en cada paso.\n"
        << "Al terminar todos los viajes, la espera final es cero; el maximo muestra la congestion observada.\n";

    out << "\nCOMPARACION FINAL: CALCULO DE LAS MISMAS RUTAS\n";
    table(out, {"Trabajo", "Secuencial (ms)", "Paralelo (ms)", "Aceleracion (x)"}, {
        {"Rutas BFS", decimal(planning.sequentialMs), decimal(planning.parallelMs),
         decimal(planning.sequentialMs / std::max(0.000001, planning.parallelMs))}
    });
    out << "BFS calcula la ruta mas corta. Paralelo usa " << config.plannerThreads << " hilos OpenMP.\n"
        << "Menos milisegundos es mejor. Aceleracion > 1 significa que el calculo paralelo fue mas rapido.\n"
        << "Esta comparacion mide la planificacion de rutas, no el trafico completo.\n";
}

void Simulation::writeCsv(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(monitor);
    std::ofstream trips(prefix + "_vehiculos.csv"), congestion(prefix + "_cruces.csv");
    if (!trips || !congestion) throw std::runtime_error("No se pueden escribir los CSV en " + prefix);
    trips << "id,origen_r,origen_c,destino_r,destino_c,pasos,llego,viaje_s,espera_s,semaforo_s,detenido_pct,semaforo_pct,orden\n";
    for (int id = 0; id < config.vehicles; ++id) {
        const auto& v = vehicles[id];
        auto t = planning.trips[id];
        long duration = v.arrived ? v.arrivalTick : tick;
        auto order = std::find(arrivalOrder.begin(), arrivalOrder.end(), id);
        trips << id+1 << ',' << t.start.r << ',' << t.start.c << ',' << t.destination.r << ',' << t.destination.c
              << ',' << planning.routes[id].size()-1 << ',' << v.arrived << ',' << duration*config.stepMs/1000.0
              << ',' << v.waitTicks*config.stepMs/1000.0 << ',' << v.lightWaitTicks*config.stepMs/1000.0
              << ',' << (duration ? 100.0*v.waitTicks/duration : 0) << ','
              << (duration ? 100.0*v.lightWaitTicks/duration : 0) << ','
              << (v.arrived ? std::to_string(order-arrivalOrder.begin()+1) : "") << '\n';
    }
    congestion << "fila,columna,espera_actual,cola_maxima,semaforo\n";
    for (int k = 0; k < CROSSINGS; ++k)
        congestion << k/(BLOCKS+1)*PERIOD+STREET_WIDTH/2 << ',' << k%(BLOCKS+1)*PERIOD+STREET_WIDTH/2
                   << ',' << queues[k] << ',' << peakQueues[k] << ',' << (lightIndex[k]>=0) << '\n';
}
