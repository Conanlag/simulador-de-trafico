# Pruebas y validación

Se ejecutó `make -C simulador/src test`. Cada escenario terminó con código 0;
se comprobaron las llegadas, el orden, las esperas, las 169 intersecciones y el
número configurado de semáforos. Las evidencias son salidas reales de consola y
CSV, en lugar de capturas de pantalla, como permite la consigna.

## Valores y propósito

| Prueba | Vehículos | Luces | Verde / amarillo / rojo (s) | Semilla | Validación |
|---|---:|---:|---|---:|---|
| 01 mínimo | 20 | 10 | 5 / 2 / 6 | 42 | Límites inferiores de configuración |
| 02 media | 100 | 16 | 2 / 1 / 3 | 17 | Mayor flujo y ciclos cortos |
| 03 máximo | 200 | 20 | 5 / 2 / 6 | 123 | Límites superiores y contención |
| 04 origen compartido | 20 | 16 | 5 / 2 / 6 | 8 | Todos parten de (49,0) y llegan a (45,3); cola externa y dos giros en 11 pasos |
| 05 rojo largo | 40 | 20 | 1 / 1 / 12 | 91 | Espera ante luces prolongadas |

En todas las pruebas, un paso representa 150 ms simulados, se usan cuatro hilos
OpenMP y el límite es 3000 pasos. El modo sin animación elimina la espera visual,
pero conserva semáforos, movimientos, hilos y métricas del mismo modelo.

## Resultados medidos

| Prueba | Llegaron | Pasos de reloj | BFS secuencial (ms) | BFS paralelo (ms) | Aceleración | Viaje promedio (s simulados) | Colisiones |
|---|---:|---:|---:|---:|---:|---:|---:|
| 01_minimo | 20/20 | 117 | 0.556 | 0.301 | 1.849 | 10.237 | 0 |
| 02_media | 100/100 | 179 | 1.496 | 0.542 | 2.761 | 13.306 | 0 |
| 03_maximo | 200/200 | 433 | 3.217 | 1.672 | 1.925 | 23.878 | 0 |
| 04_origen_compartido | 20/20 | 98 | 0.024 | 0.413 | 0.059 | 8.220 | 0 |
| 05_rojo_largo | 40/40 | 379 | 0.608 | 0.572 | 1.063 | 24.743 | 0 |

Son mediciones individuales de esta ejecución, no promedios estadísticos. Los
valores pueden variar con el equipo, la carga y el orden de los hilos. Una
aceleración menor que uno también es un resultado válido: para pocos viajes,
el costo de coordinación puede superar la ganancia del reparto.

## Evidencias por escenario

- **01_minimo**: [configuración](../pruebas/configs/01_minimo.conf), [consola](../pruebas/evidencias/01_minimo.txt), [vehículos](../pruebas/evidencias/01_minimo_vehiculos.csv), [cruces](../pruebas/evidencias/01_minimo_cruces.csv).
- **02_media**: [configuración](../pruebas/configs/02_media.conf), [consola](../pruebas/evidencias/02_media.txt), [vehículos](../pruebas/evidencias/02_media_vehiculos.csv), [cruces](../pruebas/evidencias/02_media_cruces.csv).
- **03_maximo**: [configuración](../pruebas/configs/03_maximo.conf), [consola](../pruebas/evidencias/03_maximo.txt), [vehículos](../pruebas/evidencias/03_maximo_vehiculos.csv), [cruces](../pruebas/evidencias/03_maximo_cruces.csv).
- **04_origen_compartido**: [configuración](../pruebas/configs/04_origen_compartido.conf), [consola](../pruebas/evidencias/04_origen_compartido.txt), [vehículos](../pruebas/evidencias/04_origen_compartido_vehiculos.csv), [cruces](../pruebas/evidencias/04_origen_compartido_cruces.csv).
- **05_rojo_largo**: [configuración](../pruebas/configs/05_rojo_largo.conf), [consola](../pruebas/evidencias/05_rojo_largo.txt), [vehículos](../pruebas/evidencias/05_rojo_largo_vehiculos.csv), [cruces](../pruebas/evidencias/05_rojo_largo_cruces.csv).

La tabla agregada se conserva en [comparacion.csv](../pruebas/evidencias/comparacion.csv).
La cuarta prueba comprueba además que la cola máxima incluya al menos 19 autos
esperando en el acceso compartido. Para cada viaje se valida que la espera en
semáforos no supere la espera total y que esta no supere el viaje. Los órdenes de
llegada deben ser una permutación completa de 1 a N, sin duplicados.

## Verificaciones del núcleo

`pruebas/test_core.cpp` comprueba las 13 calles y 13 avenidas en ambos sentidos
alternados, rutas rectas mínimas de 50 pasos, un desvío mínimo de 14 pasos, un giro
mínimo de 50 pasos, rechazo de coordenadas fuera de la rejilla, duración y orden
del ciclo completo de luces, rechazo de 19 vehículos y rechazo de un origen situado
en una salida. También verifica destinos interiores para varias semillas, rechazo
de destinos en el borde o dentro de una manzana y una ruta de 11 pasos desde
`(49,0)` a `(45,3)` con dos giros. El escenario de origen compartido ejecuta esta
ruta completa con 20 vehículos. El planificador comprueba además la igualdad de todas las rutas
secuenciales y paralelas de cada ejecución.

Después de cada paso se verifican la ocupación de celdas y la exclusividad de toda
la zona del cruce. Un fallo genera error. El contador de verificaciones y las
esperas por tráfico se encuentran en cada evidencia de consola. Cero colisiones
observadas no demuestra por sí solo ausencia de carreras: el argumento de
sincronización está en el reporte técnico. No se afirma validación con ThreadSanitizer.

## Repetir

```bash
make -C simulador/src test
```

Este comando actualiza los cinco archivos de configuración, las evidencias y la
tabla CSV. La tabla de este documento corresponde a la ejecución registrada;
si se repite, deben actualizarse sus cifras para mantener la misma referencia.
Para ejecutar solo un escenario:

```bash
./simulador/src/calles --headless --config pruebas/configs/03_maximo.conf --output /tmp/maximo
```

El límite de pasos o una cancelación devuelve código 2, y los autos pendientes
se muestran como tales. Una configuración inválida devuelve código 1. El script
rechaza cualquier ejecución incompleta y guarda su salida para inspección.

## Comprobaciones adicionales

Se compiló con AddressSanitizer y UndefinedBehaviorSanitizer y se ejecutó el caso
por defecto: 25/25 llegadas, cero colisiones y salida 0 sin diagnósticos de esos
instrumentos. [Evidencia y comandos](../pruebas/evidencias/sanitizadores.txt).
La detección de fugas de LeakSanitizer no estuvo disponible porque el entorno
instrumentado usa ptrace; esa comprobación se ejecutó con `detect_leaks=0`.
Esto no es una prueba de ausencia de carreras de datos.

Una terminal virtual PTY verificó la pregunta inicial, el rechazo de texto, 19 y
201, la elección de 20 vehículos con 20 semáforos, pausa con espacio, avance con
flecha, salida con Q, tablas de resultados incompletos y restauración exacta de
los atributos de terminal. Se puede repetir con `python3 pruebas/interaccion.py`.
Las pruebas del núcleo cubren también 200, Enter, fin de entrada y compatibilidad
con listas manuales. Los cinco escenarios contrastan las filas de las tablas de
viaje con los CSV y comprueban que se muestren las 169 intersecciones.
[Evidencia de interacción](../pruebas/evidencias/interaccion.txt).
[Entorno de ejecución](../pruebas/evidencias/entorno.txt).