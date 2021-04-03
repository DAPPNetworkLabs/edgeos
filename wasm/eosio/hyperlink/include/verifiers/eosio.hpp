#pragma once

#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>

namespace hyperlink {

class Verifier {
    public:
        static void Verify(const std::vector<char>& signer, const std::vector<char>& signature, const std::vector<char>& digest) {
            auto pub = eosio::unpack<eosio::public_key>(signer);
            auto sig = eosio::unpack<eosio::signature>(signature);
            auto dig = eosio::unpack<eosio::checksum256>(digest);
            eosio::assert_recover_key( dig, sig, pub);
        }

        static std::vector<char> Recover(const std::vector<char>& signature, const std::vector<char>& digest) {
            auto dig = eosio::unpack<eosio::checksum256>(digest);
            auto sig = eosio::unpack<eosio::signature>(signature);
            return eosio::pack(eosio::recover_key( dig, sig ));
        }
};

}