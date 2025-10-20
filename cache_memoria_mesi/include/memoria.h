
#ifndef MEMORIA_H
#define MEMORIA_H


#include <cstddef>
#include <array>
#include <string>

using namespace std;

class Memoria {
private:
	static const size_t size_memoria = 512;
	array<double, size_memoria> memoria_principal;
	void inicializar_memoria(string nombre_archivo_inicializar_memoria);
public:
	Memoria();
	array<double, 4> read_bloque(size_t direccion) const;
	void write_bloque(size_t direccion, array<double, 4> bloque);
};
#endif // MEMORIA_H
