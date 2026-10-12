// cuentas.h
// Estado compartido del laboratorio de la Semana 6 (Sistemas Operativos - UNSA).
//
// Hay dos saldos compartidos (saldo_a y saldo_b) y un contador de transferencias.
// Varios threads mueven unidades de un saldo al otro al mismo tiempo.
//
//  - transferir_inseguro(): no usa ninguna primitiva  -> Parte A.
//  - transferir_seguro():   protege la transición completa con std::mutex -> Parte B.
//
// Las dos versiones ejecutan EXACTAMENTE el mismo código interno (transferir_base);
// la única diferencia es si ese código corre dentro del mutex o no.
#pragma once

#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <vector>

constexpr int SALDO_INICIAL_A = 100;
constexpr int SALDO_INICIAL_B = 100;
constexpr int TOTAL = SALDO_INICIAL_A + SALDO_INICIAL_B;  // el dinero no se crea ni se destruye

enum class Cuenta { A, B };
const char* nombre_cuenta(Cuenta c);

// Un evento de la traza: quién, qué, cuándo y sobre qué dato.
struct Evento {
    int64_t t_ns;        // tiempo monotónico (steady_clock) en nanosegundos
    int hilo;            // 1..N
    std::string accion;  // verifica, rechaza, lee, escribe
    std::string dato;    // saldo_a, saldo_b, contador
    int valor;
};

// Datos propios de cada thread (no se comparten).
struct ContextoHilo {
    int id;                      // 1..N
    std::mt19937 rng;            // generador aleatorio propio
    int max_pausa_us;            // pausa máxima entre leer y escribir
    bool registrar;              // guardar traza o no
    std::vector<Evento> traza;   // traza local: no necesita lock

    ContextoHilo(int id_, unsigned semilla, int max_pausa_us_, bool registrar_);
    void pausa();                // pausa aleatoria pequeña (solo varía intercalados)
    void log(const char* accion, const char* dato, int valor);
};

class Banco {
public:
    // Estado compartido. En la Parte A se accede sin protección a propósito.
    int saldo_a = SALDO_INICIAL_A;
    int saldo_b = SALDO_INICIAL_B;
    int contador = 0;            // transferencias realizadas con éxito

    bool transferir_inseguro(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto);
    bool transferir_seguro(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto);

    int saldo_minimo() const { return saldo_minimo_; }

private:
    bool transferir_base(ContextoHilo& h, Cuenta origen, Cuenta destino, int monto);
    void escribir(ContextoHilo& h, Cuenta c, int valor);
    int& ref(Cuenta c) { return c == Cuenta::A ? saldo_a : saldo_b; }

    std::mutex m_;                 // protege saldo_a, saldo_b y contador JUNTOS (Parte B)
    std::mutex m_medicion_;        // solo instrumentación: registro del saldo mínimo
    int saldo_minimo_ = SALDO_INICIAL_A < SALDO_INICIAL_B ? SALDO_INICIAL_A : SALDO_INICIAL_B;
};

// I1: saldo_a + saldo_b == TOTAL
// I2: ningún saldo fue negativo en ningún momento
// I3: contador == transferencias exitosas reportadas por los threads
std::vector<std::string> verificar_invariantes(const Banco& b, int exitosas);

struct ResultadoCorrida {
    int saldo_a = 0, saldo_b = 0, contador = 0, exitosas = 0, saldo_minimo = 0;
    std::vector<std::string> errores;
    double tiempo_ms = 0.0;
    std::vector<Evento> traza;     // trazas de todos los threads, ordenadas por tiempo
};

// Crea el banco, lanza n_hilos threads, espera con join y verifica los invariantes.
ResultadoCorrida ejecutar_corrida(bool seguro, int n_hilos, int ops, int max_pausa_us,
                                  bool registrar_traza, unsigned semilla);
