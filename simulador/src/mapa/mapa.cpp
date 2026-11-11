#include "mapa.h"
 
bool isHStreet(int r) { return r % STREET_ROW == 0; }
bool isVStreet(int c) { return c % STREET_COL == 0; }
bool isStreet(int r, int c) { return isHStreet(r) || isVStreet(c); }
bool inBounds(int r, int c) { return r >= 0 && r < H && c >= 0 && c < W; }