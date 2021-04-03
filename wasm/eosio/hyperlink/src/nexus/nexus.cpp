#include <nexus/nexus.hpp>

namespace hyperlink {
   void nexus::init  (  config_v1 config )
   {
      eosio::require_auth(get_self());
      _config = config;
      _config_t.set(_config, get_self());
   }

   void nexus::open  (  eosio::name& owner, 
                        eosio::name& subscriber, 
                        std::vector<char>& config ) 
   {
      eosio::require_auth(owner);
      eosio::require_recipient(owner);
      eosio::require_recipient(subscriber);

      _channels.emplace(owner, [&](auto &row) {
         row.id = _channels.available_primary_key();
         row.owner = owner;
         row.subscriber = subscriber;
         row.state = std::vector<char>();
         row.config = config;
      });
   }

   void nexus::join (  uint64_t channelId, std::vector<char>& state )
   {
      auto channel = _channels.find(channelId);
      eosio::check(channel != _channels.end(), "Channel not found.");
      eosio::check(state.size() > 0, "Invalid signer.");
      eosio::require_auth(channel->subscriber);
      eosio::require_recipient(channel->owner);
      eosio::require_recipient(channel->subscriber);

      _channels.modify(channel, channel->subscriber, [&](auto &row) {
         row.state = state;
      });
   }

   void nexus::close (  uint64_t channelId )
   {
      auto channel = _channels.find(channelId);
      eosio::check(channel != _channels.end(), "Channel not found.");
      eosio::require_auth(channel->owner);
      eosio::require_recipient(channel->owner);
      eosio::require_recipient(channel->subscriber);

      _channels.erase(channel);
   }

   void nexus::submit (  uint64_t channelId,
                         eosio::name& status,
                         std::vector<char>& signature,
                         std::vector<char>& eventId, 
                         std::vector<char>& eventData  )
   {
      auto channel = _channels.find(channelId);
      eosio::check(channel != _channels.end(), "Channel not found.");
      eosio::check(channel->state.size() > 0, "Channel not joined.");
      eosio::require_auth(channel->subscriber);
      eosio::require_recipient(channel->owner);
      eosio::require_recipient(channel->subscriber);

      auto eventKey = eosio::sha256(signature.data(), signature.size());
      auto hashedEvents = _events.get_index< NEXUS_SHA_IDX >();
      auto event = hashedEvents.find(eventKey);
      eosio::check(event == hashedEvents.end(), "Already submitted");

      _events.emplace(channel->subscriber, [&](auto &row) {
         row.id = _events.available_primary_key();
         row.channelId = channelId;
         row.timestamp = current_time();
         row.status    = status;
         row.eventId = eventId;
         row.eventData = eventData;               
      });

      //clean an event for every new event relayed
      Flush(1); 
   }

  
}                       