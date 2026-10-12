// medicion.cpp - Costo con y sin protección (Parte B, punto 4).
//
// Escenario "con pausas": las mismas transferencias del experimento
//                         (25 por thread, pausa aleatoria de hasta 1 ms).
// Escenario "sin pausas": muchas transferencias seguidas (20 000 por thread)
//                         para ver el costo propio de tomar y soltar el mutex.
//
// Solo es válido comparar tiempos de versiones que dan el resultado correcto,
// por eso también se cuenta cuántas corridas rompieron algún invariante.
//
// Uso:  ./bin/medicion
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <vector>

#include "cuentas.h"

struct Escenario { const char* nombre; int ops; int pausa_us; int reps; };

int main() {
    const Escenario escenarios[] = {
        {"con pausas", 25, 1000, 20},
        {"sin pausas", 20000, 0, 10},
    };
    mkdir("resultados", 0755);
    std::ofstream csv("resultados/medicion_costo.csv");
    csv << "escenario,hilos,version,ops_por_hilo,reps,tiempo_prom_ms,desv_ms,transferencias_por_s,corridas_con_falla\n";

    std::printf("%-11s %5s %-10s %17s %10s %11s %19s\n", "escenario", "hilos", "version",
                "tiempo prom (ms)", "desv (ms)", "transf/s", "corridas con falla");
    for (const auto& e : escenarios) {
        for (int hilos : {2, 4}) {
            for (bool seguro : {false, true}) {
                std::vector<double> t;
                int fallas = 0;
                for (int rep = 0; rep < e.reps; ++rep) {
                    ResultadoCorrida r = ejecutar_corrida(seguro, hilos, e.ops, e.pausa_us, false, 7000 + rep);
                    t.push_back(r.tiempo_ms);
                    if (!r.errores.empty()) ++fallas;
                }
                double prom = 0, desv = 0;
                for (double x : t) prom += x;
                prom /= t.size();
                for (double x : t) desv += (x - prom) * (x - prom);
                desv = t.size() > 1 ? std::sqrt(desv / (t.size() - 1)) : 0.0;
                double tps = hilos * e.ops / (prom / 1000.0);
                const char* version = seguro ? "con mutex" : "sin mutex";
                std::printf("%-11s %5d %-10s %17.2f %10.2f %11.0f %15d/%d\n", e.nombre, hilos, version,
                            prom, desv, tps, fallas, e.reps);
                csv << e.nombre << ',' << hilos << ',' << version << ',' << e.ops << ',' << e.reps << ','
                    << prom << ',' << desv << ',' << tps << ',' << fallas << '\n';
            }
        }
    }
    std::printf("\nCSV guardado en: resultados/medicion_costo.csv\n");
    return 0;
}
