# Makefile - Laboratorio Semana 6 (Concurrencia y threads)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
COMUN     = src/cuentas.cpp src/experimento.cpp
HEADERS   = src/cuentas.h src/experimento.h

PROGRAMAS = bin/parte_a bin/parte_b bin/medicion bin/test_invariantes

all: $(PROGRAMAS)

bin:
	mkdir -p bin

bin/parte_a: src/parte_a.cpp $(COMUN) $(HEADERS) | bin
	$(CXX) $(CXXFLAGS) -o $@ src/parte_a.cpp $(COMUN)

bin/parte_b: src/parte_b.cpp $(COMUN) $(HEADERS) | bin
	$(CXX) $(CXXFLAGS) -o $@ src/parte_b.cpp $(COMUN)

bin/medicion: src/medicion.cpp $(COMUN) $(HEADERS) | bin
	$(CXX) $(CXXFLAGS) -o $@ src/medicion.cpp $(COMUN)

bin/test_invariantes: tests/test_invariantes.cpp $(COMUN) $(HEADERS) | bin
	$(CXX) $(CXXFLAGS) -o $@ tests/test_invariantes.cpp $(COMUN)

# Ejecuta todo en orden
run: all
	./bin/parte_a
	./bin/parte_a --hilos 2
	./bin/parte_b
	./bin/parte_b --hilos 2
	./bin/medicion
	./bin/test_invariantes

test: bin/test_invariantes
	./bin/test_invariantes

# La misma prueba contra la versión sin protección: debe FALLAR
test-inseguro: bin/test_invariantes
	-MODO=inseguro ./bin/test_invariantes

# Detector de carreras de datos (ThreadSanitizer): 1 corrida con 2 threads de cada parte.
# Se ejecuta con 'setarch -R' (sin aleatorizacion de memoria) porque en kernels nuevos
# ThreadSanitizer puede fallar al iniciar ("unexpected memory mapping").
TSANFLAGS = -std=c++17 -O1 -g -fsanitize=thread -pthread
tsan: | bin
	$(CXX) $(TSANFLAGS) -o bin/parte_a_tsan src/parte_a.cpp $(COMUN)
	$(CXX) $(TSANFLAGS) -o bin/parte_b_tsan src/parte_b.cpp $(COMUN)
	@cd bin && setarch $$(uname -m) -R ./parte_a_tsan --reps 1 --hilos 2 > /dev/null 2> tsan_a.txt; true
	@cd bin && setarch $$(uname -m) -R ./parte_b_tsan --reps 1 --hilos 2 > /dev/null 2> tsan_b.txt; true
	@echo "=== Parte A (sin proteccion) con ThreadSanitizer ==="
	@grep -m1 "FATAL" bin/tsan_a.txt; true
	@grep SUMMARY bin/tsan_a.txt | sort | uniq -c; true
	@echo "Avisos de data race en la Parte A: $$(grep -c 'WARNING: ThreadSanitizer' bin/tsan_a.txt)"
	@echo "=== Parte B (con mutex) con ThreadSanitizer ==="
	@grep -m1 "FATAL" bin/tsan_b.txt; true
	@echo "Avisos de data race en la Parte B: $$(grep -c 'WARNING: ThreadSanitizer' bin/tsan_b.txt)"

clean:
	rm -rf bin resultados

.PHONY: all run test test-inseguro tsan clean