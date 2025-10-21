#ifndef PE_H
#define PE_H

#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>
#include "cache/include/Cache.h"
#include "../MESI/MESIController.h"
#include "Interconnect/interconnect.h"

/**
 * @brief Clase que modela un Processing Element (PE) en un sistema multiprocesador.
 * 
 * Cada PE ejecuta instrucciones, accede a su caché privada y se comunica con otros componentes.
 */

// Estructura que representa una instrucción decodificada
struct Instruction {
    enum Type {
    LOAD, STORE, FMUL, FADD, INC, DEC, JNZ, LOADI, INVALID, MOV, CMPZ, JZ
    } type;
    int reg_dest = -1; // Registro destino
    int reg_src1 = -1; // Registro fuente 1
    int reg_src2 = -1; // Registro fuente 2
    int reg_cond = -1; // Registro de condición para JNZ/JZ
    int64_t address = 0; // Dirección de memoria (si aplica)
    int64_t immediate = 0; // Valor inmediato (para LOADI)
    std::string label; // Etiqueta (para saltos)
    // Para CMPZ/JZ
    bool cmp_zero_flag = false;
};

/**
 * @brief Clase que modela un Processing Element (PE) en un sistema multiprocesador.
 * 
 * Cada PE ejecuta instrucciones, accede a su caché privada y se comunica con otros componentes.
 */
class PE {
public:
    /**
     * @brief Constructor del PE.
     * @param id Identificador único del PE.
     */
    explicit PE(int id);

    /**
     * @brief Carga un conjunto de instrucciones para ejecutar.
     * @param instructions Vector de instrucciones (en formato string).
     */
    void load_instructions(const std::vector<std::string>& instructions);

    /**
     * @brief Inicia la ejecución del PE en un hilo separado.
     */
    void start();

    /**
     * @brief Espera a que el PE termine su ejecución.
     */
    void join();

    /**
     * @brief Devuelve el ID del PE.
     */
    int get_id() const;

    /**
     * @brief Imprime el estado actual de los registros del PE.
     */
    void print_registers() const;

    // ================================

    // Permite conectar el MESIController al PE
    void set_mesi_controller(class MESIController* mesi_controller) { mesi_controller_ = mesi_controller; }
    
    // Permite conectar la caché al PE
    void set_cache(class Cache* cache) { cache_ = cache; }

    // Permite conectar el interconnect al PE
    void set_interconnect(class Interconnect* interconnect) { interconnect_ = interconnect; }
    
    // ================================

private:
    /**
     * @brief Bucle principal de ejecución del PE.
     * Simula el ciclo fetch-decode-execute.
     */
    void run();

    /**
     * @brief Decodifica una instrucción en string a estructura Instruction.
     */
    Instruction parse_instruction(const std::string& instr_str);

    /**
     * @brief Ejecuta una instrucción decodificada.
     */
    void execute_instruction(const Instruction& instr);

    /**
     * @brief Procesa las etiquetas (labels) y construye el mapa de saltos.
     */
    void build_label_map();

    int id_;  // Identificador del PE
    std::vector<std::string> instructions_; // Instrucciones a ejecutar (en string)
    size_t pc_; // Program counter (índice de la instrucción actual)

    // Banco de registros (8 registros de 64 bits, REG0–REG7)
    double registers_[8] = {0};

    // Mapa de etiquetas a índice de instrucción
    std::unordered_map<std::string, size_t> label_map_;

    // Sincronización para ejecución en hilo
    std::thread thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool finished_;

    // ================================

    // Puntero a la caché privada (usado solo para debug o acceso directo, preferir MESIController)
    class Cache* cache_ = nullptr;

    // Puntero al MESIController
    class MESIController* mesi_controller_ = nullptr;

    // Puntero al interconnect
    class Interconnect* interconnect_ = nullptr;

    // ================================

};

#endif // PE_H