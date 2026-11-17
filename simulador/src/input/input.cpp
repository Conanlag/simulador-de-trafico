#include "input.h"
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

static termios originalTermios;

void initInput() {
    termios raw;
    tcgetattr(STDIN_FILENO, &originalTermios);
    raw = originalTermios;
    raw.c_lflag &= ~(ICANON | ECHO); // sin modo canonico, sin eco
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void restoreInput() {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
}

// Espera hasta timeoutMs a que haya algo para leer en stdin.
// timeoutMs < 0 espera indefinidamente.
static bool waitForInput(int timeoutMs) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeval tv;
    timeval* tvPtr = nullptr;
    if (timeoutMs >= 0) {
        tv.tv_sec  = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        tvPtr = &tv;
    }

    int ready = select(STDIN_FILENO + 1, &set, nullptr, nullptr, tvPtr);
    return ready > 0;
}

Key readKey(int timeoutMs) {
    if (!waitForInput(timeoutMs)) return Key::NONE;

    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return Key::NONE;

    if (c == ' ') return Key::SPACE;
    if (c == 'q' || c == 'Q') return Key::QUIT;

    if (c == '\033') { // posible flecha: secuencia ESC [ A/B/C/D
        if (!waitForInput(50)) return Key::NONE;
        char seq1;
        if (read(STDIN_FILENO, &seq1, 1) != 1 || seq1 != '[') return Key::NONE;

        if (!waitForInput(50)) return Key::NONE;
        char seq2;
        if (read(STDIN_FILENO, &seq2, 1) != 1) return Key::NONE;

        switch (seq2) {
            case 'A': return Key::UP;
            case 'B': return Key::DOWN;
            case 'C': return Key::RIGHT;
            case 'D': return Key::LEFT;
            default:  return Key::NONE;
        }
    }
    return Key::NONE;
}