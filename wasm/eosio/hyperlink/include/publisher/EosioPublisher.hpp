#pragma once
#include "Publisher.hpp"
#include <digesters/sha256.hpp>

namespace hyperlink {

class EosioPublisher : public Publisher {
    public:
        EosioPublisher(const eosio::name& receiver) : Publisher(receiver) {}

        void Publish(const std::vector<char>& eventData) {
            Publisher::Publish(Digester::Digest(eventData), eventData);
        }
};
}