#include "PE.h"
#include <iostream>
#include <thread>
#include <sstream>

using namespace std;

// Constructor: inicializa el PE con su ID y registros en cero
PE::PE(int id)
    : id_(id), pc_(0), finished_(false) {
    for (int i = 0; i < 8; ++i) registers_[i] = 0.0;
}

// Carga el vector de instrucciones (en string) y construye el mapa de etiquetas
void PE::load_instructions(const vector<string>& instructions) {
    lock_guard<mutex> lock(mtx_);
    instructions_ = instructions;
    pc_ = 0;
    build_label_map();
}

// Construye el mapa de etiquetas (labels) a índices de instrucción
void PE::build_label_map() {
    label_map_.clear();
    for (size_t i = 0; i < instructions_.size(); ++i) {
        string instr = instructions_[i];
        // Quita espacios al inicio
        instr.erase(0, instr.find_first_not_of(" \t"));
        // Si termina con ':' es una etiqueta
        if (!instr.empty() && instr.back() == ':') {
            string label = instr.substr(0, instr.size() - 1);
            label_map_[label] = i;
        }
    }
}

// Inicia la ejecución del PE en un hilo separado
void PE::start() {
    thread_ = thread(&PE::run, this);
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
    cout << "\n" << "[PE " << id_ << "] Estado de registros:\n";
    for (int i = 0; i < 8; ++i) {
        cout << "  REG" << i << ": " << registers_[i] << endl;
    }
}

// Bucle principal de ejecución del PE: fetch-decode-execute con soporte de saltos
void PE::run() {
    unique_lock<mutex> lock(mtx_);
    //cout << "[PE " << id_ << "] Iniciando ejecución...\n";
    while (pc_ < instructions_.size()) {
        string instr_str = instructions_[pc_];
        // Si es una etiqueta, solo avanza
        string trimmed = instr_str;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        if (!trimmed.empty() && trimmed.back() == ':') {
            ++pc_;
            continue;
        }
    Instruction instr = parse_instruction(instr_str);
        //cout << "[PE " << id_ << "] Ejecutando: " << instr_str << endl;
        // Lógica de salto para JNZ
        if (instr.type == Instruction::JNZ) {
            // Por convención, REG3 es el registro de control de bucle
            int reg_cond = 3;
            if (reg_cond >= 0 && reg_cond < 8 && registers_[reg_cond] != 0) {
                // Salta a la etiqueta si existe
                auto it = label_map_.find(instr.label);
                if (it != label_map_.end()) {
                    pc_ = it->second;
                    //print_registers();
                    continue;
                } else {
                    cout << "[PE " << id_ << "] Error: etiqueta '" << instr.label << "' no encontrada" << endl;
                }
            }
        } else {
            execute_instruction(instr);
        }
        //print_registers();
        ++pc_;
    }
    finished_ = true;
    // cout << "[PE " << id_ << "] Ejecución finalizada.\n";
    cv_.notify_all();
}

// Parser mejorado: ignora etiquetas y maneja operandos
Instruction PE::parse_instruction(const string& instr_str) {
    Instruction instr;
    instr.type = Instruction::INVALID;
    istringstream iss(instr_str);
    string op;
    iss >> op;
    // Ignora etiquetas
    if (!op.empty() && op.back() == ':') {
        instr.type = Instruction::INVALID;
        return instr;
    }
    if (op == "LOAD") {
        string reg, addr;
        iss >> reg >> addr;
        instr.type = Instruction::LOAD;
        if (reg.substr(0,3) == "R1" || reg.substr(0,3) == "REG")
            instr.reg_dest = stoi(reg.substr(reg.find_first_of("0123456789")));
        // Soporte para LOAD REGx, [REGy]
        if (!addr.empty() && addr.front() == '[' && addr.back() == ']') {
            string reg_indirect = addr.substr(1, addr.size() - 2); // quita corchetes
            if (reg_indirect.substr(0,3) == "R1" || reg_indirect.substr(0,3) == "REG")
                instr.reg_src1 = stoi(reg_indirect.substr(reg_indirect.find_first_of("0123456789")));
            instr.address = -1; // Marca especial para indirecto
        } else if (addr.find("0x") != string::npos)
            instr.address = stoll(addr, nullptr, 16);
        else if (addr.find(",") != string::npos)
            instr.address = stoll(addr.substr(0, addr.find(",")));
        else
            instr.address = 0;
    } else if (op == "LOADI") {
        // LOADI REGx, valor_inmediato
        string reg, imm;
        iss >> reg >> imm;
        instr.type = Instruction::LOADI;
        instr.reg_dest = stoi(reg.substr(reg.find_first_of("0123456789")));
        // Soporta decimal o hexadecimal
        if (imm.find("0x") != string::npos)
            instr.immediate = stoll(imm, nullptr, 16);
        else
            instr.immediate = stoll(imm);
    } else if (op == "STORE") {
        string reg, addr;
        iss >> reg >> addr;
        instr.type = Instruction::STORE;
        if (reg.substr(0,3) == "R1" || reg.substr(0,3) == "REG")
            instr.reg_src1 = stoi(reg.substr(reg.find_first_of("0123456789")));
        // Soporte para STORE REGx, [REGy]
        if (!addr.empty() && addr.front() == '[' && addr.back() == ']') {
            string reg_indirect = addr.substr(1, addr.size() - 2);
            if (reg_indirect.substr(0,3) == "R1" || reg_indirect.substr(0,3) == "REG")
                instr.reg_src2 = stoi(reg_indirect.substr(reg_indirect.find_first_of("0123456789")));
            instr.address = -1; // Marca especial para indirecto
        } else if (addr.find("0x") != string::npos)
            instr.address = stoll(addr, nullptr, 16);
        else if (addr.find(",") != string::npos)
            instr.address = stoll(addr.substr(0, addr.find(",")));
        else
            instr.address = 0;
    } else if (op == "FMUL" || op == "FADD") {
        string regd, rega, regb;
        iss >> regd >> rega >> regb;
        instr.type = (op == "FMUL") ? Instruction::FMUL : Instruction::FADD;
        instr.reg_dest = stoi(regd.substr(regd.find_first_of("0123456789")));
        instr.reg_src1 = stoi(rega.substr(rega.find_first_of("0123456789")));
        instr.reg_src2 = stoi(regb.substr(regb.find_first_of("0123456789")));
    } else if (op == "INC" || op == "DEC") {
        string reg;
        iss >> reg;
        instr.type = (op == "INC") ? Instruction::INC : Instruction::DEC;
        instr.reg_dest = stoi(reg.substr(reg.find_first_of("0123456789")));
    } else if (op == "JNZ") {
        string label;
        iss >> label;
        instr.type = Instruction::JNZ;
        instr.label = label;
    }
    return instr;
}

// Ejecuta la instrucción decodificada (solo manipula registros, no memoria real)
void PE::execute_instruction(const Instruction& instr) {

    switch (instr.type) {
        case Instruction::LOADI:
            // Carga un valor inmediato (dirección) en un registro
            if (instr.reg_dest >= 0 && instr.reg_dest < 8)
                registers_[instr.reg_dest] = static_cast<double>(instr.immediate);
            break;
        case Instruction::LOAD:
            // Soporte para LOAD REGx, [REGy] y LOAD REGx, dir
            if (instr.reg_dest >= 0 && instr.reg_dest < 8 && mesi_controller_) {
                uint16_t addr = 0;
                if (instr.address == -1 && instr.reg_src1 >= 0 && instr.reg_src1 < 8) {
                    addr = static_cast<uint16_t>(registers_[instr.reg_src1]);
                } else {
                    addr = static_cast<uint16_t>(instr.address);
                }
                std::cout << "[PE " << id_ << "] Llamando a MESIController::read para dirección " << addr << std::endl;
                auto result = mesi_controller_->read(addr);
                if (result) {
                    std::cout << "[PE " << id_ << "] MESIController devolvió dato: " << *result << std::endl;
                    registers_[instr.reg_dest] = *result;
                } else {
                    std::cout << "[PE " << id_ << "] MESIController devolvió miss, se asigna 0.0\n";
                    registers_[instr.reg_dest] = 0.0; // Simulación: asigna 0.0 en caso de miss
                }
            }
            break;

        case Instruction::STORE:
            // Soporte para STORE REGx, [REGy] y STORE REGx, dir
            if (instr.reg_src1 >= 0 && instr.reg_src1 < 8 && mesi_controller_) {
                uint16_t addr = 0;
                if (instr.address == -1 && instr.reg_src2 >= 0 && instr.reg_src2 < 8) {
                    addr = static_cast<uint16_t>(registers_[instr.reg_src2]);
                } else {
                    addr = static_cast<uint16_t>(instr.address);
                }
                std::cout << "[PE " << id_ << "] Llamando a MESIController::write para dirección " << addr << ", valor " << registers_[instr.reg_src1] << std::endl;
                mesi_controller_->write(addr, registers_[instr.reg_src1]);
            }
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
            cout << "[PE " << id_ << "] Instrucción inválida o no soportada" << endl;
    }
}