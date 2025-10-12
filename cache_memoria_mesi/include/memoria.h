
#ifndef MEMORIA_H
#define MEMORIA_H


#include <cstddef>
#include <array>
#include <string>

class Memoria {
private:
	static const size_t size_memoria = 512;
	std::array<double, size_memoria> memoria_principal;
	void inicializar_memoria(std::string nombre_archivo_inicializar_memoria);
public:
	Memoria();
	double leer(size_t direccion) const;
	void escribir(size_t direccion, double valor);
};

#endif // MEMORIA_H
