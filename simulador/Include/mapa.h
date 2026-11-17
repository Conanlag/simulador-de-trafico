#pragma once

// ---- Configuracion del mapa ----
// BLOCKS       : cuantas manzanas (cuadras) hay por lado de la ciudad
// BLOCK_SIZE   : ancho/alto en celdas de cada manzana (los edificios)
// STREET_WIDTH : ancho en celdas de cada calle; suficiente para ver unos
//                5 autos en fila dentro de una interseccion
constexpr int BLOCKS       = 12;
constexpr int BLOCK_SIZE   = 1;
constexpr int STREET_WIDTH = 3;
constexpr int PERIOD       = BLOCK_SIZE + STREET_WIDTH;

constexpr int H = BLOCKS * PERIOD + STREET_WIDTH; // alto de la grilla en celdas
constexpr int W = BLOCKS * PERIOD + STREET_WIDTH; // ancho de la grilla en celdas

bool inBounds(int r, int c);

// Una "banda" es la franja de celdas que ocupa una calle, ya sea en filas
// o en columnas segun el eje. x cae en una banda si esta dentro de los
// primeros STREET_WIDTH indices de cada periodo (manzana + calle).
bool isStreetBand(int x);

// true si la celda (r, c) es parte de alguna calle (horizontal o vertical)
bool isStreet(int r, int c);

// true si (r, c) es una interseccion: una calle horizontal cruzando una vertical
bool isIntersection(int r, int c);

// Primer indice de la banda de calle que contiene a x (solo tiene sentido
// si isStreetBand(x) es true)
int bandStart(int x);

// true si x cae justo en el centro de su banda de calle (ahi se dibuja la
// linea divisoria y, en un cruce, el semaforo)
bool isBandCenter(int x);

// Sentido de la calle que contiene a x: las bandas pares avanzan hacia
// derecha/abajo (+1) y las impares hacia izquierda/arriba (-1).
int streetDirection(int x);

// Carril central de una calle de un solo sentido.
int laneOffset(int dir);
