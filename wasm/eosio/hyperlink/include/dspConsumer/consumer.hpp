#pragma once

#include <eosio/eosio.hpp>
#include <eosio/string.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>
#include <eosio/action.hpp>
#include <eosio/permission.hpp>

using namespace eosio;
namespace hyperlink {

class consumer : public contract {
    public:
        using contract::contract;

        consumer( name receiver, name code, datastream<const char*> ds )
        : contract(receiver, code, ds),
        _requests(receiver,receiver.value) {}
 

      struct [[eosio::table("dsprequest")]] dsprequest {
         uint64_t id;
         std::vector<char> requestData;
         name requestType;
         name permission;
         uint64_t primary_key() const { return id; }
         uint64_t by_request_type() const { return requestType.value; }         
      };

    typedef eosio::multi_index< "dsprequest"_n, dsprequest
          eosio::indexed_by< "byrtype", eosio::const_mem_fun<dsprequest, uint64_t, &dsprequest::by_request_type>>, 
    > requests_t;
    requests_t _requests;
    template <class T>
    void DSPRequest(T requestData, name type, name permission, name owner){
        _requests.emplace(owner, [&](auto &row) {
            row.id = _requests.available_primary_key();
            row.requestData = pack(requestData);
            row.requestType = type;
            row.permission = permission;
        });
    }

    // called only by msig of the active channels @dsps
    ACTION dspresponse(name permission, const std::vector<char>& responseActionData, int64_t id)
    {
        require_auth( permission_level(_self, permission ) );
        if(id != -1){
            auto existing = _requests.find(id);
            eosio::check(existing != _requests.end(), "cannot find request");
            eosio::check(permission == existing->permission, "invalid permissions");            
            _requests.erase(existing);
        }
        std::vector<eosio::action> actions = unpack<std::vector<eosio::action>>(responseActionData);
        for (const auto& act : actions) {
            if(allowAction(act, permission, id))
                act.send();
        }
    }
    virtual bool allowAction(action act, name permission, name requestType, int64_t id) {
        return true;
    }
};
    
}

/// channels for each dsp:
// cron
// sigs combiner (relayer)
// signers: custom functionality
// 


// setup permissions
//  takes state.keys from channels and set as keys for the proper permissions

// modify dsps
//  create new channels
//  setup permissions
//  kill old channels