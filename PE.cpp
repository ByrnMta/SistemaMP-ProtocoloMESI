#include "PE.h"
#include <iostream>
#include <thread>
#include <sstream>

// Constructor: inicializa el PE con su ID y registros en cero
PE::PE(int id)
    : id_(id), pc_(0), finished_(false) {
    for (int i = 0; i < 8; ++i) registers_[i] = 0.0;
}

// Carga el vector de instrucciones (en string)
void PE::load_instructions(const std::vector<std::string>& instructions) {
    std::lock_guard<std::mutex> lock(mtx_);
    instructions_ = instructions;
    pc_ = 0;
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

// Bucle principal de ejecución del PE: fetch-decode-execute
void PE::run() {
    std::unique_lock<std::mutex> lock(mtx_);
    std::cout << "[PE " << id_ << "] Iniciando ejecución...\n";
    while (pc_ < instructions_.size()) {
        std::string instr_str = instructions_[pc_];
        Instruction instr = parse_instruction(instr_str);
        std::cout << "[PE " << id_ << "] Ejecutando: " << instr_str << std::endl;
        execute_instruction(instr);
        print_registers();
        ++pc_;
        // Simula retardo de ciclo
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lock.lock();
    }
    finished_ = true;
    std::cout << "[PE " << id_ << "] Ejecución finalizada.\n";
    cv_.notify_all();
}

// Parser simple de instrucciones (solo para pruebas iniciales)
Instruction PE::parse_instruction(const std::string& instr_str) {
    Instruction instr;
    instr.type = Instruction::INVALID;
    std::istringstream iss(instr_str);
    std::string op;
    iss >> op;
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
            // Simulación: no implementa saltos reales aún
            std::cout << "[PE " << id_ << "] JNZ simulado (no implementado)" << std::endl;
            break;
        default:
            std::cout << "[PE " << id_ << "] Instrucción inválida o no soportada" << std::endl;
    }
}