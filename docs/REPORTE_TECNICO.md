# Reporte técnico: simulador concurrente de tráfico

## Objetivo general

Simular vehículos que recorren rutas mínimas dentro de una ciudad, coordinando
hilos independientes sobre intersecciones compartidas. Demostrar exclusión mutua,
consistencia del estado y medición del cálculo de rutas secuencial frente al paralelo.

## Descripción del problema

Un cruce puede recibir vehículos desde calles distintas. Si dos hilos comprueban
por separado que está libre y después lo ocupan, ambos podrían entrar. La
comprobación y la reserva deben formar una operación sincronizada. Además, los
semáforos, el dibujo y los vehículos acceden al mismo estado: sus lecturas y
escrituras requieren coordinación.

## Modelado del mapa

La ciudad tiene 12 × 12 cuadras. Con una celda por manzana y tres por banda de calle,
el período es cuatro y la rejilla mide 51 × 51 celdas. Hay 13 calles horizontales,
13 avenidas verticales, 169 intersecciones y 312 segmentos entre cruces adyacentes
(13 × 12 por eje). Los accesos externos conectan estos segmentos con el borde.

Las calles se llaman C01…C13, de norte a sur, y las avenidas A01…A13, de oeste a este.
C01 circula al este, C02 al oeste y así sucesivamente. A01 circula al sur, A02 al
norte y así sucesivamente. La función `streetDirection` aplica esta misma regla a
la generación de rutas y al dibujo. Los carriles centrales son 1, 5, 9, …, 49.

El grafo de planificación contiene las celdas de los carriles centrales. Cada arista
es un desplazamiento de una celda en el sentido permitido. En el centro de un cruce
se permite continuar o girar hacia el sentido de la calle perpendicular. Ninguna
arista abandona los límites del mapa. Un vehículo termina únicamente al llegar a
su destino interior. BFS incorpora los giros necesarios antes de un extremo sin
continuación; alcanzar el borde no cuenta como llegada. Nunca reaparece ni rebota.

## Vehículos y semáforos como hilos

Al inicio se pregunta una cantidad entera entre 20 y 200. Enter conserva la cantidad
sugerida; se rechazan las entradas inválidas. El modo explícito `--headless` omite
el diálogo y usa el archivo de configuración o los valores por defecto. Las listas
manuales deben coincidir con la cantidad elegida.

Se crean entre 20 y 200 hilos `std::thread` para vehículos y entre 10 y 20 para
semáforos (20 por defecto). Todos se crean antes de avanzar el reloj. Los estados del vehículo son:
pendiente de entrada, circulando, esperando y llegado. Su viaje incluye origen,
destino, ruta, posición, reservas y contadores de espera. Todos tienen la misma
prioridad; cada hilo recibe una oportunidad de actuar por paso. El sistema operativo
determina el orden dentro de ese paso.

Los semáforos se distribuyen en cruces principales mediante índices espaciados
sobre las 169 intersecciones. Cada hilo cambia su propio estado en el ciclo
verde → amarillo → rojo → verde. Los tiempos iniciales son 5, 2 y 6 segundos
simulados; se configuran desde archivo. Las fases iniciales se reparten entre los
tres colores para evitar que todas las luces cambien juntas.

El coordinador activa primero los hilos de semáforo, espera su barrera y después
activa los de vehículos. Las épocas del reloj y los contadores pendientes forman
las condiciones de las barreras. Cada trabajador espera en `condition_variable`
entre activaciones, sin espera activa. El modo sin animación ejecuta el mismo
modelo sin demoras de dibujo; `frame_ms` cambia la velocidad visual sin alterar
las duraciones simuladas. No se incluye el tiempo humano de pausa en los viajes.

## Sincronización y consistencia

Cada una de las 169 intersecciones tiene un mutex propio. Su zona crítica comprende
toda la banda de cruce, no solo la celda central. Un vehículo solicita ese mutex
con `try_lock` y lo conserva hasta abandonar el cruce o llegar al destino. Siempre
lo libera el mismo hilo que lo adquirió, incluso al cancelar la simulación.

Un monitor adicional protege el estado compuesto: mapa de ocupación, reservas,
propietarios de cruces, luces, posiciones y métricas. La consulta de luz y la reserva
se realizan dentro del monitor. El vehículo solo entra si hay verde (o no existe
semáforo), logra adquirir el mutex y puede reservar el trayecto de cruce con su
primera celda de salida. Amarillo y rojo impiden nuevas entradas. La reserva permite
completar una maniobra ya iniciada aunque cambie la luz.

Las celdas reservadas y ocupadas son exclusivas. Los autos con el mismo origen
esperan fuera del mapa; no se dibujan superpuestos. Un trabajador que no avanza
registra su espera y duerme hasta el siguiente paso lógico. Al llegar se elimina
la ocupación y se liberan reservas y mutex. No se generan autos de reemplazo.

El dibujado usa una instantánea construida bajo el monitor. Las señales de salida
solo modifican una bandera `sig_atomic_t`; el bucle normal restaura la terminal y
solicita el cierre. Al cerrar se notifican todos los trabajadores y se hace `join`.
Si falla la creación de un hilo, se cierran los ya creados.

### Argumento de ausencia de carreras y colisiones

1. El estado mutable del simulador solo se consulta o modifica con el monitor
   adquirido; las rutas y la configuración quedan inmutables durante la ejecución.
2. La comprobación de ocupación y su actualización se ejecutan sin soltar el monitor.
3. Solo un hilo puede adquirir el mutex de cada cruce y figura como su propietario.
4. Una celda reservada por un auto no puede ser ocupada por otro.
5. El trabajo OpenMP escribe en índices distintos del vector de rutas; cada BFS
   usa su propia cola y arreglo de padres. No hay generador aleatorio compartido.
6. Después de cada paso se verifican ocupaciones, ausencia de duplicados y que cada
   vehículo dentro de un cruce sea su único propietario. Una violación aborta la
   ejecución con error; no se oculta en un contador final.

La inspección de los mecanismos respalda el argumento de consistencia. Las pruebas
con cero colisiones son evidencia experimental adicional, no una demostración
exhaustiva de todos los intercalados posibles. El modelo no demuestra ausencia de
gridlock vial para cualquier demanda ni garantiza una cota de llegada. Para una
carga que no termine se informa el límite de pasos como ejecución incompleta.

## Rutas y camino más corto

Se usa BFS porque todas las aristas tienen costo uno. BFS visita los nodos por
niveles de distancia y registra el primer predecesor de cada nodo; por ello, al
reconstruir la cadena se obtiene una ruta de longitud mínima en el grafo dirigido.
Su complejidad es O(V + E) y su memoria O(V) por vehículo. Para costos variables
sería necesario emplear Dijkstra o A* con una heurística apropiada.

Los puntos aleatorios se generan con `mt19937` y una semilla configurable. Los
orígenes pertenecen a entradas del borde y los destinos a carriles interiores
entre cruces del núcleo interior, accesible desde todas las entradas. Los destinos
manuales también pueden estar en un cruce interior, pero nunca en el borde. Las
listas manuales se validan antes de crear los hilos. Se rechazan puntos inválidos
y destinos inalcanzables; no se elige un destino distinto de manera silenciosa.

## Paralelismo utilizado

Una ejecución de referencia calcula las rutas en un bucle secuencial. Otra usa
`#pragma omp parallel for`, reparto estático y un número configurable de hilos.
Ambas reciben exactamente los mismos pares origen/destino y emplean el mismo BFS.
Se comparan todas las rutas antes de comenzar la simulación.

La medición usa `steady_clock`, en milisegundos. Se precalienta el equipo de OpenMP
antes de las mediciones; la comparación corresponde al trabajo de planificación y
su distribución, sin incluir generación de viajes ni creación inicial del equipo.
La aceleración es tiempo secuencial / tiempo paralelo. No se supone que deba ser
mayor que uno en todas las cargas: los viajes cortos y la planificación pequeña
pueden verse dominados por costos de coordinación.

Los movimientos del tráfico usan hilos concurrentes con un monitor común. El
reporte no presenta el rendimiento de la planificación como si fuera aceleración
de toda la simulación, ni afirma movimiento paralelo sin contención.

## Métricas

| Métrica | Definición implementada |
|---|---|
| Viaje por vehículo | Desde el inicio común hasta la llegada, incluida la cola externa |
| Espera total | Pasos sin avanzar por acceso, tráfico o semáforo |
| Espera en luces | Pasos detenidos ante amarillo o rojo al solicitar entrada |
| Porcentaje detenido | 100 × pasos de espera / pasos transcurridos |
| Porcentaje por semáforo | 100 × pasos detenidos por luces / pasos transcurridos |
| Orden de llegada | Registro de llegadas; empates de tick por orden concurrente |
| Primero | Primer registro, tiempo simulado e identificación del vehículo |
| Promedio | Media del viaje entre vehículos que llegaron |
| Congestión | Autos que no lograron avanzar hacia cada cruce en un paso y máximo observado |
| Verificaciones | Estados activos revisados al final de cada paso |
| Colisiones | Violaciones de ocupación o exclusión mutua detectadas |
| Esperas por tráfico | Intentos bloqueados por ocupación, reserva o mutex, incluida entrada externa |
| Planificación | Milisegundos secuenciales, paralelos y aceleración |
| Tiempo real | Tiempo de pared de la simulación y su presentación; separado del simulado |

La congestión es una muestra por paso: un auto que esperó al principio del paso
puede haber encontrado espacio al final; no es una medición continua. Las esperas
por tráfico no se llaman «choques evitados»: varias esperas pueden corresponder al
mismo encuentro. Los CSV contienen cada viaje y las 169 intersecciones. Una
simulación cancelada marca los vehículos pendientes y no inventa tiempos de llegada.

## Pruebas y resultados

### Entorno y procedimiento experimental

Los resultados siguientes corresponden a las evidencias conservadas después del
cambio a destinos interiores. Se ejecutaron los cinco escenarios mediante
`make -C simulador/src test`, con Linux WSL2 6.6.87.2, arquitectura x86_64,
16 procesadores lógicos disponibles y GCC 13.3.0. La compilación utiliza C++17,
`-O2`, `-Wall`, `-Wextra`, `-fopenmp` y `-pthread`.

Cada escenario usa cuatro hilos OpenMP para planificar, pasos de 150 ms simulados
y un máximo de 3000 pasos. Se ejecuta con `--headless`: se omiten la animación y
su espera visual, pero se conservan los hilos, las luces y las reglas de movimiento.
Cada fila representa una ejecución; no se calcularon intervalos de confianza ni
promedios de múltiples repeticiones. La semilla fija los viajes, pero el orden de
los hilos puede modificar las esperas, las llegadas y los tiempos de pared.

| Escenario | Vehículos | Semáforos | Verde / amarillo / rojo (s) | Semilla |
| --- | --- | --- | --- | --- |
| 01_minimo | 20 | 10 | 5 / 2 / 6 | 42 |
| 02_media | 100 | 16 | 2 / 1 / 3 | 17 |
| 03_maximo | 200 | 20 | 5 / 2 / 6 | 123 |
| 04_origen_compartido | 20 | 16 | 5 / 2 / 6 | 8 |
| 05_rojo_largo | 40 | 20 | 1 / 1 / 12 | 91 |

Los escenarios cubren la carga mínima, una carga intermedia, el máximo de
vehículos, un acceso compartido y un ciclo con rojo prolongado. En el cuarto caso,
los 20 autos parten de `(49,0)` y tienen como destino `(45,3)`; los demás generan
sus viajes con la semilla indicada. Las configuraciones completas y los vínculos
a cada evidencia están en [PRUEBAS.md](PRUEBAS.md).

### Llegadas y duración de los viajes

| Escenario | Llegaron | Pasos | Duración simulada (s) | Viaje promedio (s) | Primero / tiempo (s) | Colisiones |
| --- | --- | --- | --- | --- | --- | --- |
| 01_minimo | 20/20 | 117 | 17.550 | 10.237 | V18 / 4.950 | 0 |
| 02_media | 100/100 | 179 | 26.850 | 13.306 | V61 / 1.950 | 0 |
| 03_maximo | 200/200 | 433 | 64.950 | 23.878 | V11 / 0.600 | 0 |
| 04_origen_compartido | 20/20 | 98 | 14.700 | 8.220 | V19 / 1.800 | 0 |
| 05_rojo_largo | 40/40 | 379 | 56.850 | 24.743 | V5 / 1.200 | 0 |

En conjunto terminaron los 380 viajes, sin alcanzar el límite de pasos y sin
colisiones detectadas. La duración simulada es el tiempo hasta la última llegada;
el promedio considera el tiempo de cada vehículo desde el inicio común, incluida
la cola externa. El primer vehículo se identifica por el registro de llegada; si
hay empates de paso, el orden depende de la ejecución concurrente.

Con 200 vehículos, la última llegada ocurrió a los 64.950 s simulados y el viaje
medio fue de 23.878 s. En el caso de rojo prolongado, 40 vehículos necesitaron
56.850 s para completar todos los viajes y su media fue de 24.743 s. Esto muestra
que la cantidad de autos por sí sola no determina la duración observada: también
intervienen el recorrido, las luces y la contención. Como cambian las semillas y
las configuraciones, estos escenarios no aíslan el efecto causal de cada factor.

### Esperas y congestión

Las medias siguientes se calcularon a partir de los CSV individuales. «Detenido
medio» es la media aritmética de los porcentajes de cada vehículo; no es el cociente
entre la suma de esperas y la suma de viajes. El cruce indicado es uno de los que
alcanzaron la cola máxima, pues puede haber empates.

| Escenario | Espera media (s) | Luz media (s) | Detenido medio (%) | Cola máxima (autos) | Cruce |
| --- | --- | --- | --- | --- | --- |
| 01_minimo | 3.038 | 1.522 | 25.161 | 2 | (21,45) |
| 02_media | 6.598 | 1.286 | 44.375 | 10 | (1,1) |
| 03_maximo | 16.975 | 3.391 | 61.892 | 17 | (1,1) |
| 04_origen_compartido | 6.420 | 0.000 | 68.844 | 19 | (49,1) |
| 05_rojo_largo | 18.172 | 10.706 | 63.394 | 3 | (1,1) |

La carga máxima acumuló una espera media de 16.975 s por vehículo y registró
esperas en 141 de las 169 intersecciones. En cambio, el escenario de rojo largo
presentó 10.706 s medios de espera por luces: esta componente fue mayor que en los
otros cuatro escenarios. La espera por luces forma parte de la espera total;
ambas columnas no deben sumarse.

El acceso compartido alcanzó una cola de 19 autos en `(49,1)`, coherente con admitir
un vehículo mientras los restantes esperan. Todos recorrieron la misma ruta de
11 pasos, pero sus llegadas abarcaron de 1.800 a 14.700 s, debido a la contención.
La espera media por semáforos fue cero en este recorrido; su espera total se debió
al acceso y al tráfico. En los cinco escenarios, las colas finales fueron cero
una vez completados todos los viajes. Esto no implica ausencia de congestión
durante la ejecución: esa información la conserva la cola máxima.

### Comparación secuencial y paralela de BFS

| Escenario | Secuencial (ms) | Paralelo (ms) | Aceleración (×) |
| --- | --- | --- | --- |
| 01_minimo | 0.556 | 0.301 | 1.849 |
| 02_media | 1.496 | 0.542 | 2.761 |
| 03_maximo | 3.217 | 1.672 | 1.925 |
| 04_origen_compartido | 0.024 | 0.413 | 0.059 |
| 05_rojo_largo | 0.608 | 0.572 | 1.063 |

La aceleración se calcula como `T_secuencial / T_paralelo` sobre los mismos viajes,
con cuatro hilos OpenMP. Antes de simular se comprueba la igualdad exacta de las
rutas producidas por ambas versiones. Los tiempos y la aceleración se redondean
por separado para presentarlos, por lo que dividir las cifras impresas puede dar
una pequeña diferencia respecto al cociente registrado.

En esta ejecución, la planificación paralela fue más rápida en cuatro escenarios.
La mayor aceleración registrada fue 2.761× en la carga intermedia; con 200 vehículos
fue 1.925×. El aumento de trabajo no produjo una aceleración creciente en todas las
cargas. La duración de las rutas, el reparto estático y el costo de coordinación
pueden influir; una sola medición no permite cuantificar por separado sus efectos.

En el acceso compartido, la versión secuencial tardó 0.024 ms y la paralela
0.413 ms, con aceleración de 0.059×. Son 20 rutas iguales y cortas: el resultado
es compatible con que el costo de distribuir y sincronizar el trabajo supere el
ahorro de ejecutar BFS en paralelo. El experimento no mide ese costo por separado.
La comparación corresponde exclusivamente a la planificación; no demuestra una
aceleración equivalente de toda la simulación ni de los movimientos de vehículos.

### Seguridad y tiempo real observado

| Escenario | Verificaciones de estados | Esperas por tráfico (intentos) | Tiempo real de ejecución (s) |
| --- | --- | --- | --- |
| 01_minimo | 1315 | 202 | 0.164722 |
| 02_media | 7045 | 3542 | 1.17697 |
| 03_maximo | 19182 | 18112 | 5.42583 |
| 04_origen_compartido | 228 | 856 | 0.138822 |
| 05_rojo_largo | 6189 | 1991 | 0.978307 |

Las cinco ejecuciones acumularon 33959 verificaciones de estados activos,
sin violaciones de ocupación o propiedad de cruces. El contador no representa
pasos de reloj: en cada paso pueden verificarse varios vehículos. Los intentos
bloqueados por tráfico tampoco representan autos distintos ni colisiones evitadas.

El tiempo real procede de la línea final de cada reporte de consola y está separado
del reloj simulado. Depende del equipo, la planificación de hilos y la generación
de la salida; las ejecuciones anteriores usan el modo sin animación. Estas cifras
no constituyen una comparación secuencial/paralela del simulador completo.

### Verificación de rutas, límites e interacción

Las pruebas del núcleo comprueban sentidos de circulación, distancias mínimas
conocidas, ciclo de luces y rechazo de configuraciones inválidas. También verifican
que las rutas generadas continúen dentro del mapa después de entrar, que cada
movimiento sea una arista permitida y que los destinos en el borde o en una manzana
se rechacen. Se revisan varias semillas.

La regresión de giros comprueba la ruta `(49,0) → (49,5) → (45,5) → (45,3)`:
el vehículo gira al norte y después al oeste antes de terminar en su destino.
El escenario de origen compartido ejecuta esa ruta con 20 hilos y verifica que
todos completen sus 11 movimientos. Así se comprueba el cambio de comportamiento:
alcanzar un límite del mapa no se registra como final del viaje.

El validador contrasta las filas de la consola con los CSV, comprueba todas las
llegadas y que su orden sea una permutación de 1 a N. Para cada vehículo exige
`espera en luces ≤ espera total ≤ tiempo de viaje`; además, comprueba que se
reporten las 169 intersecciones y la cantidad configurada de semáforos.

Una prueba con terminal virtual verifica el diálogo inicial, entradas inválidas,
pausa, avance de un paso, salida con Q y restauración de los atributos de terminal.
La cancelación devuelve código 2 y muestra pendientes, sin convertirlos en llegadas.
La evidencia está en [interaccion.txt](../pruebas/evidencias/interaccion.txt).

Existe también una [comprobación previa con sanitizadores](../pruebas/evidencias/sanitizadores.txt)
que no reportó errores de AddressSanitizer ni UndefinedBehaviorSanitizer. Esa
salida corresponde a una versión anterior con destinos en el borde y 16 semáforos;
no valida el cambio actual a destinos interiores. No se ejecutó ThreadSanitizer
y la detección de fugas se desactivó por la limitación de ptrace del entorno.

### Reportes generados y reproducción

La consola organiza el reporte en cuatro tablas: resumen de ejecución, tiempos por
vehículo, congestión de las intersecciones y comparación de BFS. El resumen permite
identificar si la simulación terminó; el detalle de viajes permite revisar origen,
destino, orden, tiempo y esperas; la tabla de cruces conserva las colas máximas.

Cada escenario conserva tres archivos con el mismo prefijo:

- `nombre.txt`: salida de consola y diagnósticos de la ejecución.
- `nombre_vehiculos.csv`: una fila por vehículo, con coordenadas, longitud de ruta,
  estado de llegada, viaje, esperas, porcentajes y orden.
- `nombre_cruces.csv`: una fila por intersección, con coordenadas, cola final,
  cola máxima e indicación de semáforo.

[comparacion.csv](../pruebas/evidencias/comparacion.csv) reúne los resultados de
los cinco escenarios. Por ejemplo, los detalles de la carga máxima están en
[consola](../pruebas/evidencias/03_maximo.txt),
[vehículos](../pruebas/evidencias/03_maximo_vehiculos.csv) y
[cruces](../pruebas/evidencias/03_maximo_cruces.csv).

Para repetir la batería completa:

```bash
make -C simulador/src test
```

Para ejecutar solo la carga máxima y exportar sus métricas sin sobrescribir las
evidencias conservadas:

```bash
./simulador/src/calles --headless --config pruebas/configs/03_maximo.conf --output /tmp/maximo
```

La batería completa actualiza las evidencias y el CSV agregado. Si se vuelve a
ejecutar, las tablas de este reporte y de `PRUEBAS.md` deben actualizarse con la
misma ejecución para evitar mezclar mediciones.

## Conclusiones técnicas y aprendizajes

La separación entre grafo inmutable y estado concurrente permite planificar rutas
en paralelo sin coordinar cada expansión de BFS. En el tráfico, en cambio, la
ocupación de un cruce exige una operación atómica sobre varios recursos. El monitor
simplifica ese contrato a costa de serializar las actualizaciones del estado.

El mutex del cruce y la reserva de salida evitan autorizar dos vehículos al mismo
tiempo y reducen el bloqueo dentro de la intersección. Una luz verde autoriza el
intento, pero no sustituye la exclusión mutua. La espera en una variable de condición
permite pausar, cerrar y coordinar pasos sin bucles de espera activa.

Los 380 viajes completados sin colisiones y la cola de 19 autos en el acceso
compartido respaldan el funcionamiento observado de la admisión y las reservas.
El caso de dos giros confirma que el viaje termina en el destino interior. Estas
pruebas cubren las cargas registradas; no garantizan ausencia de bloqueos viales
para cualquier configuración.

Los resultados distinguen tiempo simulado, tiempo de pared y costo de
planificación. OpenMP mejoró cuatro de las cinco mediciones de BFS, mientras que
el caso de rutas cortas favoreció la versión secuencial. Por tanto, el paralelismo
requiere suficiente trabajo para compensar su coordinación. Una mejora futura sería medir múltiples repeticiones de BFS,
introducir control de gridlock y reducir el alcance del monitor sin debilitar las
invariantes. También conviene repetir la instrumentación con sanitizadores sobre
la versión actual. Los nombres de integrantes, institución, docente y reflexiones
personales deben incorporarse con información del equipo; no se deducen de las
evidencias técnicas.
