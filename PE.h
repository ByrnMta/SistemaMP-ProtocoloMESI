#ifndef PE_H
#define PE_H

#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <unordered_map>

/**
 * @brief Clase que modela un Processing Element (PE) en un sistema multiprocesador.
 * 
 * Cada PE ejecuta instrucciones, accede a su caché privada y se comunica con otros componentes.
 */

// Estructura que representa una instrucción decodificada
struct Instruction {
    enum Type {
        LOAD, STORE, FMUL, FADD, INC, DEC, JNZ, LOADI, INVALID
    } type;
    int reg_dest = -1; // Registro destino
    int reg_src1 = -1; // Registro fuente 1
    int reg_src2 = -1; // Registro fuente 2
    int64_t address = 0; // Dirección de memoria (si aplica)
    int64_t immediate = 0; // Valor inmediato (para LOADI)
    std::string label; // Etiqueta (para saltos)
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
};

#endif // PE_H