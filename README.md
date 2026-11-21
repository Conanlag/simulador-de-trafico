# Simulador concurrente de tráfico

Ciudad de 12 × 12 cuadras, con calles de un sentido alternado, rutas mínimas BFS,
un hilo por vehículo, un hilo por semáforo y cruces protegidos con mutex.
No se reciclan vehículos: terminan al llegar a su destino dentro de la ciudad. Alcanzar el borde
no cuenta como llegada; la ruta incluye los giros necesarios para continuar por las calles.

## Compilar y ejecutar

Requisitos: Linux/POSIX, GCC con C++17 y OpenMP, Make. Python 3 para las pruebas.
No se necesitan paquetes de Python.

```bash
make -C simulador/src
./simulador/src/calles --config simulador/config/default.conf
```

Desde `simulador/src` también se puede usar `make run` con valores por defecto.
Al inicio se pregunta cuántos vehículos se desean, entre **20 y 200**. Enter acepta
la cantidad sugerida; texto, decimales y números fuera del rango se vuelven a pedir.
Si el archivo contiene listas manuales, la cantidad debe coincidir con sus tamaños.
Controles: **espacio** pausa/reanuda, **flechas** avanzan un paso y **Q** termina.
Al finalizar se imprime la tabla de resultados. Una salida anticipada se identifica
como simulación incompleta y devuelve código 2; un error devuelve 1.

Para ejecutar sin animación y exportar métricas:

```bash
./simulador/src/calles --headless --config simulador/config/default.conf --output /tmp/trafico
```

Genera `/tmp/trafico_vehiculos.csv` y `/tmp/trafico_cruces.csv`. El directorio del
prefijo debe existir. `--headless` omite la pregunta inicial y toma la cantidad del
archivo o el valor por defecto. Sin terminal interactiva se omite la animación;
la pregunta sigue activa salvo que se indique `--headless`. `--help` muestra los argumentos.

Al terminar aparecen tablas alineadas con el resumen, cada viaje, la congestión de
las 169 intersecciones y la comparación secuencial/paralela del cálculo de rutas.
En la tabla de viajes, **Luz (s)** es la espera en semáforos y **Luz (%)** su
porcentaje del viaje. **Espera (s)** incluye todo el tiempo detenido y **Parado (%)**
su porcentaje. El orden 1 identifica al primero en llegar. Los tiempos son segundos
simulados; si se cancela, los pendientes se marcan y su tiempo lleva un asterisco.

## Configuración

Editar [default.conf](simulador/config/default.conf). Los comentarios empiezan con `#`.

| Clave | Valor inicial | Significado |
|---|---:|---|
| `vehicles` | 25 | Entre 20 y 200; un hilo por vehículo |
| `lights` | 20 | Entre 10 y 20 cruces principales con semáforo |
| `green_seconds` | 5 | Duración del verde |
| `yellow_seconds` | 2 | Duración del amarillo; prohíbe entrar |
| `red_seconds` | 6 | Duración del rojo; prohíbe entrar |
| `step_ms` | 150 | Tiempo **simulado** de un paso |
| `frame_ms` | 150 | Tiempo de visualización entre cuadros |
| `planner_threads` | 4 | Hilos OpenMP para planificación, entre 1 y 64 |
| `seed` | 42 | Semilla positiva para generar los viajes |
| `max_ticks` | 20000 | Límite para identificar ejecuciones incompletas |
| `starts` | aleatorio | Lista opcional `fila,columna;fila,columna;...` |
| `destinations` | aleatorio | Lista opcional con el mismo formato |

Las duraciones admiten decimales positivos hasta 86400 segundos. Se redondean hacia
arriba al número de pasos necesario. Cada lista manual debe contener tantas
coordenadas como vehículos. Se pueden repetir orígenes: los autos esperan fuera del
mapa antes de entrar. Si solo se proporciona una lista, la otra se genera al azar.

El mapa ocupa coordenadas de 0 a 50. Los carriles centrales son 1, 5, 9, …, 49.
Las calles C01…C13 se numeran de norte a sur; las avenidas A01…A13, de oeste a este.
C01 va al este, C02 al oeste, alternadamente. A01 va al sur, A02 al norte,
alternadamente. Los orígenes deben ser entradas del borde y los destinos puntos
de un carril central dentro del mapa (ambas coordenadas entre 1 y 49). Los destinos
aleatorios se eligen entre cruces de las calles interiores, accesibles desde todas
las entradas. Por ejemplo, desde `(49,0)` hasta `(45,3)` el auto
gira al norte en `(49,5)` y al oeste en `(45,5)`. Las configuraciones antiguas con
destinos en el borde deben actualizarse: ahora se rechazan. Para una lista completa funcional,
ver [el caso de origen compartido](pruebas/configs/04_origen_compartido.conf).

Las magnitudes `BLOCKS`, `BLOCK_SIZE` y `STREET_WIDTH` se definen en
[mapa.h](simulador/Include/mapa.h); el modelo validado usa 12, 1 y 3 respectivamente.

## Sincronización, resultados y alcance

Todos los vehículos tienen un destino y una ruta mínima calculada antes de arrancar.
Los semáforos tienen sus propios hilos. Un coordinador avanza el reloj lógico con
barreras mediante una variable de condición; los trabajadores duermen entre pasos.
Una pausa detiene también los semáforos y el tiempo simulado.

Cada intersección es una zona completa de 3 × 3 celdas con mutex exclusivo. Antes
de entrar se reserva el recorrido del cruce y la primera celda de salida. Un monitor
protege ocupaciones, reservas, luces y métricas. El dibujo recibe copias del estado.
Esto favorece la consistencia; los movimientos comparten un monitor y no se pretende
que ese tramo escale linealmente con el número de núcleos.

La tabla compara **planificación secuencial y paralela de los mismos viajes**, no dos
ejecuciones completas del tráfico. También informa tiempo real de ejecución,
tiempos simulados de viaje y espera, porcentaje detenido, orden de llegada y colas
por cruce. Se registra el tiempo desde el inicio común, incluida la espera para
entrar al mapa. Los empates se desempatan por orden de registro concurrente.

Los semáforos representan una autorización común para entrar al cruce; la exclusión
mutua resuelve los accesos de ambas calles. Quien ya está cruzando completa la
maniobra aunque cambie la luz. Los demás cruces también tienen mutex. Es un modelo
discreto con carril central, sin adelantamientos ni prioridades de emergencia.

Las rutas no se recalculan por congestión. No hay garantía de tiempo máximo de
llegada para cualquier combinación de tráfico; el límite de pasos informa el caso
como incompleto, sin inventar llegadas. Las cinco cargas documentadas terminaron.

## Pruebas y entregables

```bash
make -C simulador/src test
```

La prueba del núcleo verifica sentidos, distancias mínimas conocidas, bordes,
transiciones de luces y configuración inválida. Después se ejecutan cinco escenarios
completos y se actualizan sus evidencias de consola y CSV.

- [Reporte técnico](docs/REPORTE_TECNICO.md), con campos de equipo por completar.
- [Pruebas y validación](docs/PRUEBAS.md).
- [Evidencias de ejecución](pruebas/evidencias/).
- [Comparación de rendimiento](pruebas/evidencias/comparacion.csv).
