#pragma once
#include <eosio/eosio.hpp>
#include <eosio/string.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>
#include <common/ipfs.hpp>
#include <common/time.hpp>
#include "config.hpp"

namespace hyperlink {

class [[eosio::contract("nexus")]] nexus : public eosio::contract {
   public:
      using contract::contract;

      nexus( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
      : contract(receiver, code, ds), 
      _config_t(receiver, receiver.value),
      _channels(receiver, receiver.value),
      _events(receiver, receiver.value) {
         _config = _config_t.get_or_default();
      }

      struct [[eosio::table("config"), eosio::contract("nexus")]] config_v1 {
         uint32_t eventTTL = NEXUS_EVENT_TTL;
      };

      struct [[eosio::table("channels"), eosio::contract("nexus")]] channels_v1 {
         uint64_t id;
         eosio::name owner;
         eosio::name subscriber;
         std::vector<char> config;
         // updated on every event
         std::vector<char> state;
         uint64_t primary_key() const { return id; }
         uint64_t by_owner() const { return owner.value; }
         uint64_t by_sub() const { return subscriber.value; }
      };

      struct [[eosio::table("events"), eosio::contract("nexus")]] events_v1 {
         uint64_t id;
         uint64_t channelId;
         uint32_t timestamp;
         eosio::name status;
         std::vector<char> eventId;
         std::vector<char> eventData;
                  
         uint64_t primary_key() const { return id; }
         uint64_t by_channel() const { return channelId; }
         uint64_t by_status() const { return status.value; }
         eosio::checksum256 by_eventId() const { return eosio::sha256(eventId.data(), eventId.size()); }
      };

      typedef eosio::singleton< NEXUS_CONFIG_TBL, config_v1 > 
      config_t; 

      typedef eosio::multi_index< NEXUS_CHANNEL_TBL, channels_v1, 
      eosio::indexed_by< NEXUS_OWNER_IDX, eosio::const_mem_fun<channels_v1, uint64_t, &channels_v1::by_owner>>, 
      eosio::indexed_by< NEXUS_SUB_IDX, eosio::const_mem_fun<channels_v1, uint64_t, &channels_v1::by_sub>>> 
      channels_t;

      typedef eosio::multi_index< NEXUS_EVENT_TBL, events_v1, 
      eosio::indexed_by< NEXUS_CHANNEL_IDX, eosio::const_mem_fun<events_v1, uint64_t, &events_v1::by_channel>>,
      eosio::indexed_by< NEXUS_STATUS_IDX, eosio::const_mem_fun<events_v1, uint64_t, &events_v1::by_status>>,
      eosio::indexed_by< NEXUS_SHA_IDX, eosio::const_mem_fun<events_v1, eosio::checksum256, &events_v1::by_eventId>>> 
      events_t;
      #include "../../include/common/expiringEvents.hpp"

      config_v1 _config;
      config_t _config_t;
      channels_t _channels;
      ACTION init    ( config_v1 config );
      ACTION open    (eosio::name& owner, 
                        eosio::name& subscriber, 
                        std::vector<char>& config);
      ACTION join    ( uint64_t channelId, std::vector<char>& state );
      ACTION close   ( uint64_t channelId );
      //ACTION flush   ( uint64_t depth );
      ACTION submit   ( uint64_t channelId,
                        eosio::name& status,
                        std::vector<char>& state,  
                        std::vector<char>& eventID,
                        std::vector<char>& eventData  );
};

}