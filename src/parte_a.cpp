// parte_a.cpp - Parte A: provocar el fallo.
// Varios threads transfieren unidades entre dos saldos compartidos y actualizan
// un contador SIN ninguna primitiva de sincronización. Entre cada lectura y su
// escritura hay una pausa aleatoria pequeña. Se repite al menos 100 veces y se
// registran las corridas que rompen la suma total o los límites.
//
// Uso:  ./bin/parte_a              (4 threads, 100 repeticiones)
//       ./bin/parte_a --hilos 2
#include "experimento.h"

int main(int argc, char** argv) {
    Parametros p = leer_parametros(argc, argv);
    correr_experimento(false, p, "parte_a_" + std::to_string(p.hilos) + "hilos");
    return 0;
}
