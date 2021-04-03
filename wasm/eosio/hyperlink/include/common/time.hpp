#pragma once

namespace hyperlink {

    static const uint32_t current_time() {
        return eosio::current_time_point().sec_since_epoch();
    }
        
}