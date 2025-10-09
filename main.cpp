#include "PE.h"
#include <vector>
#include <string>
#include <iostream>

// Para compilar: g++ -std=c++11 -pthread PE.cpp main.cpp -o test_pe
// ./test_pe

int main() {
    // Crear un PE con ID 0
    PE pe0(0);

    // Instrucciones de prueba: manipulan registros y simulan operaciones
    std::vector<std::string> instrucciones = {
        "LOAD REG0, 0x10",      // REG0 = 16
        "INC REG0",             // REG0 = 17
        "LOAD REG1, 0x2",       // REG1 = 2
        "FMUL REG2, REG0, REG1",// REG2 = REG0 * REG1 = 34
        "FADD REG3, REG2, REG0",// REG3 = REG2 + REG0 = 51
        "DEC REG3",             // REG3 = 50
        "STORE REG3, 0x2000",   // Simula un store
        "JNZ LOOP"               // Simula un salto (no implementado)
    };
    pe0.load_instructions(instrucciones);

    // Iniciar y esperar a que termine
    pe0.start();
    pe0.join();

    std::cout << "Prueba de PE finalizada.\n";
    return 0;
}