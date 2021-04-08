#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <syscall.h>
#include <iostream>

#include "../include/libedgeos.h"

class Bridge {
    public:
    int nonce;
    Bridge(const char * from, const char * to){

    }
    void get_latest_incoming_messages(){
            // auto nextInboundEvm1MessageId = linkContract1.methods.next_incoming_message_id().call();
            // // chain1.next_incoming_message_id

            // // save to state
            
            // // nextInboundEvm1MessageId
            // if (nextInboundEvm1MessageId < this.current_state.nextInboundEvm1MessageId) {
            //     // wait until state updates, we are ahead. TODO: logger
            //     return;
            // }


            // const outboundEvm2Message = linkContract2.methods.getOutboundMessage(nextInboundEvm1MessageId).call();
            // //return { ...outboundEvm2Message, id: nextInboundEvm1MessageId };
            // return outboundEvm2Message;
    }

    void push_trx(char * message, int nonce, void (*f)(void *)){
        // const web3 = this.current_state.web3Evm1;
        // const medGasPrice = await web3.eth.getGasPrice();
        // const gasPrice = medGasPrice * this.current_state.gasPriceMultiplier;
        // const privateKey = this.current_state.evm1PrivateKey;
        // const linkContract = new web3.eth.Contract(link.abi, this._config.evm1ContractAddress)
        // this._dspFacilities.log("INFO", { message: "building raw tx evm1", msg: message });
        // const trxData = linkContract.methods.pushInboundMessage(message.id, message.message).encodeABI();
        // let gasLimit;
        // try {
        //     gasLimit = await linkContract.methods.pushInboundMessage(
        //         message.id,
        //         message.message
        //     ).estimateGas({ from: this._state.account1 });
        //     this._dspFacilities.log("INFO", { message: `gas limit: ${gasLimit}` });
        // } catch(e) {
        //     this._dspFacilities.log("ERROR", { message: e.message });
        //     return;
        // };
        // gasLimit = this.numberToHex(Math.round(parseInt(gasLimit) * 1.5));
        // const rawTx = {
        //     nonce: this.numberToHex(nonce),
        //     to: this._config.evm1ContractAddress,
        //     data: trxData,
        //     value: this.numberToHex(0),
        //     gasPrice: gasPrice.toString(),
        //     gas: gasLimit
        // }
        // this._dspFacilities.log("INFO", { message: "pushing raw tx evm1", rawTx });
        // const tx = await web3.eth.accounts.signTransaction(rawTx, privateKey);
        // this._state.nonce1++;
        // return web3.eth.sendSignedTransaction(tx.rawTransaction);
    }
    void push_transactions();
    void start();
};
Bridge * bridge_singleton;

void Bridge::push_transactions(){
    // auto messages = get_latest_incoming_messages();


    // if (messageRes && messageRes.id && messageRes.message) {
        // auto message = { message: messageRes.message, id: messageRes.id };
        // auto txHash = push_trx(message, nonce,[](){

        // });        
        // return txHash;
    // }
    // read from chain A, sign and post on chain B
    // read from chain B, sign and post on chain A        
    
    // edgeos_schedule(60*1000, [](){
    //     bridge_singleton->push_transactions();            
    // });
}
void Bridge::start(){
    auto b = this;
    // edgeos_schedule(60*1000, [](){
    //     bridge_singleton->push_transactions();            
    // });
}

int main(int argc, const char **argv){
    // read nonce from state
    const char * from = argv[0];
    const char * to = argv[1];
    bridge_singleton = new Bridge(from,to);
    bridge_singleton->push_transactions();
    elog("bridge process running: " + std::string(from) + " " + std::string(to));

    
    return 0;
}
