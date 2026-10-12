# Concurrencia – Carrera y reparación controlada (C++)

Sistemas Operativos - Tarea Concurrencia
Alumna: Bedregal Coaguila, Karla Miluska

Varios threads (`std::thread`) transfieren unidades entre dos saldos compartidos
(`saldo_a`, `saldo_b`) y actualizan un contador de transferencias.

- **Parte A:** sin protección y con pausas aleatorias entre lectura y escritura, para
  provocar condiciones de carrera. Se repite 100 veces y se registran las violaciones.
- **Parte B:** se protege la transición completa con `std::mutex` (`std::lock_guard`)
  y se repiten exactamente las mismas pruebas. También se mide el costo y se deja una
  prueba automatizada que falla si un invariante se rompe.

## Invariantes

| Código | Invariante |
|---|---|
| I1 | `saldo_a + saldo_b == 200` (el dinero no se crea ni se pierde) |
| I2 | ningún saldo es negativo en ningún momento (no hay sobregiro) |
| I3 | `contador ==` número de transferencias exitosas reportadas por los threads |

## Estructura

```
src/cuentas.h, src/cuentas.cpp        estado compartido, transferencia insegura/segura, invariantes
src/experimento.h, src/experimento.cpp repetición de pruebas, CSV y búsqueda del intercalado defectuoso
src/parte_a.cpp                        Parte A: provoca el fallo (sin mutex)
src/parte_b.cpp                        Parte B: misma prueba con mutex
src/medicion.cpp                       costo con y sin protección
tests/test_invariantes.cpp             prueba automatizada
Makefile
```

Los programas guardan sus resultados (CSV y trazas) en `resultados/`.

## Requisitos

Linux o WSL, `g++` con soporte de C++17 y `make`.

## Compilar y ejecutar

```bash
make                       # compila todo en bin/
./bin/parte_a              # Parte A: 4 threads, 100 repeticiones
./bin/parte_a --hilos 2    # Parte A con 2 threads
./bin/parte_b              # Parte B: misma prueba con mutex
./bin/parte_b --hilos 2
./bin/medicion             # costo con y sin protección
./bin/test_invariantes     # prueba automatizada (pasa)
MODO=inseguro ./bin/test_invariantes   # la misma prueba sin protección (falla)
make tsan                  # ThreadSanitizer: detecta las carreras de datos de la Parte A
make run                   # ejecuta todo en orden
```

Opciones de `parte_a` y `parte_b`: `--hilos N`, `--reps N`, `--ops N`, `--pausa MICROSEGUNDOS`, `--semilla N`.

## Resultados obtenidos

| Prueba | Corridas con invariante roto |
|---|---|
| Parte A, 4 threads | 100/100 |
| Parte A, 2 threads | 100/100 |
| Parte B, 4 threads | 0/100 |
| Parte B, 2 threads | 0/100 |

Los números exactos cambian en cada ejecución y en cada computadora, porque el orden en que
el planificador intercala los threads no es determinista.
