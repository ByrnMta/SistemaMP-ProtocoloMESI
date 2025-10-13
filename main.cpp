#include "PE.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>

// Para compilar: g++ -std=c++11 -pthread PE.cpp main.cpp -o test_pe
// ./test_pe


int main() {
    const int num_pes = 1;
    std::vector<std::unique_ptr<PE>> pes;
    std::vector<std::vector<std::string>> instrucciones_pes(num_pes);

    // Programa de prueba: bucle con salto (JNZ) y contador REG3
    /*std::vector<std::string> programa = {
        "LOAD REG3, 0x3",   // REG3 = 3 (contador de iteraciones)
        "LOAD REG0, 0x0",   // REG0 = 0 (acumulador)
        "LOOP:",
        "INC REG0",         // REG0++
        "DEC REG3",         // REG3--
        "JNZ LOOP",         // Si REG3 != 0, salta a LOOP
        "STORE REG0, 0x1000"// Simula un store del acumulador
    };*/

    std::vector<std::string> programa = {
        "INC REG0",         // REG0 = 1
        "INC REG0",         // REG0 = 2
        "INC REG1",         // REG1 = 1
        "FMUL REG2, REG0, REG1", // REG2 = REG0 * REG1 = 2
        "FADD REG3, REG2, REG0", // REG3 = REG2 + REG0 = 4
        "DEC REG3",         // REG3 = 3
        "LOOP:",
        "DEC REG3",         // REG3--
        "JNZ LOOP"          // Bucle hasta que REG3 == 0
    };

    // Personaliza el valor inicial de REG3 para cada PE (diferente número de iteraciones)
    for (int i = 0; i < num_pes; ++i) {
        instrucciones_pes[i] = programa;
        // Modifica la instrucción de carga de REG3 para cada PE
        //instrucciones_pes[i][0] = "LOAD REG3, 0x" + std::to_string(i + 2); // REG3 = 2,3,4,5
    }

    // Crea y lanza los PEs
    for (int i = 0; i < num_pes; ++i) {
        pes.push_back(std::make_unique<PE>(i));
        pes[i]->load_instructions(instrucciones_pes[i]);
        pes[i]->start();
    }

    // Espera a que todos terminen
    for (int i = 0; i < num_pes; ++i) {
        pes[i]->join();
    }

    std::cout << "Prueba de 4 PEs con saltos y bucles finalizada.\n";
    return 0;
}