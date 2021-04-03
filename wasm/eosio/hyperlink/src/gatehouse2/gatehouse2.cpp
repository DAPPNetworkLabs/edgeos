#include <ingester/EosioIngester.hpp>
#include <publisher/EosioPublisher.hpp>

namespace hyperlink {


// add expiry
class [[eosio::contract("gatehouse2")]] gatehouse2 : public eosio::contract {
    public:
        using contract::contract;

        gatehouse( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
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
            _ingester.RemoveSigner(signer);
        }
        ACTION removesigner(const std::vector<char>& signer) {        
        }

        ACTION ingest(const std::vector<char>& eventId,
                      const std::vector<char>& eventData) 
        {
            // verify is in order and didn't skip
            // if timedout, call ingestFailure
        }
        
        ACTION ingestFailure(const std::vector<char>& eventId,
                             const std::vector<char>& eventData, 
                             const std::vector<char>& eventError) 
        {
            // verify is in order and didn't skip
            // called after ingest was called and failed
            // accumulate sigs
            // if has enough, call
        }

        ACTION cron(const std::vector<char>& eventId,
                    const std::vector<char>& eventData) 
        {
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

        using transfer_action = eosio::action_wrapper<"transfer"_n, &gatehouse::transfer>;
};

}