// experimento.cpp
#include "experimento.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sys/stat.h>

static void uso(const char* prog) {
    std::fprintf(stderr,
                 "Uso: %s [--hilos N] [--reps N] [--ops N] [--pausa MICROSEG] [--semilla N]\n", prog);
    std::exit(2);
}

Parametros leer_parametros(int argc, char** argv) {
    Parametros p;
    for (int i = 1; i < argc; ++i) {
        if (i + 1 >= argc) uso(argv[0]);
        int v = std::atoi(argv[i + 1]);
        if (std::strcmp(argv[i], "--hilos") == 0) p.hilos = v;
        else if (std::strcmp(argv[i], "--reps") == 0) p.reps = v;
        else if (std::strcmp(argv[i], "--ops") == 0) p.ops = v;
        else if (std::strcmp(argv[i], "--pausa") == 0) p.pausa_us = v;
        else if (std::strcmp(argv[i], "--semilla") == 0) p.semilla = static_cast<unsigned>(v);
        else uso(argv[0]);
        ++i;
    }
    if (p.hilos < 1 || p.reps < 1 || p.ops < 1 || p.pausa_us < 0) uso(argv[0]);
    return p;
}

std::vector<Evento> buscar_actualizacion_perdida(const std::vector<Evento>& traza) {
    std::map<std::string, std::pair<int, size_t>> leido_por;  // dato -> (hilo, posición)
    for (size_t i = 0; i < traza.size(); ++i) {
        const Evento& ev = traza[i];
        if (ev.accion == "lee") {
            auto it = leido_por.find(ev.dato);
            if (it != leido_por.end() && it->second.first != ev.hilo) {
                // otro thread leyó este dato y todavía no lo escribe
                std::set<int> hilos = {it->second.first, ev.hilo};
                std::vector<Evento> eventos;
                int escrituras = 0;
                for (size_t k = it->second.second; k < traza.size(); ++k) {
                    const Evento& e = traza[k];
                    if (e.dato == ev.dato && hilos.count(e.hilo)) {
                        eventos.push_back(e);
                        if (e.accion == "escribe" && ++escrituras == 2) return eventos;
                    }
                }
                return eventos;
            }
            leido_por[ev.dato] = {ev.hilo, i};
        } else if (ev.accion == "escribe" || ev.accion == "rechaza") {
            auto it = leido_por.find(ev.dato);
            if (it != leido_por.end() && it->second.first == ev.hilo) leido_por.erase(it);
        }
    }
    return {};
}

std::string formatear_eventos(const std::vector<Evento>& eventos) {
    if (eventos.empty()) return "  (no se encontro el patron en esta corrida)\n";
    std::string s;
    char buf[160];
    int64_t t0 = eventos.front().t_ns;
    for (const Evento& e : eventos) {
        std::snprintf(buf, sizeof buf, "  t=+%9.1f us  T%d  %-8s %-9s = %d\n",
                      (e.t_ns - t0) / 1000.0, e.hilo, e.accion.c_str(), e.dato.c_str(), e.valor);
        s += buf;
    }
    return s;
}

static std::string unir(const std::vector<std::string>& v, const char* sep) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) s += (i ? sep : "") + v[i];
    return s;
}

int correr_experimento(bool seguro, const Parametros& p, const std::string& nombre) {
    mkdir("resultados", 0755);
    std::printf("=== %s: %d threads, %d transferencias c/u, %d repeticiones, pausa max %d us ===\n",
                seguro ? "CON mutex (Parte B)" : "SIN proteccion (Parte A)",
                p.hilos, p.ops, p.reps, p.pausa_us);
    std::printf("Invariantes: I1 saldo_a+saldo_b=%d | I2 saldos>=0 | I3 contador=exitosas\n\n", TOTAL);

    std::ofstream csv("resultados/" + nombre + ".csv");
    csv << "rep,saldo_a,saldo_b,suma,contador,exitosas,saldo_minimo,tiempo_ms,estado,invariantes_rotos\n";

    int fallas = 0, i1 = 0, i2 = 0, i3 = 0, rep_guardada = 0;
    double tiempo_total = 0.0;
    ResultadoCorrida guardada;

    for (int rep = 1; rep <= p.reps; ++rep) {
        ResultadoCorrida r = ejecutar_corrida(seguro, p.hilos, p.ops, p.pausa_us, true,
                                              p.semilla + 100u * rep);
        tiempo_total += r.tiempo_ms;
        bool ok = r.errores.empty();
        if (!ok) {
            ++fallas;
            for (const auto& e : r.errores) {
                if (e.rfind("I1", 0) == 0) ++i1;
                if (e.rfind("I2", 0) == 0) ++i2;
                if (e.rfind("I3", 0) == 0) ++i3;
            }
            if (rep_guardada == 0) { rep_guardada = rep; guardada = r; }
        }
        std::printf("rep %3d: A=%5d B=%5d suma=%5d contador=%3d exitosas=%3d -> %s %s\n",
                    rep, r.saldo_a, r.saldo_b, r.saldo_a + r.saldo_b, r.contador, r.exitosas,
                    ok ? "OK" : "FALLA", unir(r.errores, "; ").c_str());
        csv << rep << ',' << r.saldo_a << ',' << r.saldo_b << ',' << r.saldo_a + r.saldo_b << ','
            << r.contador << ',' << r.exitosas << ',' << r.saldo_minimo << ',' << r.tiempo_ms << ','
            << (ok ? "OK" : "FALLA") << ",\"" << unir(r.errores, " | ") << "\"\n";
    }

    std::printf("\n--- Resumen ---\n");
    std::printf("Corridas con al menos un invariante roto: %d/%d\n", fallas, p.reps);
    std::printf("  I1 (suma total distinta de %d): %d corridas\n", TOTAL, i1);
    std::printf("  I2 (sobregiro: algun saldo fue negativo): %d corridas\n", i2);
    std::printf("  I3 (contador distinto de transferencias exitosas): %d corridas\n", i3);
    std::printf("Tiempo promedio por corrida: %.1f ms\n", tiempo_total / p.reps);
    std::printf("CSV guardado en: resultados/%s.csv\n", nombre.c_str());

    if (rep_guardada) {
        auto eventos = buscar_actualizacion_perdida(guardada.traza);
        char cab[400];
        std::snprintf(cab, sizeof cab,
                      "Intercalado defectuoso encontrado en la repeticion %d\n"
                      "Resultado final: A=%d B=%d contador=%d exitosas=%d\n"
                      "Invariantes rotos: %s\n\n"
                      "Eventos sobre el mismo dato (actualizacion perdida):\n",
                      rep_guardada, guardada.saldo_a, guardada.saldo_b, guardada.contador,
                      guardada.exitosas, unir(guardada.errores, "; ").c_str());
        std::string texto = std::string(cab) + formatear_eventos(eventos);
        std::string ruta = "resultados/" + nombre + "_intercalado.txt";
        std::ofstream f(ruta);
        f << texto << "\nTraza completa de esa corrida (tiempo_ns, hilo, accion, dato, valor):\n";
        for (const Evento& e : guardada.traza)
            f << e.t_ns << ", T" << e.hilo << ", " << e.accion << ", " << e.dato << ", " << e.valor << '\n';
        std::printf("\n%s", texto.c_str());
        std::printf("\nTraza completa guardada en: %s\n", ruta.c_str());
    }
    return fallas;
}
