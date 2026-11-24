#!/usr/bin/env python3
"""Cinco escenarios con evidencias reales y validacion de CSV; solo biblioteca estandar."""
import csv
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
BINARY = ROOT / 'simulador/src/calles'
EVIDENCE = ROOT / 'pruebas/evidencias'
CONFIGS = ROOT / 'pruebas/configs'


def table_rows(text):
    return [[cell.strip() for cell in line.split('|')[1:-1]]
            for line in text.splitlines() if line.startswith('|')]


def table_row(text, label):
    return next(row for row in table_rows(text) if row[0] == label)


def run():
    EVIDENCE.mkdir(exist_ok=True)
    CONFIGS.mkdir(exist_ok=True)
    cases = [
        ('01_minimo', 20, 10, 5, 2, 6, 42, False),
        ('02_media', 100, 16, 2, 1, 3, 17, False),
        ('03_maximo', 200, 20, 5, 2, 6, 123, False),
        ('04_origen_compartido', 20, 16, 5, 2, 6, 8, True),
        ('05_rojo_largo', 40, 20, 1, 1, 12, 91, False),
    ]
    summary = []
    for name, count, lights, green, yellow, red, seed, manual in cases:
        config = (f'vehicles={count}\nlights={lights}\ngreen_seconds={green}\n'
                  f'yellow_seconds={yellow}\nred_seconds={red}\nstep_ms=150\n'
                  f'frame_ms=150\nmax_ticks=3000\nplanner_threads=4\nseed={seed}\n')
        if manual:
            config += 'starts=' + ';'.join(['49,0'] * count) + '\n'
            config += 'destinations=' + ';'.join(['45,3'] * count) + '\n'
        path = CONFIGS / (name + '.conf')
        path.write_text(config)
        args = [str(BINARY), '--headless', '--config', str(path), '--output', str(EVIDENCE / name)]
        result = subprocess.run(args, text=True, capture_output=True, timeout=180)
        (EVIDENCE / (name + '.txt')).write_text(result.stdout + result.stderr)
        assert result.returncode == 0, f'{name}: ejecucion incompleta; ver evidencia'
        assert table_row(result.stdout, 'Vehiculos que llegaron')[1] == f'{count} / {count}'
        assert table_row(result.stdout, 'Colisiones detectadas')[1] == '0'
        assert table_row(result.stdout, 'Hilos de vehiculos')[1] == str(count)
        with (EVIDENCE / (name + '_vehiculos.csv')).open() as file:
            rows = list(csv.DictReader(file))
        assert len(rows) == count
        assert all(r['llego'] == '1' for r in rows)
        assert sorted(int(r['orden']) for r in rows) == list(range(1, count + 1))
        assert all(float(r['espera_s']) <= float(r['viaje_s']) for r in rows)
        assert all(float(r['semaforo_s']) <= float(r['espera_s']) for r in rows)
        assert all((r['origen_r'], r['origen_c']) != (r['destino_r'], r['destino_c']) for r in rows)
        assert all(0 < int(r['destino_r']) < 50 and 0 < int(r['destino_c']) < 50 for r in rows)
        displayed = [r for r in table_rows(result.stdout) if len(r) == 9 and r[0].isdigit()]
        assert len(displayed) == count
        for shown, record in zip(displayed, rows):
            assert shown[0] == record['id'] and shown[1] == record['orden']
            for value, key in zip(shown[4:], ['viaje_s', 'espera_s', 'semaforo_s', 'semaforo_pct', 'detenido_pct']):
                assert abs(float(value) - float(record[key])) < 0.001
        with (EVIDENCE / (name + '_cruces.csv')).open() as file:
            crosses = list(csv.DictReader(file))
        assert len(crosses) == 169
        assert sum(int(c['semaforo']) for c in crosses) == lights
        assert len([r for r in table_rows(result.stdout) if len(r) == 4 and r[0].startswith('(')]) == 169
        if manual:
            assert all(int(r['pasos']) == 11 for r in rows)
            assert max(int(c['cola_maxima']) for c in crosses) >= 19
        timings = table_row(result.stdout, 'Rutas BFS')[1:]
        ticks = table_row(result.stdout, 'Pasos de simulacion')[1]
        average = table_row(result.stdout, 'Viaje promedio (s)')[1]
        summary.append([name, str(count), str(lights), ticks, *timings, average, '0'])
        print(f'OK {name}: {count}/{count} llegaron, sin colisiones', flush=True)
    with (EVIDENCE / 'comparacion.csv').open('w') as file:
        writer = csv.writer(file)
        writer.writerow(['prueba', 'vehiculos', 'semaforos', 'ticks', 'secuencial_ms', 'paralelo_ms', 'aceleracion', 'promedio_viaje_s', 'colisiones'])
        writer.writerows(summary)
    print('OK: cinco pruebas y evidencias guardadas')

if __name__ == '__main__':
    run()