#include <dspConsumer/consumer.hpp>

namespace hyperlink {

class [[eosio::contract("gatehouse2")]] sample : public consumer {
    public:
        struct webRequst {
            string url;
        };
        struct cronRequst {
            string url;
        };
    ACTION test(){                
        DSPRequest(
            cronRequst{"http://google.com"},
            "cron"_n,
            "single.dsp"_n,
            _self);


        DSPRequest(
            webRequst{"http://google.com"},
            "websingle"_n,
            "single.dsp"_n,
            _self);
    }

    void invokeWebRequest(){
        DSPRequest(
            webRequst{"http://google.com"},
            "web"_n,
            "cons.dsps"_n,
            _self);        
    }

    ACTION cronresponse(){
        require_auth(permission_level(_self,"single.dsp"_n));
        // abort if too early
        // do something
    }
    ACTION webresponse(){
        require_auth( permission_level(_self,"cons.dsps"_n));        
    }
    ACTION webresponse2(name actor){
        require_auth( permission_level(_self, actor));

    }
};

}