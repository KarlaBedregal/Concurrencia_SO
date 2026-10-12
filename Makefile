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

# Detector de carreras de datos (ThreadSanitizer): 1 corrida con 2 threads de cada parte
TSANFLAGS = -std=c++17 -O1 -g -fsanitize=thread -pthread
tsan: | bin
	$(CXX) $(TSANFLAGS) -o bin/parte_a_tsan src/parte_a.cpp $(COMUN)
	$(CXX) $(TSANFLAGS) -o bin/parte_b_tsan src/parte_b.cpp $(COMUN)
	@echo "=== Parte A (sin proteccion) con ThreadSanitizer ==="
	@cd bin && ./parte_a_tsan --reps 1 --hilos 2 2>&1 >/dev/null | grep SUMMARY | sort | uniq -c; true
	@cd bin && n=$$(./parte_a_tsan --reps 1 --hilos 2 2>&1 >/dev/null | grep -c "WARNING: ThreadSanitizer"); echo "Avisos de data race en la Parte A: $$n"
	@echo "=== Parte B (con mutex) con ThreadSanitizer ==="
	@cd bin && n=$$(./parte_b_tsan --reps 1 --hilos 2 2>&1 >/dev/null | grep -c "WARNING: ThreadSanitizer"); echo "Avisos de data race en la Parte B: $$n"

clean:
	rm -rf bin resultados

.PHONY: all run test test-inseguro tsan clean
