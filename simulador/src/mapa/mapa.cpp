#include "mapa.h"

bool inBounds(int r, int c) {
    return r >= 0 && r < H && c >= 0 && c < W;
}

bool isStreetBand(int x) {
    return (x % PERIOD) < STREET_WIDTH;
}

bool isStreet(int r, int c) {
    return isStreetBand(r) || isStreetBand(c);
}

bool isIntersection(int r, int c) {
    return isStreetBand(r) && isStreetBand(c);
}

int bandStart(int x) {
    return (x / PERIOD) * PERIOD;
}

bool isBandCenter(int x) {
    return isStreetBand(x) && (x % PERIOD) == STREET_WIDTH / 2;
}

int streetDirection(int x) {
    return (x / PERIOD) % 2 == 0 ? 1 : -1;
}

int laneOffset(int /*dir*/) {
    return STREET_WIDTH / 2;
}
