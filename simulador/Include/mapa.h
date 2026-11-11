#pragma once

// Configuración del mapa
const int BLOCKS = 12;                 // manzanas por lado (12 x 12)
const int H = BLOCKS * 2 + 1;          // alto en celdas (calles + manzanas)
const int W = BLOCKS * 2 + 1;          // ancho en celdas
const int STREET_ROW = 2;              // calle horizontal cada 2 filas
const int STREET_COL = 2;              // calle vertical cada 2 columnas

bool isHStreet(int r);
bool isVStreet(int c);
bool isStreet(int r, int c);
bool inBounds(int r, int c);