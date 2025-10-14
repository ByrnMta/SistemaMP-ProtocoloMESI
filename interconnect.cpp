#include "interconnect.h"
#include <mutex>

Interconnect::Interconnect(size_t fifo_depth)
	: fifo_depth_(fifo_depth) {}

void Interconnect::register_cache(uint8_t cache_id, Cache* cache) {
	regs_cache[cache_id] = cache;
}

bool Interconnect::enqueue_mssg(uint8_t pe_id, const Message& msg) {
	std::lock_guard<std::mutex> lock(queue_mutex_);
	if (mssg_queue.size() >= fifo_depth_) {
		return false;
	}
	mssg_queue.push(msg);
	return true;
}

void Interconnect::process_mssg() {
	std::lock_guard<std::mutex> lock(queue_mutex_);
	if (mssg_queue.empty()) {
		return;
	}
	Message msg = mssg_queue.front();
	mssg_queue.pop();
	// Procesamiento real del mensaje aquí
}

void Interconnect::receive_mssg(const Message& msg) {
	enqueue_mssg(msg.src_id, msg);
}

