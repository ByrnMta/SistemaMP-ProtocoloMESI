#pragma once

#include <iostream>
#include <vector>

#include "../cache/include/Cache.h"
#include "../Interconnect/interconnect.h"
#include "MESIController.h"
#include "../PE/PE.h"


inline void connect_mesi_controllers(std::vector<PE*>& pes, std::vector<Cache*>& caches, Interconnect* interconnect) {
    // Inicializa el vector de MESIControllers en el Interconnect
    for (size_t i = 0; i < pes.size(); ++i) {
        auto* mesi = new MESIController(caches[i], interconnect, pes[i]->get_id());
        pes[i]->set_mesi_controller(mesi);
        interconnect->attach_mesi_controller(mesi, pes[i]->get_id());
    }
}
