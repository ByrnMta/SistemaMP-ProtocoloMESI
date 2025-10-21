
#include "PE/PE.h"
#include "cache/include/Cache.h"
#include "Interconnect/interconnect.h"
#include "MESI/connect_mesi.h"
#include "cache/include/memoria.h"
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

vector<string> leer_programa_asm(const string& filename) {
    vector<string> instrucciones;
    ifstream file(filename);
    string linea;
    while (getline(file, linea)) {
        size_t pos = linea.find('@');
        if (pos != string::npos) linea = linea.substr(0, pos);
        // Elimina espacios y líneas vacías
        if (!linea.empty() && linea.find_first_not_of(" \t\r\n") != string::npos)
            instrucciones.push_back(linea);
    }
    cout << "[Main] Programa ensamblador leído desde " << filename << " con " << instrucciones.size() << " instrucciones.\n";   
    return instrucciones;
}

int main() {
    const int num_pes = 4;
    const int N = 9; // tamaño total de los vectores
    const int segmento_base = N / num_pes;
    vector<int> segmentos(num_pes, segmento_base);
    segmentos[num_pes - 1] += N % num_pes;
    int dir_A = 0, dir_B = 0, dir_sumas = 0, dir_resultado = 0;
    cout << "AQUÍ PASÓ" << endl;
    // Leer el programa ensamblador
    vector<string> programa_completo = leer_programa_asm("productoP.asm");

    // Extraer direcciones base desde el .asm (solo del bloque inicial)
    for (const auto& instr : programa_completo) {
        if (instr.find("LOADI REG0,") != string::npos) {
            string val = instr.substr(instr.find(",") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            dir_A = (val.find("0x") == 0) ? stoi(val, nullptr, 16) : stoi(val);
            cout << "[Main] Dirección base A: " << dir_A << endl;
        } else if (instr.find("LOADI REG1,") != string::npos) {
            string val = instr.substr(instr.find(",") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            dir_B = (val.find("0x") == 0) ? stoi(val, nullptr, 16) : stoi(val);
        } else if (instr.find("LOADI REG2,") != string::npos) {
            string val = instr.substr(instr.find(",") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            dir_sumas = (val.find("0x") == 0) ? stoi(val, nullptr, 16) : stoi(val);
        }
        // Solo hasta el primer MOV REG0, REG2
        if (instr.find("MOV REG0, REG2") != string::npos) break;
    }

   // Imprimir vector programa_completo
    cout << "\n--- Programa Ensamblador Completo ---\n";
    for (const auto& instr : programa_completo) {
        cout << instr << endl;
    }
    cout << "--------------------------------------\n";


    // Divide el programa en dos partes: suma parcial y lógica de espera/suma final
    vector<string> bloque_suma_parcial, bloque_final;
    bool en_final = false;
    for (const auto& instr : programa_completo) {
        if (instr.find("MOV REG0, REG2") != string::npos) en_final = true;
        if (!en_final)
            bloque_suma_parcial.push_back(instr);
        else
            bloque_final.push_back(instr);
    }

    // Imprimie el bloque de suma parcial y el bloque final
    cout << "\n--- Bloque Suma Parcial ---\n";
    for (const auto& instr : bloque_suma_parcial) {
        cout << instr << endl;
    }
    cout << "---------------------------\n";

    cout << "\n--- Bloque Final ---\n";
    for (const auto& instr : bloque_final) {
        cout << instr << endl;
    }
    cout << "---------------------\n";

    // Extraer solo la penúltima dirección LOADI REG1 del bloque_final (resultado final)
    int count = 0;
    string penultima_val;
    for (const auto& instr : bloque_final) {
        if (instr.find("LOADI REG1,") != string::npos) {
            penultima_val = instr.substr(instr.find(",") + 1);
            penultima_val.erase(0, penultima_val.find_first_not_of(" \t"));
            ++count;
        }
    }
    if (count > 0) {
        dir_resultado = (penultima_val.find("0x") == 0) ? stoi(penultima_val, nullptr, 16) : stoi(penultima_val);
        cout << "[Main] Dirección resultado final: " << dir_resultado << " (" << penultima_val << ")" << endl;
    }

    Memoria memoria_principal;
    Interconnect interconnect(num_pes, &memoria_principal, 16);
    vector<unique_ptr<Cache>> caches;
    vector<unique_ptr<PE>> pes;

    for (int i = 0; i < num_pes; ++i) {
        caches.push_back(make_unique<Cache>());
        pes.push_back(make_unique<PE>(i));
        pes[i]->set_cache(caches[i].get());
        pes[i]->set_interconnect(&interconnect);
    }
 
    vector<PE*> pes_ptrs;
    vector<Cache*> cache_ptrs;
    for (int i = 0; i < num_pes; ++i) {
        pes_ptrs.push_back(pes[i].get());
        cache_ptrs.push_back(caches[i].get());
    }
    connect_mesi_controllers(pes_ptrs, cache_ptrs, &interconnect);


    // Cargar instrucciones y registros en cada PE
    auto to_hex = [](int val) {
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::hex << val;
        return ss.str();
    };

    for (int i = 0; i < num_pes; ++i) {
        vector<string> programa_pe;
        for (const auto& instr : bloque_suma_parcial) {
            if (instr.find("LOADI REG0,") != string::npos) {
                programa_pe.push_back("LOADI REG0, " + to_hex(dir_A + i * segmento_base));
            } else if (instr.find("LOADI REG1,") != string::npos) {
                programa_pe.push_back("LOADI REG1, " + to_hex(dir_B + i * segmento_base));
            } else if (instr.find("LOADI REG2,") != string::npos) {
                programa_pe.push_back("LOADI REG2, " + to_hex(dir_sumas + i));
            } else if (instr.find("LOADI REG3,") != string::npos) {
                programa_pe.push_back("LOADI REG3, " + to_string(segmentos[i]));
            } else {
                programa_pe.push_back(instr);
            }
        }
        if (i == 0) {
            programa_pe.insert(programa_pe.end(), bloque_final.begin(), bloque_final.end());
        }
        pes[i]->load_instructions(programa_pe);
        // Imprimir por cada PE las instrucciones cargadas
        cout << "\n--- Instrucciones cargadas en PE " << i << " ---\n";
        for (const auto& instr : programa_pe) {
            cout << instr << endl;
        }
        cout << "--------------------------------------\n";
        pes[i]->print_registers();
    }

    // Ejecuta los PEs
    for (int i = 0; i < num_pes; ++i) pes[i]->start();
    for (int i = 0; i < num_pes; ++i) pes[i]->join();

    cout << "\n--- Resultado final de registros ---\n";
    for (int i = 0; i < num_pes; ++i) pes[i]->print_registers();

    return 0;
}