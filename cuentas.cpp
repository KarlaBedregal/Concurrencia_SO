// cuentas.cpp
#include "cuentas.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

using Reloj = std::chrono::steady_clock;

const char* nombre_cuenta(Cuenta c) { return c == Cuenta::A ? "saldo_a" : "saldo_b"; }

// ------------------------------------------------------------- ContextoHilo
ContextoHilo::ContextoHilo(int id_, unsigned semilla, int max_pausa_us_, bool registrar_)
    : id(id_), rng(semilla), max_pausa_us(max_pausa_us_), registrar(registrar_) {}

void ContextoHilo::pausa() {
    // Pausa pequeña aleatoria entre lectura y escritura.
    // Solo sirve para variar los intercalados, NO para imponer un orden.
    if (max_pausa_us <= 0) return;
    std::uniform_int_distribution<int> d(0, max_pausa_us);
    std::this_thread::sleep_for(std::chrono::microseconds(d(rng)));
}

void ContextoHilo::log(const char* accion, const char* dato, int valor) {
    if (!registrar) return;
    int64_t t = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    Reloj::now().time_since_epoch()).count();
    traza.push_back({t, id, accion, dato, valor});   // vector propio del thread
}

// ------------------------------------------------------------------- Banco
void Banco::escribir(ContextoHilo& h, Cuenta c, int valor) {
    ref(c) = valor;
    h.log("escribe", nombre_cuenta(c), valor);
    // m_medicion_ es SOLO instrumentación: hace confiable el registro del mínimo.
    // No protege la transferencia ni cambia el resultado.
    std::lock_guard<std::mutex> g(m_medicion_);
    if (valor < saldo_minimo_) saldo_minimo_ = valor;
}

bool Banco::transferir_base(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto) {
    // 1) verificar límite: ¿alcanza el saldo? (saldo >= monto)
    int s_verificado = ref(origen);
    h.log("verifica", nombre_cuenta(origen), s_verificado);
    if (s_verificado < monto) {
        h.log("rechaza", nombre_cuenta(origen), s_verificado);
        return false;
    }
    h.pausa();                                   // ventana entre verificar y actuar

    // 2) descontar del origen: leer, pausa, escribir
    int s_origen = ref(origen);
    h.log("lee", nombre_cuenta(origen), s_origen);
    h.pausa();
    escribir(h, origen, s_origen - monto);

    // 3) abonar al destino: leer, pausa, escribir
    int s_destino = ref(destino);
    h.log("lee", nombre_cuenta(destino), s_destino);
    h.pausa();
    escribir(h, destino, s_destino + monto);

    // 4) actualizar el contador: leer, pausa, escribir
    int c = contador;
    h.log("lee", "contador", c);
    h.pausa();
    contador = c + 1;
    h.log("escribe", "contador", c + 1);
    return true;
}

bool Banco::transferir_inseguro(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto) {
    // Parte A: sin protección, los pasos de varios threads se intercalan.
    return transferir_base(h, origen, destino, monto);
}

bool Banco::transferir_seguro(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto) {
    // Parte B: el mutex cubre la transición COMPLETA (verificar + descontar
    // + abonar + contar). lock_guard libera el mutex al salir del bloque,
    // incluso con return temprano o excepción (RAII).
    std::lock_guard<std::mutex> g(m_);
    return transferir_base(h, origen, destino, monto);
}

// ------------------------------------------------------------- invariantes
std::vector<std::string> verificar_invariantes(const Banco& b, int exitosas) {
    std::vector<std::string> errores;
    char buf[160];
    int suma = b.saldo_a + b.saldo_b;
    if (suma != TOTAL) {
        std::snprintf(buf, sizeof buf, "I1 suma total %d != %d", suma, TOTAL);
        errores.push_back(buf);
    }
    int minimo = std::min({b.saldo_minimo(), b.saldo_a, b.saldo_b});
    if (minimo < 0) {
        std::snprintf(buf, sizeof buf, "I2 sobregiro: algun saldo llego a %d", minimo);
        errores.push_back(buf);
    }
    if (b.contador != exitosas) {
        std::snprintf(buf, sizeof buf, "I3 contador %d != exitosas %d", b.contador, exitosas);
        errores.push_back(buf);
    }
    return errores;
}

// ------------------------------------------------------------- una corrida
static void trabajador(Banco& banco, bool seguro, ContextoHilo& h, int ops, int& resultado) {
    std::uniform_real_distribution<double> moneda(0.0, 1.0);
    std::uniform_int_distribution<int> montos(10, 40);
    int exitosas = 0;                            // variable local: no se comparte
    for (int i = 0; i < ops; ++i) {
        Cuenta origen = moneda(h.rng) < 0.5 ? Cuenta::A : Cuenta::B;
        Cuenta destino = origen == Cuenta::A ? Cuenta::B : Cuenta::A;
        int monto = montos(h.rng);
        bool ok = seguro ? banco.transferir_seguro(h, origen, destino, monto)
                         : banco.transferir_inseguro(h, origen, destino, monto);
        if (ok) ++exitosas;
    }
    resultado = exitosas;                        // cada thread escribe SU posición
}

ResultadoCorrida ejecutar_corrida(bool seguro, int n_hilos, int ops, int max_pausa_us,
                                  bool registrar_traza, unsigned semilla) {
    Banco banco;
    std::vector<ContextoHilo> ctx;
    ctx.reserve(n_hilos);
    for (int i = 0; i < n_hilos; ++i)
        ctx.emplace_back(i + 1, semilla + i, max_pausa_us, registrar_traza);
    std::vector<int> resultados(n_hilos, 0);

    auto inicio = Reloj::now();
    std::vector<std::thread> hilos;
    for (int i = 0; i < n_hilos; ++i)
        hilos.emplace_back(trabajador, std::ref(banco), seguro, std::ref(ctx[i]), ops,
                           std::ref(resultados[i]));
    for (auto& t : hilos) t.join();              // join: espera que terminen (no da exclusión)
    auto fin = Reloj::now();

    ResultadoCorrida r;
    for (int e : resultados) r.exitosas += e;
    r.saldo_a = banco.saldo_a;
    r.saldo_b = banco.saldo_b;
    r.contador = banco.contador;
    r.saldo_minimo = banco.saldo_minimo();
    r.errores = verificar_invariantes(banco, r.exitosas);
    r.tiempo_ms = std::chrono::duration<double, std::milli>(fin - inicio).count();
    if (registrar_traza) {
        for (auto& c : ctx) r.traza.insert(r.traza.end(), c.traza.begin(), c.traza.end());
        std::stable_sort(r.traza.begin(), r.traza.end(),
                         [](const Evento& x, const Evento& y) { return x.t_ns < y.t_ns; });
    }
    return r;
}
