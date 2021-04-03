#pragma once

#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>

namespace hyperlink {

class Digester {
    public:
        static std::vector<char> Digest(const std::vector<char>& data) {
            return eosio::pack(eosio::sha256( data.data(), data.size() ));
        }
};

}