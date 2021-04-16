const threads = require('bthreads');
// const {TextEncoder,TextDecoder} = require('util');
const { WasmFs } = require('@wasmer/wasmfs');
const { WASI } = require('@wasmer/wasi');
let bindings = null;



// if (typeof window === 'undefined'){
//     bindings = require("@wasmer/wasi/lib/bindings/node").default;
// }
// else{
    bindings = require("@wasmer/wasi/lib/bindings/browser").default;
// }
if (!WebAssembly.instantiateStreaming) {
    WebAssembly.instantiateStreaming = async (resp, importObject) => {
            const source = await (await resp).arrayBuffer();
            return await WebAssembly.instantiate(source, importObject);
    };
}

// Create promise to handle Worker calls whilst
// module is still initialising
// let wasmResolve;
// let wasmReady = new Promise((resolve) => {
//     wasmResolve = resolve;
// })
function callback(f) {
    return ginstance.exports.__indirect_function_table.get(f);
}

function uint8Array(pos, len) {
    return new Uint8Array(ginstance.exports.memory.buffer, pos, len);
}

function decodeStr(pos, len) {
    return (new TextDecoder()).decode(uint8Array(pos, len));
}

function encodeString(str) {
    const memory = ginstance.exports.memory;
    const encoder = new TextEncoder();
    const a = encoder.encode(str);
    var c = new Uint8Array(a.length + 1);
    c.set(a, 0);
    c.set([0],a.length);
    const memoryForString = ginstance.exports.__allocate_string(c.length);
    var memoryArray = new Uint8Array(memory.buffer, memoryForString, c.length);
    for (let i = 0; i < memoryArray.length; i++) {
        memoryArray[i] = c[i];
        }
    
    return memoryForString;
};
const {parent} = threads;


function convertArgs(args){
    const realArgs = [];
    for (let index = 0; index < args.length; index++) {
        if(index == 0){
            const len = args[index]
            const pos = args[++index]
            const str = decodeStr(pos,len);
            realArgs.push(str);
        }
        else{
            realArgs.push(args[index]);
        }
    }    
    return realArgs;
}

function proxyFunc(key){
    return (...oargs)=>{
        const args = convertArgs(oargs);  
        try{
            parent.call('syscall',[{key, args}]).then(res=>{
                // const res2 = encodeString(res);
                // const cb = oargs[oargs.length-2]; 
                // const cbcb = oargs[oargs.length-1];   
                // const callbackFn = callback(cb);
    
                // callbackFn(res.length,res2,cbcb);
            });
        }
        catch(e){
            console.log(e)
            throw e;
        }
    };
}
function toProxies(importNames){
    const res = {};
    Object.keys(importNames).forEach(key=>{
        res[key] = proxyFunc(key);
    })
    return res;
}

let ginstance;
parent.hook('init', async function({wasm,imports,processArgs,fshash,command}) {
    
    const wasmFs = new WasmFs();
    // wasm = Buffer.from(wasm);
    function getImports(imports,wasmModule){
        return {
            "edgeos":toProxies(imports),
            ...getUserImports(wasmModule)
        }
    }
    function fromJSONFixed(vol, json) {
        const sep = "/";
        for (let filename in json) {
          const data = json[filename];
          const isDir = data ? Object.getPrototypeOf(data) === null : data === null;
          // const isDir = typeof data === "string" || ((data as any) instanceof Buffer && data !== null);
          if (!isDir) {
            const steps = filenameToSteps(filename);
            if (steps.length > 1) {
              const dirname = sep + steps.slice(0, steps.length - 1).join(sep);
              // @ts-ignore
              vol.mkdirpBase(dirname, 0o777);
            }
            vol.writeFileSync(filename, (data) || "");
          } else {
            // @ts-ignore
            vol.mkdirpBase(filename, 0o777);
          }
        }
      }
      async function loadIpfsToFSJSON(fsIpfs){
        if(fsIpfs){
            // read files from ipfs
            const json ={};
            for await (const file of globalIpfsWrapper.ipfs.get(fsIpfs)) {

                if (!file.content) continue;
                let data = ''                
                const localPath = file.path.slice(fsIpfs.length);
                // console.log(localPath)
              
              for await (const chunk of file.content) {
                    data += chunk.toString()

                }                
                json[localPath] = new Buffer(data);
                
            }
            return json;
            // console.log("finalJson",json)
            // this.fromJSONFixed(this.wasmFs.volume, json);
        }
    }
    
    
    async function saveIpfsHash(newJson){
        // const newJson = wasmFs.toJSON();
        // console.log("newJson",newJson)

        const filesNames = Object.keys(newJson);
        const files = filesNames.filter(a=>a.indexOf('/dev/') !== 0).map(fileName=>{
            return {
                path: `${fileName}`,
                content: newJson[fileName]
            }
        })
        
        let selected;
        for await (const result of globalIpfsWrapper.ipfs.addAll(files)) {
            selected = result;
        }
        return selected.cid.toString();
    }      
    function getUserImports(wasmModule){
        return {
            "edgeos_user":{            
                snapshot: ()=>{
                    return encodeString(wasmFs.toJSON());
    
                },
                restore: (jsonLen,json,cb)=>{
                    const jsonStr = decodeStr(json,jsonLen);
                    //
                    fromJSONFixed(wasmFs.volume, jsonStr).then(a=>{
                        callback(cb)();
                    });
                    return 
                },
            },
    
        }
    }    
    async function transformWASM(wasm) {
        const typedArray = new Uint8Array(wasm);
        const loweredWasmModuleBytes = await lowerI64Imports(typedArray);
        return loweredWasmModuleBytes;
    }
    
    const originalWriteFileSync = wasmFs.fs.writeFileSync;
    wasmFs.fs.writeFileSync = (path, text, opts) => {
        // recalc ipfs root
        originalWriteFileSync(path, text, opts);
    };
    if(fshash){
        const fsjson = await loadIpfsToFSJSON(fshash);
        await fromJSONFixed(wasmFs.volume, fsjson);
        // load wasm from command
        const wasmBin = wasmFs.fs.readFileSync(command);
        // save ipfs root
        wasm = transformWASM(Buffer.from(wasmBin));        
    }
    WebAssembly.compile(wasm).then(wasmModule=>{
        let wasi  = new WASI({
            preopens: {
                '/':'/',
            },
          
            env: {
                PWD:'/',
            },
            
            args: processArgs ? [command, ...processArgs] : [command],
            
            // OPTIONAL: The environment bindings (fs, path),
            // useful for using WASI in diferent environments
            // such as Node.js, Browsers, ...
            bindings: {
                // exit: (code?: number) -> void
                // kill: (pid: number, signal?: string | number) -> void
                // randomFillSync: (buffer: Buffer, offset?: number, size?: number) -> Buffer
                // isTTY: () -> bool
                // fs: Filesystem (with similar API interface as Node 'fs' module)
                // path: Path  (with similar API Interface as Node 'path' module)
                ...bindings,
                fs: wasmFs.fs,
                exit: (code) =>{     
                    console.log('exit');
                    parent.fire('exit',[code]);
                    throw new Error('exit:' + code);
                },
            }
            });
        WebAssembly.instantiate(wasmModule, {
            ...getImports(imports, wasmModule),
            ...wasi.getImports(wasmModule),
        }).then(instance=>{    
            const wasmExports = instance.exports;
            ginstance = instance;

            // Resolve our exports for when the messages
            // to execute functions come through
            // wasmResolve(wasmExports);
            wasi.start(ginstance);        
            // Send back initialised message to main thread
            parent.fire('inited',[Object.keys(wasmExports)]);
        });
    } );
    
});
parent.hook('callback', async function({cb,cbcb,result}) {
    const r = JSON.stringify(result);
    const res2 = encodeString(r);
    callback(cb)(r.length,res2,cbcb);
});
parent.hook('call', async function({ eventId, eventData}) {    
    try{
        const method = wasmInstance[eventData.method];
        const result = method.apply(null, eventData.arguments);
        parent.fire('callresult',[{
            eventData: result,
            eventId: eventId
        }]);
    }
    catch(e){
        parent.fire('callerror',[{
            eventData: "An error occured executing WASM instance function: " + error.toString(),
            eventId: eventId
        }]);
    }
            
                
            
});