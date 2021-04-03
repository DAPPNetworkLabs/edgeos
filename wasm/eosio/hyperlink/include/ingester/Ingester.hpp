#pragma once
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>
#include <common/time.hpp>
#include "config.hpp"

namespace hyperlink {

#define HYPERLINK_INGESTER_ABI \
   TABLE sub_config_v1 {\
      uint64_t signers = 2;\
      uint64_t execs = 0;\
      uint32_t eventTTL = SUB_EVENT_TTL;\
   };\
   TABLE sub_event_v1 {\
      uint64_t id;\
      uint32_t timestamp;\
      std::vector<uint64_t> signers;\
      std::vector<uint64_t> execs;\
      std::vector<char> eventHash;\
      uint64_t primary_key() const { return id; }\
      eosio::checksum256 by_sha() const { return eosio::sha256(eventHash.data(), eventHash.size()); }\
   };\
   TABLE sub_signer_v1 {\
      uint64_t id;\
      std::vector<char> signer;\
      uint64_t primary_key() const { return id; }\
      eosio::checksum256 by_sha() const { return eosio::sha256(signer.data(), signer.size()); }\
   };\
   typedef eosio::multi_index< SUB_CONFIG_TBL, sub_config_v1 > hyperlink_ingester_config_abi_t;\
   typedef eosio::multi_index< SUB_EVENT_TBL, sub_event_v1, \
   eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_event_v1, eosio::checksum256, &sub_event_v1::by_sha>>> \
   hyperlink_ingester_events_abi_t;\
   typedef eosio::multi_index< SUB_SIGNER_TBL, sub_signer_v1, \
   eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_signer_v1, eosio::checksum256, &sub_signer_v1::by_sha>>> \
   hyperlink_ingester_signers_abi_t;\
   typedef eosio::multi_index< SUB_EXEC_TBL, sub_signer_v1, \
   eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_signer_v1, eosio::checksum256, &sub_signer_v1::by_sha>>> \
   hyperlink_ingester_execs_abi_t;

class Ingester {
   public:
      Ingester(const eosio::name& receiver) 
      : _self(receiver), 
      _config(receiver, receiver.value), 
      _events(receiver, receiver.value), 
      _signers(receiver, receiver.value), 
      _execs(receiver, receiver.value)
      {}

      HYPERLINK_INGESTER_ABI

      typedef eosio::singleton< SUB_CONFIG_TBL, sub_config_v1 > config_t;

      typedef eosio::multi_index< SUB_EVENT_TBL, sub_event_v1,
      eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_event_v1, eosio::checksum256, &sub_event_v1::by_sha>>>
      events_t;

      typedef eosio::multi_index< SUB_SIGNER_TBL, sub_signer_v1,
      eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_signer_v1, eosio::checksum256, &sub_signer_v1::by_sha>>>
      signers_t;

      typedef eosio::multi_index< SUB_EXEC_TBL, sub_signer_v1,
      eosio::indexed_by< SUB_SHA_IDX, eosio::const_mem_fun<sub_signer_v1, eosio::checksum256, &sub_signer_v1::by_sha>>>
      execs_t;

      eosio::name _self;
      config_t _config;
      events_t _events;
      signers_t _signers;
      execs_t _execs;

      void Ingest(   const std::vector<char>& signer,
                     const std::vector<char>& signature,
                     const std::vector<char>& eventHash ) 
      {
         //THIS METHOD MUST BE EXTENDED TO COMPARE THE SIGNER/SIGNATURE/HASH

         auto signerKey = eosio::sha256(signer.data(), signer.size());
         bool isExec = false;
         uint64_t signerId = 0;

         auto execs = _execs.get_index<SUB_SHA_IDX>();
         auto exec = execs.find(signerKey);

         if(exec != execs.end()) {
            isExec = true;
            signerId = exec->id;
         } else {
            auto signers = _signers.get_index<SUB_SHA_IDX>();
            auto existing = signers.find(signerKey);
            eosio::check(existing != signers.end(), "Signer not authorized.");
            signerId = existing->id;
         }

         auto eventKey = eosio::sha256(eventHash.data(), eventHash.size());
         auto events = _events.get_index<SUB_SHA_IDX>();
         auto existing = events.find(eventKey);
         if(existing == events.end()) {
            //new event
            _events.emplace(_self, [&](auto &row) {
               row.id = _events.available_primary_key();
               row.timestamp = current_time();
               if(isExec) row.execs.push_back(signerId);
               if(!isExec) row.signers.push_back(signerId);
               row.eventHash = eventHash;               
            });

         } else {
            //modify event
            events.modify(existing, _self, [&](auto &row) {
               if(isExec) row.execs.push_back(signerId);
               if(!isExec) row.signers.push_back(signerId);             
            });
         }
      }

      void Digest(   const std::vector<char>& eventHash,
                     const std::vector<char>& eventData ) 
      {
         //THIS METHOD MUST BE EXTENDED TO COMPARE THE EVENT HASH TO THE EVENT DATA

         auto eventKey = eosio::sha256(eventHash.data(), eventHash.size());
         auto events = _events.get_index<SUB_SHA_IDX>();
         auto existing = events.find(eventKey);
         eosio::check(existing != events.end(), "Event does not exist.");

         auto config = _config.get_or_default();
         eosio::check(config.signers <= existing->signers.size(), "Not enough signers.");
         eosio::check(config.execs <= existing->execs.size(), "Not enough executives.");
         eosio::check(current_time() <= existing->timestamp + config.eventTTL, "Event has expired.");
         events.erase(existing);
      }

      void Config(const sub_config_v1& config) {
         _config.set(config, _self);
      }

      void AddSigner(const std::vector<char>& signer) {
         auto signers = _signers.get_index<SUB_SHA_IDX>();
         auto existing = signers.find(eosio::sha256(signer.data(), signer.size()));
         eosio::check(existing == signers.end(), "Signer already added.");
         _signers.emplace(_self, [&](auto &row) {
            row.id = _signers.available_primary_key();
            row.signer = signer;
         });
      }
      void RemoveSigner(const std::vector<char>& signer) {
         auto signers = _signers.get_index<SUB_SHA_IDX>();
         auto existing = signers.find(eosio::sha256(signer.data(), signer.size()));
         eosio::check(existing != signers.end(), "Signer already removed or invalid.");
         signers.erase(existing);
      }
      void AddExecutive(const std::vector<char>& signer) {
         auto signers = _execs.get_index<SUB_SHA_IDX>();
         auto existing = signers.find(eosio::sha256(signer.data(), signer.size()));
         eosio::check(existing == signers.end(), "Executive already added.");
         _execs.emplace(_self, [&](auto &row) {
            row.id = _execs.available_primary_key();
            row.signer = signer;
         });
      }
      void RemoveExecutive(const std::vector<char>& signer) {
         auto signers = _execs.get_index<SUB_SHA_IDX>();
         auto existing = signers.find(eosio::sha256(signer.data(), signer.size()));
         eosio::check(existing != signers.end(), "Executive already removed or invalid.");
         signers.erase(existing);
      }
};
}