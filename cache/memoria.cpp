#include "include/memoria.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

Memoria::Memoria() {
	memoria_principal.fill(0.0);
    string nombre_archivo_inicializar_memoria = "cache/mem.mif";
    inicializar_memoria(nombre_archivo_inicializar_memoria);
}

array<double, 4> Memoria::read_bloque(size_t direccion) const {
	if (direccion >= size_memoria) {
		throw out_of_range("Direccion de memoria fuera de rango");
	}
	
    // Se obtiene el número de bloque (la memoria principal puede verse como 128 bloques de 4 datos cada uno)
    int num_bloque = direccion / 4;  // de 0 a 127

    // Se obtiene la dirección inicial o base del bloque a buscar
    int direccion_base_bloque = num_bloque * 4;

    array<double, 4> bloque_leido;

    // Se buscan los cuatro datos del bloque
	for (int i = 0; i < 4; ++i) {
		bloque_leido[i] = memoria_principal[direccion_base_bloque + i];
	}
	return bloque_leido;
}

void Memoria::write_bloque(size_t direccion, array<double, 4> bloque) {
	if (direccion >= size_memoria) {
		throw out_of_range("Direccion de memoria fuera de rango");
	}

    // Se obtiene el número de bloque (la memoria principal puede verse como 128 bloques de 4 datos cada uno)
    int num_bloque = direccion / 4;  // de 0 a 127

    // Se obtiene la dirección inicial o base del bloque a buscar
    int direccion_base_bloque = num_bloque * 4;

	// Se escriben los datos en el bloque (write-back)
	for (int i = 0; i < 4; ++i) {
		memoria_principal[direccion_base_bloque + i] = bloque[i];
	}
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
