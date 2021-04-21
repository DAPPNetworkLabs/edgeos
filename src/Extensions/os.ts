function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
export const osextensions = {
    "spawn":async ({json, pid, cb, edgeOSKernel})=>{
        const owner = json.owner;
        const newpid= `${owner}:${json.pid}`;
        if(pid.split(':')[0] !== "system" && pid !== "0"){
            // only init and kernel can spawn processes for now
            return {
                "error":"not allowed"
            }
        }
            
        // const fshash = json.fshash;
        // const command = json.command;
        // const args = json.args;
        const wasm = json.wasm;
        const res = await edgeOSKernel.wasmWorker(
                wasm,
                newpid,
                json,
                (code,stdout, stderr)=>{                    
                    if(json.onDestroy){
                        edgeOSKernel.workers[pid].call('callback',[{
                            cb:cb,
                            cbcb:json.onDestroy,
                            result: {code, request:json,stdout,stderr},
                            request:json
                        }]);
                    }
                }
            );
        
        // add exit callback -> (for respawn)

        edgeOSKernel.processProxies[newpid] = res.proxy;
        return {
            pid:newpid
        };
    
    },    
    "schedule":async ({json, pid, cb, edgeOSKernel})=>{
            await delay(json.ms);
            return {};
    },
    "ipc_call": async ({json, pid, cb, edgeOSKernel})=>{
        // only allow communication between same owner processes
        return {
            ...(await (edgeOSKernel.processProxies[json.pid][json.method](json.message)))
        }
    
    },
    "get_pid": async ({json, pid, cb, edgeOSKernel})=>{
        return {
            pid
        }    
    }
}