#include <ingester/EosioIngester.hpp>
#include <publisher/EosioPublisher.hpp>

namespace hyperlink {

class [[eosio::contract("ethtokenpegnexus")]] ethtokenpeg : public eosio::contract {
    public:
        using contract::contract;

        ethtokenpegnexus( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
        : contract(receiver, code, ds), 
        _ingester(receiver),
        _publisher(receiver) {}

        HYPERLINK_INGESTER_ABI

        HYPERLINK_PUBLISHER_ABI

        EosioIngester _ingester;
        EosioPublisher _publisher;

        struct eventContent {
            bool success;
            eosio::name from;
            eosio::name to;
            uint64_t value;
        };

        ACTION addsigner(const std::vector<char>& signer) {
            eosio::require_auth(get_self());
            _ingester.AddSigner(signer);
        }

        ACTION ingest(const std::vector<char>& signer,
                      const std::vector<char>& signature,
                      const std::vector<char>& eventHash) 
        {
            _ingester.Ingest(signer,signature,eventHash);
        }

        ACTION digest(const std::vector<char>& eventData)
        {
            _ingester.Digest(eventData);
            auto event = eosio::unpack<eventContent>(eventData);
            transfer_action{get_self(), {get_self(), "active"_n}}.send(get_self(),event.to,event.value);
            auto receipt = eosio::pack(eventContent{true, event.from, event.to, event.value});
            _publisher.Publish(receipt);
        }

        ACTION transfer(eosio::name from, eosio::name to, uint64_t value) {
            eosio::require_auth(from);
            eosio::require_recipient(from);
            eosio::require_recipient(to);
            if(from != get_self()) {
                auto event = eosio::pack(eventContent{false,from,to,value});
                _publisher.Publish(event);
            }            
        }

        using transfer_action = eosio::action_wrapper<"transfer"_n, &ethtokenpegnexus::transfer>;
};

}
