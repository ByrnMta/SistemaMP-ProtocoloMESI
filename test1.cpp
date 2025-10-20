#include "PE/PE.h"
#include "cache/include/Cache.h"
#include "Interconnect/interconnect.h"
#include "MESI/connect_mesi.h"
#include "cache/include/memoria.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>

using namespace std;

int main() {
    // Configuración del sistema para 2 PEs
    const int num_pes = 2;
    Memoria memoria_principal;
    Interconnect interconnect(num_pes, &memoria_principal, 16);
    vector<unique_ptr<Cache>> caches;
    vector<unique_ptr<PE>> pes;

    // Crea PEs y sus cachés, y los conecta al interconnect
    for (int i = 0; i < num_pes; ++i) {
        caches.push_back(make_unique<Cache>());
        pes.push_back(make_unique<PE>(i));
        pes[i]->set_cache(caches[i].get());
        pes[i]->set_interconnect(&interconnect);
    }

    // Conecta MESIControllers a los PEs
    vector<PE*> pes_ptrs;
    vector<Cache*> cache_ptrs;
    for (int i = 0; i < num_pes; ++i) {
        pes_ptrs.push_back(pes[i].get());
        cache_ptrs.push_back(caches[i].get());
    }
    connect_mesi_controllers(pes_ptrs, cache_ptrs, &interconnect);

    // Inicializa datos en memoria (caché) SOLO en PE 1
    array<double, 4> datosA = {42.0, 0, 0, 0}; // Valor único para identificar el origen
    array<double, 4> datosB = {51.0, 0, 0, 0};

    caches[1]->write_linea_cache(0x00, datosA); // Solo PE 1 tiene la línea 0x00
    caches[1]->write_linea_cache(0x20, datosB); // Solo PE 1 tiene la línea 0x20

    // Programa: PE 0 intentará leer 0x20 (miss local, hit remoto en PE 1)
    
    vector<string> programa_pe0 = {
        "LOADI REG0, 0x60",        // REG0 = dirección a leer
        "LOAD REG1, [REG0]"        // REG1 = valor leído (debería venir de PE 1)
    };
    vector<string> programa_pe1 = {
        "LOADI REG0, 0x20",        // REG0 = dirección a leer
        "LOADI REG1, 22",          // REG1 = 22
        "STORE REG1, [REG0]",      // Almacena REG1 en la dirección de REG0
        "LOADI REG0, 0x00",        // REG0 = dirección a leer
        "LOAD REG1, [REG0]",       // REG1 = valor leído (debería ser local)
        "LOADI REG0, 0x40",        // REG0 = dirección a leer
        "LOAD REG1, [REG0]"        // REG1 = valor leído (debería ser local)
    };
    vector<string> programa_pe2 = {
        "LOADI REG0, 0x120",        // REG0 = dirección a leer
        "LOAD REG1, [REG0]"        // REG1 = valor leído (debería ser local)
    };

    // Carga y ejecuta el programa en cada PE
    pes[0]->load_instructions(programa_pe0);
    pes[1]->load_instructions(programa_pe1);
    //pes[2]->load_instructions(programa_pe2);
    pes[0]->start();
    pes[1]->start();
    //pes[2]->start();

    // Espera a que todos los PEs terminen
    pes[0]->join();
    pes[1]->join();
    //pes[2]->join();

    cout << "\n--- Resultado final de registros ---\n";
    for (int i = 0; i < num_pes; ++i) {
        pes[i]->print_registers();
    }
    return 0;
}