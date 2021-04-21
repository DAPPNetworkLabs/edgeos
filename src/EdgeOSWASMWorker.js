import {globalIpfsWrapper} from './IPFSRepo';
import { lowerI64Imports } from "@wasmer/wasm-transformer";
const threads = require('bthreads');
// const {TextEncoder,TextDecoder} = require('util');

const { WasmFs } = require('@wasmer/wasmfs');
const { WASI } = require('@wasmer/wasi');
const memfs_1 = require("memfs");
const {MemFS, Memory} =require('./memfs');
const bindings  = require("@wasmer/wasi/lib/bindings/browser").default;
import {
    Volume,
    // filenameToSteps,
    // DirectoryJSON,
    // TFilePath,
    // pathToFilename
  } from "memfs/lib/volume";
  
  const uint8ArrayConcat = require('uint8arrays/concat')
  const uint8ArrayString = require('uint8arrays/to-string')
  const all = require('it-all')
  


    
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

function dlog(...args){
    parent.fire('log',[{messages:["worker debug",...args]}]);
}
function getImports(imports,wasmModule){
    return {
        "edgeos":toProxies(imports),
        ...getUserImports(wasmModule)
    }
}
async function loadFileFromIPFS(path){
    await globalIpfsWrapper.ready();
    const arr = uint8ArrayString(uint8ArrayConcat(await all(globalIpfsWrapper.ipfs.cat(path))),"base64") //uint8ArrayString
    return Buffer.from(arr,"base64");
}
async function loadModuleFromIPFS(path){
    return await transformWASM(await loadFileFromIPFS(path));
}
function getUserImports(wasmModule){
    return {
        "edgeos_user":{            
            snapshot: ()=>{
                return encodeString(wasmFs.toJSON());

            },
            restore: (jsonLen,json,cb)=>{

                return 
            },
            run: (jsonLen,json,cb)=>{
                // const wasmBin = wasmFs.fs.readFileSync(command);
                // parent.fire('log',[{messages:["loading wasm", wasmBin.length]}]);
                // wasm = await transformWASM(Buffer.from(wasmBin));  

                // create wasi module in the same fs
    
                // let wasi  = new WASI({
                //     preopens: {
                //         '/':'/',
                //     },
                  
                //     env: {
                //         PWD:'/',
                //     },
                    
                //     args: processArgs ? [command, ...processArgs] : [command],
                    
                //     // OPTIONAL: The environment bindings (fs, path),
                //     // useful for using WASI in diferent environments
                //     // such as Node.js, Browsers, ...
                //     bindings: {
                //         // exit: (code?: number) -> void
                //         // kill: (pid: number, signal?: string | number) -> void
                //         // randomFillSync: (buffer: Buffer, offset?: number, size?: number) -> Buffer
                //         // isTTY: () -> bool
                //         // fs: Filesystem (with similar API interface as Node 'fs' module)
                //         // path: Path  (with similar API Interface as Node 'path' module)
                //         ...bindings,
                //         fs: wasmFs.fs,
                //         exit: (code) =>{     
                //             console.log('exit');
                //             parent.fire('exit',[code]);
                            
                //             throw new Error('exit:' + code);
                //         },
                //     }
                //     });
                // WebAssembly.instantiate(wasmModule, {
                //     ...getImports(imports, wasmModule),
                //     ...wasi.getImports(wasmModule),
                // })



                // stdin 
                // command, args
                // start
                // direct kernel callback 
                // exit callback (code, stdout, stderr)
                // restore kernel callback
                return 
            },                
        },

    }
} 
async function saveIpfsHash(wasmFs){
    const newJson = wasmFs.toJSON();
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

async function transformWASM(wasm) {
    const typedArray = new Uint8Array(wasm);
    const loweredWasmModuleBytes = await lowerI64Imports(typedArray);
    return loweredWasmModuleBytes;
}
async function loadIpfsToFS(fsIpfs, wasmFs,thememfs){
    // const sep = "/";
    const res = {};
        await globalIpfsWrapper.ready();
        wasmFs.volume = new Volume();
        // wasmFs.volume.mkdirpBase("s",0o777);
        // if(thememfs){
        //     thememfs.addDirectory('s')
        // }
        for await (const file of globalIpfsWrapper.ipfs.get(fsIpfs)) {
            // parent.fire('log',[{messages:["loading file",file.path]}]);
            let localPath = file.path.slice(fsIpfs.length);

            if(localPath.length == 0)
                continue;
            // localPath = 's' + localPath;
            localPath = localPath.slice(1);
            if (!file.content) {     
                if(thememfs){
                    thememfs.addDirectory(localPath)
                }
                
                wasmFs.volume.mkdirpBase(localPath,0o777);
                // res[localPath] = localPath;
                // dlog("map path",localPath);
                continue;
            }

            // let data = ''                
            
            // console.log(localPath)
          
        //   for await (const chunk of file.content) {
        //         data += chunk.toString()
        //     }    
            const arr = uint8ArrayConcat(await all(file.content)) //uint8ArrayString
            // dlog("loaded file",localPath);
        
            // json[localPath] = new Buffer(data);
            //   const steps = filenameToSteps(localPath);
            //   if (steps.length > 1) {
            //     const dirname = sep + steps.slice(0, steps.length - 1).join(sep);
            //     // @ts-ignore
            //     vol.mkdirpBase(dirname, 0o777);
            //   }
            wasmFs.volume.writeFileSync(localPath, uint8ArrayString(arr));
            if(thememfs){
                thememfs.addFile(localPath,arr)
            }
            // wasmFs.volume.writeFileSync(localPath, wasmFs.volume.readFileSync(localPath));
        }
        wasmFs.volume.mkdirSync("/dev");
        
        wasmFs.volume.writeFileSync("/dev/stdin",'');
        wasmFs.volume.writeFileSync("/dev/stdout",'');
        wasmFs.volume.writeFileSync("/dev/stderr",'');      
        wasmFs.fs = memfs_1.createFsFromVolume(wasmFs.volume);
        wasmFs.volume.releasedFds = [0, 1, 2];
        const fdErr = wasmFs.volume.openSync("/dev/stderr", "w");
        const fdOut = wasmFs.volume.openSync("/dev/stdout", "w");

        const fdIn = wasmFs.volume.openSync("/dev/stdin", "r");
        
        return res;
}
let ginstance;
parent.hook('init', async function({imports,process}) {
    const { args,command,fshash} =  process;
    const path = (!command || command[0] == '/') ? fshash + command : command;
    dlog("loading wasm from ",fshash,command);
    const wasmBytes = await loadModuleFromIPFS(path);
    dlog("loaded wasm from ",path);

    
    const wasmFs = new WasmFs();
    // wasm = Buffer.from(wasm);
    

    


    
    // const originalWriteFileSync = wasmFs.fs.writeFileSync;
    // wasmFs.fs.writeFileSync = (path, text, opts) => {
    //     // recalc ipfs root
    //     originalWriteFileSync(path, text, opts);
    // };
    const { fsModule } =  process;

    let preopens = {
        '/':'/'
    };
    if(!fsModule){
        dlog("loading fs",fshash);

        await loadIpfsToFS(fshash,wasmFs);
        const originalWriteFileSync = wasmFs.fs.writeFileSync;
        const originalReadFileSync = wasmFs.fs.readFileSync;
        const originalReadSync = wasmFs.fs.readSync;
        const originalfstatSync = wasmFs.fs.fstatSync;
        
        wasmFs.fs.fstatSync = (...args)=>{            
            const res =  originalfstatSync(...args);
            dlog("****fstatSync fd",args[0],args[1], JSON.stringify(res));  
            return res;
        }
        const origOpenSync = wasmFs.fs.openSync;
        wasmFs.fs.readSync = (...args)=>{            
            const res =  originalReadSync(...args);
            dlog("****reading fd",args[0], args[2], args[3],res.toString());  
            return res;
        }
        wasmFs.fs.openSync = (path, flags,mode) => {
            // recalc ipfs root
            dlog("****opening file", path);

            return origOpenSync(path, flags,mode);
            };
        wasmFs.fs.writeFileSync = (path, text,opts) => {
            // recalc ipfs root
            return originalWriteFileSync(path, text,opts);
        };        
        wasmFs.fs.readFileSync = (path,opts) => {
            // recalc ipfs root
            dlog("****reading file", path);
            return originalReadFileSync(path,opts);
        };   
    }

    WebAssembly.compile(wasmBytes).then(async wasmModule=>{
        // parent.fire('log',[{messages:["compiled wasm"]}]);
        dlog('compiled wasm');
        let thememfs;

        let wasi  = new WASI({
            preopens,          
            env: {
                PWD:'/',
            },            
            args: args ? [command, ...args] : [command],           
            bindings: {
                ...bindings,
                fs: wasmFs.fs,
                exit: (code) =>{     
                    let stderr = "";
                    let stdout = "";
                    if(thememfs){
                        const res = thememfs.hostFlush();
                        stdout = res.stdout;
                        stderr = res.stderr;
                        // dlog("result2",stdout,stderr);
                    }
                    else{
                        stdout = wasmFs.fs.readFileSync("/dev/stdout", "utf8");
                        stderr = wasmFs.fs.readFileSync("/dev/stderr", "utf8");
                    }
                    // parent.fire('log',[{messages:["stdout",response]}]);
                    parent.fire('exit',[code,stdout,stderr]);
                        // console.log(response); 
                    
                    
                    throw new Error('exit:' + code);
                },
            }});
        let imports1 = wasi.getImports(wasmModule);
        if(fsModule){

            const path = fsModule[0] == '/' ? fshash + fsModule : fsModule;
            const memFSWasmBytes = await loadModuleFromIPFS(path);
            dlog("waiting for memfs");
            thememfs = new MemFS(memFSWasmBytes,wasmFs);
            await thememfs.ready;
            dlog("memfs ready");
            if(imports1["wasi_snapshot_preview1"])
                imports1["wasi_snapshot_preview1"] = {...imports1["wasi_snapshot_preview1"],...thememfs.exports}
            else
                imports1["wasi_unstable"] = {...imports1["wasi_unstable"],...thememfs.exports}
        }

        WebAssembly.instantiate(wasmModule, {
            ...getImports(imports, wasmModule),
            ...imports1,

        }).then(async instance=>{    
            const wasmExports = instance.exports;
            ginstance = instance;
            if(thememfs){
                thememfs.hostMem = new Memory(instance.exports.memory);

            }

            if(command && fsModule){
                dlog("loading fs in fsmodule",fshash);
                try{
                    const preopensF = await loadIpfsToFS(fshash,wasmFs,thememfs);
                    // preopens = preopensF;            
                    // const wasmBin = wasmFs.fs.readFileSync('/s' + command); //{encoding:"buffer"}
                    // const wasmBuffer = Buffer.from(wasmBin);
                    // dlog("loaded wasm2",wasmBin.length,wasmBuffer.length);
                    //wasm = await transformWASM(wasmBuffer);  
                    // wasm = wasmBuffer;
    
                const originalWriteFileSync = wasmFs.fs.writeFileSync;
                const originalReadFileSync = wasmFs.fs.readFileSync;
                const originalReadSync = wasmFs.fs.readSync;
                const originalfstatSync = wasmFs.fs.fstatSync;
                
                wasmFs.fs.fstatSync = (...args)=>{            
                    const res =  originalfstatSync(...args);
                    dlog("****fstatSync fd",args[0],args[1], JSON.stringify(res));  
                    return res;
                }
                const origOpenSync = wasmFs.fs.openSync;
                wasmFs.fs.readSync = (...args)=>{            
                    const res =  originalReadSync(...args);
                    dlog("****reading fd",args[0], args[2], args[3],res.toString());  
                    return res;
                }
                wasmFs.fs.openSync = (path, flags,mode) => {
                    // recalc ipfs root
                    dlog("****opening file", path);
    
                    return origOpenSync(path, flags,mode);
                    };
                wasmFs.fs.writeFileSync = (path, text,opts) => {
                    // recalc ipfs root
                    return originalWriteFileSync(path, text,opts);
                };        
                wasmFs.fs.readFileSync = (path,opts) => {
                    // recalc ipfs root
                    dlog("****reading file", path);
                    return originalReadFileSync(path,opts);
                };        
            }
                catch(e){
                    dlog("error",e.toString());
                    throw e;
                }
                // preopens['/sysroot'] = '/s/sysroot';
                
                // preopens['/'] = '/';
                
            }            
            dlog('calling start');
            if(thememfs)
                thememfs.setStdIn("");
            wasi.start(ginstance); 
            let stderr = "";
            let stdout = "";
            if(thememfs){
                const res = thememfs.hostFlush();
                stdout = res.stdout;
                stderr = res.stderr;
                // dlog("result1",stdout,stderr);
            }
            else{
                stdout = wasmFs.fs.readFileSync("/dev/stdout", "utf8");
                stderr = wasmFs.fs.readFileSync("/dev/stderr", "utf8");
            }
            // Send back initialised message to main thread
            parent.fire('inited',[Object.keys(wasmExports),stdout,stderr]);
        });
    } ).catch(e=>{
        dlog('error',e.toString());
        // parent.fire('initerror',[{}]);

    });
    
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