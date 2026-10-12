// parte_b.cpp - Parte B: demostrar la corrección.
// Se repiten EXACTAMENTE las mismas pruebas de la Parte A (mismos threads,
// mismas transferencias, mismas pausas y semillas), pero cada transferencia
// completa se ejecuta dentro de un std::mutex.
//
// Uso:  ./bin/parte_b              (4 threads, 100 repeticiones)
//       ./bin/parte_b --hilos 2
#include "experimento.h"

int main(int argc, char** argv) {
    Parametros p = leer_parametros(argc, argv);
    int fallas = correr_experimento(true, p, "parte_b_" + std::to_string(p.hilos) + "hilos");
    return fallas ? 1 : 0;   // código de salida distinto de 0 si algún invariante se rompió
}
