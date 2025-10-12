#include <chrono>
#include <thread>
#include "interconnect.h"
#include <iostream>

// Constructor: inicializa el interconect y sus componentes
Interconnect::Interconnect() {
	
}

// ------------------------------------------------
// -    Metodos de control de hilo propio         -
// ------------------------------------------------

// Inicia el hilo propio del Interconnect
void Interconnect::start() {
	running_ = true;
	thread_ = std::thread(&Interconnect::run, this);
}

// Detiene el hilo propio del Interconnect
void Interconnect::stop() {
	running_ = false;
	if (thread_.joinable()) {
		thread_.join();
	}
}

// Ciclo principal del hilo: procesa mensajes continuamente
void Interconnect::run() {
	while (running_) {
		process_mssg();
	}
}


// ------------------------------------------------
// -    Metodos de flujo de mensajes              -
// ------------------------------------------------

// Registra una caché (PE) en el interconect
void Interconnect::register_cache(uint8_t cache_id, Cache* cache) {
	regs_cache[cache_id] = cache;
}

// Encola un mensaje desde un PE para ser procesado por el interconect
void Interconnect::enqueue_mssg(uint8_t pe_id, const Message& msg) {
	std::lock_guard<std::mutex> lock(queue_mutex_);
	mssg_queue.push(msg);
}

// Procesa el siguiente mensaje en la cola (simula acceso al bus/memoria)
void Interconnect::process_mssg() {
	std::lock_guard<std::mutex> lock(queue_mutex_);
	if (mssg_queue.empty()) {
			if (mssg_queue.empty()) {
				return;
	}
	Message msg = mssg_queue.front();
	mssg_queue.pop();
	switch (msg.type) {
		case MessageType::READ:
			// Procesar solicitud de lectura
			break;
		case MessageType::WRITE:
			// Procesar solicitud de escritura
			break;
		case MessageType::READ_RESP:
			// Procesar respuesta de lectura
			break;
		case MessageType::WRITE_RESP:
			// Procesar respuesta de escritura
			break;
		case MessageType::INVALIDATE:
			// Procesar solicitud de invalidación
			break;
		case MessageType::INV_ACK:
			// Procesar acknowledge de invalidación
			break;
		case MessageType::BROADCAST_INVALIDATE:
			// Procesar difusión de invalidación
			break;
		case MessageType::INV_COMPLETE:
			// Procesar notificación de finalización de invalidación
			break;
		default:
			break;
	}
}

// Recibe un mensaje de un PE u otro componente y lo encola para procesamiento posterior
void Interconnect::receive_mssg(const Message& msg) {
	enqueue_mssg(msg.src_id, msg);
}
