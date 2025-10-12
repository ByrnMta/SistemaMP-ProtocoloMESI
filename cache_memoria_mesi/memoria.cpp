#include "include/memoria.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

Memoria::Memoria() {
	memoria_principal.fill(0.0);
    string nombre_archivo_inicializar_memoria = "mem.mif";
    inicializar_memoria(nombre_archivo_inicializar_memoria);
}

double Memoria::leer(size_t direccion) const {
	if (direccion >= size_memoria) {
		throw out_of_range("Direccion de memoria fuera de rango");
	}
	return memoria_principal[direccion];
}

void Memoria::escribir(size_t direccion, double valor) {
	if (direccion >= size_memoria) {
		throw out_of_range("Direccion de memoria fuera de rango");
	}
	memoria_principal[direccion] = valor;
}

// Se inicializa por medio de un .mif
void Memoria::inicializar_memoria(string nombre_archivo_inicializar_memoria) {

    // Se abre el archivo MIF
    ifstream file(nombre_archivo_inicializar_memoria);

    // Verificar si el archivo se abrió correctamente
    if (!file.is_open()) {
        cerr << "[Memoria] ERROR: Cannot open MIF file: " << nombre_archivo_inicializar_memoria << endl;
        return;
    }
    string line;
    bool content_started = false;

    // Se lee el contenido del archivo
    while (getline(file, line)) {

        // Se busca el inicio del contenido
        if (line.find("CONTENT BEGIN") != string::npos) {
            content_started = true;
            continue;
        }

        // Se busca el final del contenido
        if (line.find("END") != string::npos) {
            break;
        }

        // Si no se ha comenzado el contenido, se continua a la siguiente línea
        if (!content_started) {
            continue;
        }

        // Se procesa la línea para extraer dirección y dato
        stringstream ss(line);
        string addr_str, colon, data_str;

        // Se extraen los componentes de la línea
        ss >> addr_str >> colon >> data_str;

        if (colon != ":") {
            continue;
        }

        // Se verifica que las cadenas no estén vacías
        if (addr_str.empty() || data_str.empty()) {
            continue;
        }

        // Se convierten las cadenas a los tipos adecuados y se almacenan en memoria
        size_t addr = stoul(addr_str, nullptr, 16);
        double data = stod(data_str); // decimal
        memoria_principal[addr] = data;
    }

    file.close();
}
