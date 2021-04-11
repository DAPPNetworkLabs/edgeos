const Web3 = require('web3');

export const web3extensions = {
    "web3_subscribe":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
            const {address, eventName, ethEndpoint,onMessage,abi,fromBlock} = json;
            let web3 = new Web3(ethEndpoint);
            const theContract = new web3.eth.Contract(
                abi,
                address,
                {
                    from: address, 
                    gas: 15015, 
                    gasPrice: 500000}
              );

              theContract.events[eventName]({
                fromBlock: fromBlock || 0
              },function(error, result){
                if (error){                    
                    console.log(error);
                    return;
                }
                edgeOSKernel.workers[pid].call('callback',[{
                    cb:cb,
                    cbcb:onMessage,                    
                    result: result,
                    request:json
                }]);
            });
            return {};
    },
    "web3_contract_call":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas} = json;
        let web3 = new Web3(ethEndpoint);
        const theContract = new web3.eth.Contract(
            abi,
            address,
            {
                from, 
                gas, 
                gasPrice}
          );
        return await theContract.methods[fn].call(...args).call({
            from, 
            gas, 
            gasPrice});
    },
    "web3_estimate_gas":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas} = json;
        let web3 = new Web3(ethEndpoint);
        const theContract = new web3.eth.Contract(
            abi,
            address,
            {
                from, 
                gas, 
                gasPrice}
          );
          return await theContract.methods[fn].call(...args).estimateGas({
            from, 
            gas});
    },
    "web3_get_last_nonce":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, block, ethEndpoint} = json;
        let web3 = new Web3(ethEndpoint);
        return await web3.eth.getTransactionCount(address, block || "pending");

    },
    "web3_contract_send":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas,value} = json;
        let web3 = new Web3(ethEndpoint);
        const theContract = new web3.eth.Contract(
            abi,
            address,
            {
                from, 
                gas, 
                gasPrice}
          );
          return await theContract.methods[fn].call(...args).send({
            from, 
            gas, 
            gasPrice,value});
    },
    "web3_get_proof":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas,value} = json;
        let web3 = new Web3(ethEndpoint);
        ///web3.eth.getProof
    },
    "web3_get_uncle":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas,value} = json;
        let web3 = new Web3(ethEndpoint);
        ///web3.eth.getUncle
    },
    "web3_get_block":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas,value} = json;
        let web3 = new Web3(ethEndpoint);
        ///web3.eth.getBlock
    },
    
    "web3_get_code":async ({json, pid, cb, cbcb,edgeOSKernel})=>{ 
        const {address, args,fn, ethEndpoint,abi,from,gasPrice,gas,value} = json;
        let web3 = new Web3(ethEndpoint);
        ///web3.eth.getCode
    },
    
    // todo: ens
    
}