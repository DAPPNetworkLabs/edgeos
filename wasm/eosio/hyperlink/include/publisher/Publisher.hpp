#pragma once
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <common/time.hpp>
#include "config.hpp"

namespace hyperlink {

#define HYPERLINK_PUBLISHER_ABI \
   TABLE pub_config_v1 {\
      uint64_t nonce = 0;\
      uint32_t eventTTL = PUB_EVENT_TTL;\
   };\
   TABLE pub_event_v1 {\
      uint64_t nonce;\
      uint32_t timestamp;\
      std::vector<char> eventHash;\
      std::vector<char> eventData;\
      uint64_t primary_key() const { return nonce; }\
   };\
   typedef eosio::multi_index< PUB_CONFIG_TBL, pub_config_v1 > hyperlink_publisher_config_abi_t;\
   typedef eosio::multi_index< PUB_EVENT_TBL, pub_event_v1 > hyperlink_publisher_events_abi_t;

class Publisher {
   public:
      Publisher(const eosio::name& receiver) 
      : _self(receiver), 
      _config_t(receiver, receiver.value), 
      _events(receiver, receiver.value)
      {}

      HYPERLINK_PUBLISHER_ABI

      typedef eosio::singleton< PUB_CONFIG_TBL, pub_config_v1 > config_t;
      typedef eosio::multi_index< PUB_EVENT_TBL, pub_event_v1 > events_t;

      eosio::name _self;
      config_t _config_t;

      void Config(const pub_config_v1& config) {
         _config_t.set(config, _self);
      }

      void Publish(const std::vector<char>& eventHash, const std::vector<char>& eventData) {
         _events.emplace(_self, [&](auto &row) {
            row.nonce = useNonce();
            row.timestamp = current_time();
            row.eventHash = eventHash;
            row.eventData = eventData;
         });
         Flush(1);
      }

#include "../common/expiringEvents.hpp"
   private:
      uint64_t useNonce() {
         auto config = _config_t.get_or_default();
         auto nonce = config.nonce++;
         _config_t.set(config, _self);
         return nonce;
      }
};
}