export const osextensions = {
    "spawn":async ({json, pid, cb, edgeOSKernel})=>{
        const res = await edgeOSKernel.wasmWorker(json.wasm,
            [JSON.stringify(edgeOSKernel.initOpts)]);            
        edgeOSKernel.processProxies[res.pid] = res.proxy;
        return {
            pid:res.pid
        };
    
    },

    "schedule":async ({json, pid, cb, edgeOSKernel})=>{
            await delay(json.ms);
            return {};
    },
    "ipc_call": async ({json, pid, cb, edgeOSKernel})=>{
        // parse call from abi
        // get proxy object
        return {
            "result":await (edgeOSKernel.processProxies[pid][json.method](json.message))
        }
        // run command in remote process
        // return callback
    
    },
    "readfile": async ({pid, method, message},cb, edgeOSKernel)=>{
    },
    "writefile": async ({pid, method, message},cb, edgeOSKernel)=>{
    },
}