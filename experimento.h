// experimento.h
// Funciones comunes para repetir la misma prueba muchas veces (Parte A y Parte B),
// guardar los resultados en CSV y reconstruir un intercalado defectuoso desde la traza.
#pragma once

#include <string>
#include <vector>

#include "cuentas.h"

struct Parametros {
    int hilos = 4;          // 2 o 4 threads
    int reps = 100;         // repeticiones (mínimo 100)
    int ops = 25;           // transferencias por thread
    int pausa_us = 1000;    // pausa máxima aleatoria entre leer y escribir (microsegundos)
    unsigned semilla = 2026;
};

Parametros leer_parametros(int argc, char** argv);

// Busca el primer caso en que dos threads leen el mismo dato antes de que el
// primero lo escriba (leer-leer-escribir-escribir) y devuelve esos eventos.
std::vector<Evento> buscar_actualizacion_perdida(const std::vector<Evento>& traza);

std::string formatear_eventos(const std::vector<Evento>& eventos);

// Ejecuta las repeticiones, imprime y guarda resultados/<nombre>.csv.
// Devuelve la cantidad de corridas con algún invariante roto.
int correr_experimento(bool seguro, const Parametros& p, const std::string& nombre);
