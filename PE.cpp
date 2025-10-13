#include "PE.h"
#include <iostream>
#include <thread>
#include <sstream>

// Constructor: inicializa el PE con su ID y registros en cero
PE::PE(int id)
    : id_(id), pc_(0), finished_(false) {
    for (int i = 0; i < 8; ++i) registers_[i] = 0.0;
}

// Carga el vector de instrucciones (en string) y construye el mapa de etiquetas
void PE::load_instructions(const std::vector<std::string>& instructions) {
    std::lock_guard<std::mutex> lock(mtx_);
    instructions_ = instructions;
    pc_ = 0;
    build_label_map();
}

// Construye el mapa de etiquetas (labels) a índices de instrucción
void PE::build_label_map() {
    label_map_.clear();
    for (size_t i = 0; i < instructions_.size(); ++i) {
        std::string instr = instructions_[i];
        // Quita espacios al inicio
        instr.erase(0, instr.find_first_not_of(" \t"));
        // Si termina con ':' es una etiqueta
        if (!instr.empty() && instr.back() == ':') {
            std::string label = instr.substr(0, instr.size() - 1);
            label_map_[label] = i;
        }
    }
}

// Inicia la ejecución del PE en un hilo separado
void PE::start() {
    thread_ = std::thread(&PE::run, this);
}

// Espera a que el hilo termine
void PE::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

// Devuelve el ID del PE
int PE::get_id() const {
    return id_;
}

// Imprime el estado de los registros del PE
void PE::print_registers() const {
    std::cout << "[PE " << id_ << "] Estado de registros:\n";
    for (int i = 0; i < 8; ++i) {
        std::cout << "  REG" << i << ": " << registers_[i] << std::endl;
    }
}

// Bucle principal de ejecución del PE: fetch-decode-execute con soporte de saltos
void PE::run() {
    std::unique_lock<std::mutex> lock(mtx_);
    std::cout << "[PE " << id_ << "] Iniciando ejecución...\n";
    while (pc_ < instructions_.size()) {
        std::string instr_str = instructions_[pc_];
        // Si es una etiqueta, solo avanza
        std::string trimmed = instr_str;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (!trimmed.empty() && trimmed.back() == ':') {
            ++pc_;
            continue;
        }
        Instruction instr = parse_instruction(instr_str);
        std::cout << "[PE " << id_ << "] Ejecutando: " << instr_str << std::endl;
        // Lógica de salto para JNZ
        if (instr.type == Instruction::JNZ) {
            // Por convención, REG3 es el registro de control de bucle
            int reg_cond = 3;
            if (reg_cond >= 0 && reg_cond < 8 && registers_[reg_cond] != 0) {
                // Salta a la etiqueta si existe
                auto it = label_map_.find(instr.label);
                if (it != label_map_.end()) {
                    pc_ = it->second;
                    print_registers();
                    continue;
                } else {
                    std::cout << "[PE " << id_ << "] Error: etiqueta '" << instr.label << "' no encontrada" << std::endl;
                }
            }
        } else {
            execute_instruction(instr);
        }
        print_registers();
        ++pc_;
    }
    finished_ = true;
    std::cout << "[PE " << id_ << "] Ejecución finalizada.\n";
    cv_.notify_all();
}

// Parser mejorado: ignora etiquetas y maneja operandos
Instruction PE::parse_instruction(const std::string& instr_str) {
    Instruction instr;
    instr.type = Instruction::INVALID;
    std::istringstream iss(instr_str);
    std::string op;
    iss >> op;
    // Ignora etiquetas
    if (!op.empty() && op.back() == ':') {
        instr.type = Instruction::INVALID;
        return instr;
    }
    if (op == "LOAD") {
        std::string reg, addr;
        iss >> reg >> addr;
        instr.type = Instruction::LOAD;
        if (reg.substr(0,3) == "R1" || reg.substr(0,3) == "REG")
            instr.reg_dest = std::stoi(reg.substr(reg.find_first_of("0123456789")));
        // Simula dirección como entero si es 0x...
        if (addr.find("0x") != std::string::npos)
            instr.address = std::stoll(addr, nullptr, 16);
        else if (addr.find(",") != std::string::npos)
            instr.address = std::stoll(addr.substr(0, addr.find(",")));
        else
            instr.address = 0;
    } else if (op == "STORE") {
        std::string reg, addr;
        iss >> reg >> addr;
        instr.type = Instruction::STORE;
        if (reg.substr(0,3) == "R1" || reg.substr(0,3) == "REG")
            instr.reg_src1 = std::stoi(reg.substr(reg.find_first_of("0123456789")));
        if (addr.find("0x") != std::string::npos)
            instr.address = std::stoll(addr, nullptr, 16);
        else if (addr.find(",") != std::string::npos)
            instr.address = std::stoll(addr.substr(0, addr.find(",")));
        else
            instr.address = 0;
    } else if (op == "FMUL" || op == "FADD") {
        std::string regd, rega, regb;
        iss >> regd >> rega >> regb;
        instr.type = (op == "FMUL") ? Instruction::FMUL : Instruction::FADD;
        instr.reg_dest = std::stoi(regd.substr(regd.find_first_of("0123456789")));
        instr.reg_src1 = std::stoi(rega.substr(rega.find_first_of("0123456789")));
        instr.reg_src2 = std::stoi(regb.substr(regb.find_first_of("0123456789")));
    } else if (op == "INC" || op == "DEC") {
        std::string reg;
        iss >> reg;
        instr.type = (op == "INC") ? Instruction::INC : Instruction::DEC;
        instr.reg_dest = std::stoi(reg.substr(reg.find_first_of("0123456789")));
    } else if (op == "JNZ") {
        std::string label;
        iss >> label;
        instr.type = Instruction::JNZ;
        instr.label = label;
    }
    return instr;
}

// Ejecuta la instrucción decodificada (solo manipula registros, no memoria real)
void PE::execute_instruction(const Instruction& instr) {
    switch (instr.type) {
        case Instruction::LOAD:
            // Simulación: carga valor dummy (por ejemplo, la dirección como double)
            if (instr.reg_dest >= 0 && instr.reg_dest < 8)
                registers_[instr.reg_dest] = static_cast<double>(instr.address);
            break;
        case Instruction::STORE:
            // Simulación: no hace nada, solo imprime
            if (instr.reg_src1 >= 0 && instr.reg_src1 < 8)
                std::cout << "[PE " << id_ << "] STORE simulado de REG" << instr.reg_src1 << " a dirección " << instr.address << std::endl;
            break;
        case Instruction::FMUL:
            if (instr.reg_dest >= 0 && instr.reg_dest < 8 && instr.reg_src1 >= 0 && instr.reg_src1 < 8 && instr.reg_src2 >= 0 && instr.reg_src2 < 8)
                registers_[instr.reg_dest] = registers_[instr.reg_src1] * registers_[instr.reg_src2];
            break;
        case Instruction::FADD:
            if (instr.reg_dest >= 0 && instr.reg_dest < 8 && instr.reg_src1 >= 0 && instr.reg_src1 < 8 && instr.reg_src2 >= 0 && instr.reg_src2 < 8)
                registers_[instr.reg_dest] = registers_[instr.reg_src1] + registers_[instr.reg_src2];
            break;
        case Instruction::INC:
            if (instr.reg_dest >= 0 && instr.reg_dest < 8)
                registers_[instr.reg_dest] += 1.0;
            break;
        case Instruction::DEC:
            if (instr.reg_dest >= 0 && instr.reg_dest < 8)
                registers_[instr.reg_dest] -= 1.0;
            break;
        case Instruction::JNZ:
            // La lógica de salto está en run(), aquí no se hace nada
            break;
        default:
            std::cout << "[PE " << id_ << "] Instrucción inválida o no soportada" << std::endl;
    }
}