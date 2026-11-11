#include "render.h"
#include "mapa.h"
#include <iostream>
#include <string>
using namespace std;

void initScreen() {
    cout << "\033[2J\033[?25l" << flush;   // limpiar pantalla y ocultar cursor
}

void restoreScreen() {
    cout << "\033[?25h\n" << flush;         // mostrar cursor
}

void draw(const vector<Car>& cars) {
    vector<vector<string>> cells(H, vector<string>(W));

    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) {
            if (isHStreet(r) && isVStreet(c))
                cells[r][c] = "\033[37m┼\033[0m";
            else if (isHStreet(r))
                cells[r][c] = "\033[90m─\033[0m";
            else if (isVStreet(c))
                cells[r][c] = "\033[90m│\033[0m";
            else
                cells[r][c] = "\033[32m▒\033[0m";   // manzanas / edificios
        }
    }

    for (const auto& car : cars)
        cells[car.r][car.c] = "\033[" + to_string(car.color) + "m●\033[0m";

    cout << "\033[H";   // cursor al inicio
    for (int r = 0; r < H; ++r) {
        for (int c = 0; c < W; ++c) cout << cells[r][c] << ' ';
        cout << '\n';
    }
    cout << "Ctrl+C para salir\n" << flush;
}