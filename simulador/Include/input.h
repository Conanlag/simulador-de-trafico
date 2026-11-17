#pragma once

enum class Key { NONE, SPACE, UP, DOWN, LEFT, RIGHT, QUIT };

// Pone la terminal en modo "crudo": sin eco y sin esperar Enter.
void initInput();

// Devuelve la terminal a como estaba antes de initInput().
void restoreInput();

// Espera hasta timeoutMs por una tecla (timeoutMs < 0 = esperar sin limite).
// Devuelve Key::NONE si se agota el tiempo sin que llegue ninguna tecla.
Key readKey(int timeoutMs);