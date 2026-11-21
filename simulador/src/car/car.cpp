#include "car.h"
Car carView(Point current, Point next, int id) {
    const int colors[] = {91, 93, 94, 95, 96, 97};
    return {current.r, current.c, next.r - current.r, next.c - current.c, colors[id % 6]};
}
