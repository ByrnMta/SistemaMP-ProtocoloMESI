#ifndef MESI_CONTROLLER_H
#define MESI_CONTROLLER_H

#include <cstdint>
#include <optional>
#include <mutex>
#include <condition_variable>

#include "../cache/include/Cache.h"
#include "../Interconnect/bus_types.h"
#include "../Interconnect/interconnect.h"

using namespace std;

// Forward declaration para evitar dependencias circulares
class Interconnect;

class MESIController {
public:
	MESIController(Cache* cache, Interconnect* interconnect, int pe_id);

	optional<double> read(uint16_t address);
	void write(uint16_t address, double value);
	optional<array<double,4>> process_bus_message(const BusMessage& msg);

	optional<array<double,4>> handle_cache_miss_bus(const BusMessage& msg);
	void handle_invalidate_bus(const BusMessage& msg);
	optional<double> request_line_from_bus(uint16_t address, MessageType type, double value, MESIState new_state);
	void request_write_back(uint16_t address, const array<double,4>& linea_cache);
	void send_invalidate_to_others(uint16_t address);

private:
	Cache* cache_;
	Interconnect* interconnect_;
	int pe_id_;
};

#endif // MESI_CONTROLLER_H
