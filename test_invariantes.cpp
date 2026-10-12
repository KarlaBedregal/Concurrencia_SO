// test_invariantes.cpp - Prueba automatizada (Parte B, punto 6).
// FALLA (código de salida 1) si algún invariante se rompe.
//
//   ./bin/test_invariantes                  -> versión con mutex (pasa)
//   MODO=inseguro ./bin/test_invariantes    -> versión sin protección (falla)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../src/cuentas.h"

static int pasadas = 0, fallidas = 0;

static void comprobar(bool ok, const std::string& nombre, const std::string& detalle = "") {
    if (ok) {
        ++pasadas;
        std::printf("  [OK]     %s\n", nombre.c_str());
    } else {
        ++fallidas;
        std::printf("  [FALLA]  %s\n           %s\n", nombre.c_str(), detalle.c_str());
    }
}

static std::string unir(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) s += (i ? "; " : "") + v[i];
    return s;
}

// Repite la corrida y exige que NINGUNA rompa un invariante.
static void prueba_concurrencia(bool seguro, const char* modo, int hilos, int repeticiones) {
    std::string nombre = "invariantes con " + std::to_string(hilos) + " threads (" +
                         std::to_string(repeticiones) + " corridas)";
    for (int rep = 0; rep < repeticiones; ++rep) {
        ResultadoCorrida r = ejecutar_corrida(seguro, hilos, 25, 1000, false, 500 + 100 * rep);
        if (!r.errores.empty()) {
            comprobar(false, nombre, std::string("[") + modo + "] repeticion " +
                                     std::to_string(rep) + ": invariante roto -> " + unir(r.errores));
            return;
        }
    }
    comprobar(true, nombre);
}

static bool contiene(const std::vector<std::string>& errores, const char* codigo) {
    for (const auto& e : errores)
        if (e.rfind(codigo, 0) == 0) return true;
    return false;
}

int main() {
    const char* env = std::getenv("MODO");
    bool seguro = !(env && std::strcmp(env, "inseguro") == 0);
    const char* modo = seguro ? "seguro" : "inseguro";
    std::printf("Pruebas de invariantes (MODO=%s)\n", modo);

    prueba_concurrencia(seguro, modo, 2, 30);
    prueba_concurrencia(seguro, modo, 4, 30);

    // Sanidad: el verificador sí detecta estados rotos armados a mano.
    { Banco b; b.saldo_a += 30;
      comprobar(contiene(verificar_invariantes(b, 0), "I1"), "verificador detecta dinero creado (I1)"); }
    { Banco b; b.saldo_a = -10; b.saldo_b = TOTAL + 10;
      comprobar(contiene(verificar_invariantes(b, 0), "I2"), "verificador detecta saldo negativo (I2)"); }
    { Banco b; b.contador = 4;
      comprobar(contiene(verificar_invariantes(b, 5), "I3"), "verificador detecta contador inconsistente (I3)"); }

    std::printf("\nResultado: %d pasadas, %d fallidas\n", pasadas, fallidas);
    return fallidas ? 1 : 0;
}
