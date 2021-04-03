export const web3extensions = {
    "eosio_read":{
        args:["string","string","string","string"], 
        callbackType: "int",
        kernelCallback: true,
        fn:async (uri, type, address, topic, edgeOSKernel)=>{
        }
    },
    "eosio_call":{
        args:["string","string","string","string","string"], 
        callbackType: "int",
        kernelCallback: true,
        fn:async (uri,jsonInterface,address,method,from, edgeOSKernel)=>{
        }
    },
    "eosio_getinfo":{
        args:["string","string","string","string","string"], 
        callbackType: "int",
        kernelCallback: true,
        fn:async (uri,jsonInterface,address,method,from, edgeOSKernel)=>{
        }
    },
}