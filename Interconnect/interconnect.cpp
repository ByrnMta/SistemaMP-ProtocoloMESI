#include <iostream>

#include "interconnect.h"
#include "Interconnect/bus_types.h"
#include "../MESI/MESIController.h"

using namespace std;

Interconnect::Interconnect(int num_pes, Memoria* memoria, size_t queue_depth)
    : num_pes(num_pes), queue_depth_(queue_depth), main_memory_(memoria) {
    mesi_controllers.resize(num_pes, nullptr);
}



// ====================================================================================METODOS PRINCIPALES


// Procesa un mensaje específico del bus
optional<InterconnectResponse> Interconnect::process_messages(const BusMessage& msg) {

    unique_lock<mutex> lock(bus_mutex);

    
    bus_cv.wait(lock, [&] {
        return bus_queue.size() > 0 && bus_queue.front().sender_id == msg.sender_id && (current_active_pe_id == -1 || current_active_pe_id == msg.sender_id);
    });

    // Marca el PE actual como activo
    current_active_pe_id = msg.sender_id;
    bus_cv.notify_all();

    cout << "[Interconnect] PE " << msg.sender_id << " tiene el turno para procesar su mensaje." << endl;
    optional<InterconnectResponse> result;
    switch (bus_queue.front().type) {
        case READ_MISS: {
            InterconnectResponse resp = handle_cache_miss(bus_queue.front());
            result = resp;
            break;
        }
        case WRITE_MISS: {
            InterconnectResponse resp = handle_cache_miss(bus_queue.front());
            result = resp;
            break;
        }
        case INVALIDATE:
            handle_invalidate(bus_queue.front());
            result = nullopt;
            break;
        case WRITE_BACK:
            handle_write_back(bus_queue.front());
            result = nullopt;
            break;
        default:
            cerr << "[Interconnect] Tipo de mensaje desconocido." << endl;
            result = nullopt;
            break;
    }

    // Remueve el mensaje procesado de la cola
    bus_queue.pop();
    cout << "[Interconnect] Mensaje de PE " << msg.sender_id << " procesado y removido de la cola." << endl;

    // Actualiza el PE activo con el siguiente en la cola o -1 si está vacía
    if (!bus_queue.empty()) {
        current_active_pe_id = bus_queue.front().sender_id;
    } else {
        current_active_pe_id = -1;
    }
    bus_cv.notify_all();

    return result;
}


// Envía un mensaje al interconnect y espera su turno para usar el bus
void Interconnect::send_message(const BusMessage& msg) {

    unique_lock<mutex> lock(bus_mutex);                                             // Bloquea el mutex del bus

    if (bus_queue.size() < queue_depth_) {                                          // Si hay espacio en la cola

        cout << "[PE " << msg.sender_id << "] Solicita acceso al bus para mensaje tipo " << msg.type << " en dirección " << msg.address << endl;
        
        bus_queue.push(msg);                                                        // Encola el mensaje
        bus_traffic++;                                                              // Incrementa el tráfico del bus
        
        print_bus_state();

        cout << "[PE " << msg.sender_id << "] Mensaje encolado. Esperando turno..." << endl;
        
        bus_cv.notify_all();                                                        // Notifica a los PEs que hay un nuevo mensaje
        
        bus_cv.wait(lock, [&] {                                                     // Espera hasta que sea su turno, re
            return current_active_pe_id != msg.sender_id;
        });
        
        cout << "[PE " << msg.sender_id << "] Turno concedido." << endl;
        
    } else {
        cerr << "[Interconnect] Cola de mensajes llena. Mensaje descartado." << endl;
    }
}


// ==================================================================================== FUNCIONES DE CONFIGURACIÓN ===


// Adjunta un MESIController al interconnect
void Interconnect::attach_mesi_controller(MESIController* mesi, int pe_id) {
    if (pe_id >= 0 && pe_id < num_pes) {
        if (mesi_controllers.size() < num_pes) mesi_controllers.resize(num_pes, nullptr);
        mesi_controllers[pe_id] = mesi;
    }
}


// ==================================================================================== FUNCIONES AUXILIARES ===



InterconnectResponse Interconnect::handle_cache_miss(const BusMessage& msg) {
    string tipo = (msg.type == WRITE_MISS) ? "WRITE_MISS" : "READ_MISS";
    cout << "[VERIF-INTERCONNECT] INICIO " << tipo << ": PE " << msg.sender_id << " solicita dirección " << msg.address << endl;

    optional<array<double,4>> linea;
    bool from_memory = false;

    for (int i = 0; i < num_pes; ++i) {
        if (i != msg.sender_id && mesi_controllers.size() > i && mesi_controllers[i]) {
            cout << "[VERIF-INTERCONNECT] Consultando MESIController de PE " << i << " por línea " << msg.address << endl;
            linea = mesi_controllers[i]->process_bus_message(msg);
            if (linea.has_value()) {
                break;
            }
        }
    }

    if (!linea.has_value()) {
        cout << "[VERIF-INTERCONNECT] Línea NO encontrada en ningún PE, cargando de memoria principal para dirección " << msg.address << endl;
        linea = array<double,4>{9.9,0.0,0.0,0.0}; // Línea simulada para pruebas
        //linea = main_memory_->read_bloque(msg.address);
        from_memory = true;
    } else {
        cout << "[VERIF-INTERCONNECT] Línea encontrada en algún PE para dirección " << msg.address << endl;
    }

    cout << "[VERIF-INTERCONNECT] FIN " << tipo << " para dirección " << msg.address << endl;
    return InterconnectResponse{*linea, from_memory};
}


void Interconnect::handle_invalidate(const BusMessage& msg) {
    cout << "[VERIF-INTERCONNECT] Procesando INVALIDATE de PE " << msg.sender_id << " para dirección " << msg.address << endl;
    for (int i = 0; i < num_pes; ++i) {
        if (i != msg.sender_id && mesi_controllers.size() > i && mesi_controllers[i]) {
            cout << "[VERIF-INTERCONNECT] Enviando INVALIDATE a PE " << i << " para dirección " << msg.address << endl;
            mesi_controllers[i]->process_bus_message(msg);
            cout << "[VERIF-INTERCONNECT] INVALIDATE procesado por PE " << i << " para dirección " << msg.address << endl;
        }
    }
    cout << "[VERIF-INTERCONNECT] FIN INVALIDATE para dirección " << msg.address << endl;
}


void Interconnect::handle_write_back(const BusMessage& msg) {
    if (main_memory_) {
        cout << "[Interconnect] Recibido WRITE_BACK de PE " << msg.sender_id << " para dirección " << msg.address << endl;
        //main_memory_->write_bloque(msg.address, msg.cache_line);
        cout << "[Interconnect] Línea escrita en memoria principal por WRITE_BACK." << endl;
    }
}


// ====================================================================================


int Interconnect::get_bus_traffic() const {
    return bus_traffic;
}

void Interconnect::print_bus_state() const {
    cout << "Trafico: " << bus_traffic << endl;
    cout << "En cola: " << bus_queue.size() << endl;
}
