#!/usr/bin/env python3
"""Comprueba el dialogo inicial y los controles reales mediante una terminal PTY."""
import os
import pathlib
import pty
import re
import select
import subprocess
import termios
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
master, slave = pty.openpty()
original = termios.tcgetattr(slave)
process = subprocess.Popen([str(ROOT / 'simulador/src/calles')],
                           stdin=slave, stdout=slave, stderr=slave, close_fds=True)
data = b''


def until(predicate, timeout=10):
    global data
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate(data):
            return
        ready, _, _ = select.select([master], [], [], 0.05)
        if ready:
            data += os.read(master, 65536)
    raise AssertionError('No se recibio la pantalla esperada')


try:
    until(lambda d: b'Cantidad de vehiculos' in d)
    os.write(master, b'abc\n19\n201\n20\n')
    until(lambda d: b'Tick: 0' in d)
    assert b'numero entero' in data and b'cantidad debe estar entre 20 y 200' in data
    assert b'Se iniciaran 20 hilos de vehiculos y 20 hilos de semaforos' in data
    os.write(master, b' ')
    until(lambda d: b'PASO A PASO' in d)
    paused_tick = int(re.findall(rb'Tick: (\d+)', data)[-1])
    start = len(data)
    os.write(master, b'\x1b[C')
    until(lambda d: (b'Tick: ' + str(paused_tick + 1).encode()) in d[start:])
    os.write(master, b'q')
    until(lambda d: b'Simulacion incompleta' in d)
    process.wait(timeout=10)
    assert process.returncode == 2
    assert b'RESULTADOS DE LA SIMULACION' in data
    assert b'| Hilos de vehiculos' in data and b'Pendiente' in data
    assert b'viaje en curso' in data.lower() and b'COMPARACION FINAL' in data
    assert termios.tcgetattr(slave) == original
    (ROOT / 'pruebas/evidencias/interaccion.txt').write_text(
        'OK: pregunta inicial; rechaza texto, 19 y 201; acepta 20 vehiculos.\n'
        'OK: se anuncian 20 hilos de vehiculos y 20 de semaforos.\n'
        'OK: espacio pausa; flecha derecha avanza un paso; Q devuelve codigo 2.\n'
        'OK: tablas finales con pendientes y tiempos en curso; terminal restaurada.\n')
    print('OK: cantidad inicial, validacion, pausa, avance, tablas y restauracion de terminal')
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
    os.close(master)
    os.close(slave)