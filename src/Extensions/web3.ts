const Web3 = require('web3');

export const web3extensions = {
    "web3_subscribe":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ // type, address, topic,
            if(!edgeOSKernel.extensionObjects.web3){
                edgeOSKernel.extensionObjects.web3 = {
                    connections:{},
                    cid:0
                };
            }
            let web3 = new Web3(json.ethEndpoint);
            let address = json.address;
            const myContract = new web3.eth.Contract(
                json.abi,
                address,
                {
                    from: address, 
                    gas: 15015, 
                    gasPrice: 500000}
              )
              myContract.events[json.eventName]({
                fromBlock: 0
              },function(error, result){
                if (error){                    
                    console.log(error);
                    return;
                }
                edgeOSKernel.workers[pid].call('callback',[{
                    cb:cb,
                    cbcb:json.onMessage,
                    // pass remove event
                    result: result
                }]);                    
            });
            // send removed
            const cid = edgeOSKernel.extensionObjects.web3.cid++;
            edgeOSKernel.extensionObjects.web3.connections[cid] = web3;
            return {cid:cid.toString()};        
    },
    "web3_contract_call":{

    },
    "web3_estimate_gas":{
    },
    "web3_get_last_nonce":{
    },
    "web3_contract_send":{
    },
}