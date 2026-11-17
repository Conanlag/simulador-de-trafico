#include "render.h"
#include "mapa.h"
#include <iostream>
#include <string>
using namespace std;

static string colorize(const string& code, const string& ch) {
    return "\033[" + code + "m" + ch + "\033[0m";
}

void initScreen() {
    cout << "\033[2J\033[?25l" << flush; // limpiar pantalla y ocultar cursor
}

void restoreScreen() {
    cout << "\033[?25h\n" << flush; // mostrar cursor de nuevo
}

// Celda de fondo: edificio (bloque solido) o calle (linea angosta).
static string backgroundCell(int r, int c) {
    bool hBand = isStreetBand(r);
    bool vBand = isStreetBand(c);

    if (!hBand && !vBand)
        return colorize("37", "█"); // manzana: bloque solido y parejo

    if (hBand && vBand)
        return colorize("90", "┼"); // interseccion (se tapa con el semaforo)

    if (hBand && isBandCenter(r))
        return colorize("90", streetDirection(r) > 0 ? "→" : "←");

    if (vBand && isBandCenter(c))
        return colorize("90", streetDirection(c) > 0 ? "↓" : "↑");

    if (hBand)
        return colorize("90", "─"); // calle horizontal: linea angosta

    return colorize("90", "│");     // calle vertical: linea angosta
}

static string lightGlyph(LightColor state) {
    switch (state) {
        case LightColor::GREEN:  return colorize("92", "■");
        case LightColor::YELLOW: return colorize("93", "■");
        case LightColor::RED:    return colorize("91", "■");
    }
    return colorize("92", "■");
}

// El auto se dibuja como una flecha que apunta hacia donde avanza.
static string carGlyph(const Car& car) {
    string arrow = "●";
    if (car.dr == -1)      arrow = "▲";
    else if (car.dr == 1)  arrow = "▼";
    else if (car.dc == -1) arrow = "◀";
    else if (car.dc == 1)  arrow = "▶";
    return colorize(to_string(car.color), arrow);
}

void draw(const vector<Car>& cars,
          const vector<TrafficLight>& lights,
          SimMode mode,
          long tick) {
    vector<vector<string>> cells(H, vector<string>(W));

    for (int r = 0; r < H; ++r)
        for (int c = 0; c < W; ++c)
            cells[r][c] = backgroundCell(r, c);

    for (const auto& light : lights)
        cells[light.row][light.col] = lightGlyph(light.state);

    for (const auto& car : cars)
        cells[car.r][car.c] = carGlyph(car);

    cout << "\033[H"; // cursor al inicio, sin limpiar (evita parpadeo)
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) cout << cells[r][c] << ' ';
        cout << "\033[K\n"; // borra restos de un frame anterior mas ancho
    }

    // Controles en solo 2 lineas, para que el cuadro completo nunca sea
    // mas alto que una terminal comun y no haga scroll en cada frame.
    cout << "\033[K" << "Tick: " << tick
         << "   Modo: " << (mode == SimMode::FLUID ? "FLUIDO" : "PASO A PASO") << '\n';
    cout << "\033[K"
         << "[ESPACIO] pausar/reanudar  ·  [FLECHAS] avanzar un tick  ·  [Q] salir\n";
    cout << flush;
}
