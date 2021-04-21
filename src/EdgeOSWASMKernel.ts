import Libp2p from 'libp2p'
import Websockets from 'libp2p-websockets'
import WebRTCStar from 'libp2p-webrtc-star'
import { NOISE } from 'libp2p-noise'
import Mplex from 'libp2p-mplex'
import Bootstrap from 'libp2p-bootstrap'
// const multiaddr = require('multiaddr')
const PeerId = require('peer-id')
var toString = require('stream-to-string');
const KadDHT = require('libp2p-kad-dht')
// const pipe = require('it-pipe')
// const uint8ArrayConcat = require('uint8arrays/concat')
// const uint8ArrayToString = require('uint8arrays/to-string')

import Gossipsub from 'libp2p-gossipsub'
import {globalIpfsWrapper} from './IPFSRepo';
import {
    Volume,
    filenameToSteps,
    DirectoryJSON,
    TFilePath,
    pathToFilename
  } from "memfs/lib/volume";
import { WASI } from "@wasmer/wasi"
import { WasmFs } from "@wasmer/wasmfs"
import { lowerI64Imports } from "@wasmer/wasm-transformer";
const threads = require('bthreads');
let ginstance;
import { extensions } from './Extensions'
const bindings = require("@wasmer/wasi/lib/bindings/browser").default;
function callback(f) {
    return ginstance.exports.__indirect_function_table.get(f);
}

function uint8Array(pos, len) {
    return new Uint8Array(ginstance.exports.memory.buffer, pos, len);
}

function decodeStr(pos, len) {
    return (new TextDecoder()).decode(uint8Array(pos, len));
}
function cstrLen(pos) {
    let len = 0;
    const mem = new Uint8Array(ginstance.exports.memory.buffer, pos);
    while(ginstance.exports.memory.buffer.byteLength > (pos+len) && mem[len] !== 0){
        len++;
    }
    return len;
}


function dlog(...args){
    console.log("debug",...args);
}
  
const fulldefs = {   
    // IPC
    // pubsub between processes
    // express (http server)
    // eosio
    // extensions:    
    // 
    ...extensions    
}
let definitions = {};
let kernelJSFuncs = {};
Object.keys(fulldefs).forEach(k=>{
    const fulldef = fulldefs[k];
    definitions[k] = {
    };
    kernelJSFuncs[k] = fulldef;
})

function callKernelJSFunc(key,oargs,edgeOSKernel){
    try{
        const func = kernelJSFuncs[key];
        const args = convertArgsToJS(oargs);
        dlog("calling", key);
        let ores = func({
            json:args[0],
            pid:args[1],
            cb:args[2],
            cbcb:args[3],
            edgeOSKernel});
        let res = ores;

        res.then(a=>{
            const pid = args[1]
            const cb = oargs[oargs.length-2]; 
            const cbcb = oargs[oargs.length-1]; 
            a.request = args[0];
            // send callback to original thread
            if(pid == "0"){
                const res2 = JSON.stringify(a);
                const ptra = encodeString(res2);
                // dlog("kcb",key,res2.length,cb,cbcb);
                try{
                    const callbackFn = callback(cb);
                    callbackFn(res2.length,ptra,cbcb);
                }
                catch(e){
                    console.log("e",e)
                    throw e;
                }
            }
            else{
                // dlog("pcb",pid,key,cb,cbcb);
                edgeOSKernel.workers[pid].call('callback',[{cb,cbcb,result:a}]);
            }
        }).catch(e=>{
            console.log(e);
        });
    
    }
    catch(e){
        console.log(e);
        throw e;
    }
    return true;
}
function getKernelImports(edgeOSKernel:EdgeOSKernel){
    const edgeosObj = {};
    const keys = Object.keys(definitions);
    keys.forEach(key=>{
        edgeosObj[key]=(...args)=>{
                return callKernelJSFunc(key,args,edgeOSKernel);
        }        
    })
    return {
        "edgeos": edgeosObj,
    }
}

function convertArgsToJS(args){
    const realArgs = [];    
    for (let index = 0; index < args.length; index++) {
        // const arg = definition.args[index];        
        if(index < 4){
            const len = args[index]
            const pos = args[++index]
            let str = decodeStr(pos,len); 
            if(index == 1)           
                 str = JSON.parse(str)
            realArgs.push(str);
        }
        else{
            realArgs.push(args[index]);
        }
    }
    // if(definition.kernelCallback){
    // realArgs.push(args[args.length-1]);
    // }
    return realArgs;
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
function convertArgsWasm(args,pid){
    const realArgs = [];
    for (let index = 0; index < args.length-2; index++) {
        // const arg = definition.args[index];        
        if(index == 0){
            const strPos = encodeString(args[index]);
            realArgs.push(args[index].length);
            realArgs.push(strPos);
        }
        else{
            realArgs.push(args[index]);
        }
    }
    const pidStrPos = encodeString(pid);
    realArgs.push(pid.length);
    realArgs.push(pidStrPos);
    realArgs.push(args[args.length-2]);
    realArgs.push(args[args.length-1]);

    return realArgs;
}
async function transformWASM(wasm) {
    const typedArray = new Uint8Array(wasm);
    const loweredWasmModuleBytes = await lowerI64Imports(typedArray);
    return loweredWasmModuleBytes;
}

export class EdgeOSKernel{    
    wasmFs: WasmFs;
    workers:any={};
    extensionObjects:any = {
        
    };
    processProxies:any={};
    wasi: WASI;
    kernelWASM: any;
    wasmModule: WebAssembly.Module;
    instance: WebAssembly.Instance;
    wasmbytes: Uint8Array;
    wasiImports: Record<string, Record<string, Function>>;
    pid: number = 0;
    _peerId: any
    libp2p: Libp2p
    initOpts: any
    constructor({
        kernelWASM,
        initOpts
    }){
        this.initOpts = initOpts;
        const wasmFs = new WasmFs();
        this.wasmFs = wasmFs;  
        // const originalWriteFileSync = wasmFs.fs.writeFileSync;
        // wasmFs.fs.writeFileSync = (path, text,opts) => {
        //     dlog('File written:', path);
        //     originalWriteFileSync(path, text,opts);
        // };
        try{
            this.wasmFs.fs.mkdirSync('/s');
        }catch(e){}

        this.kernelWASM = kernelWASM;
             
    }
    async init(){
        await globalIpfsWrapper.ready();        
        this.wasmbytes = await transformWASM(this.kernelWASM);
        this.wasmModule = await WebAssembly.compile(this.wasmbytes);        
        let wasi  = new WASI({
            preopens: {
                '/':'/',
                '/s':'/s',
            },
          
            env: {
                PWD:'/',
            },
          
            // todo: replace with reading manifest.
            args: [JSON.stringify(this.initOpts)],
          
            
            bindings: {

              // kill: (pid: number, signal?: string | number) -> void
              // isTTY: () -> bool
              // fs: Filesystem (with similar API interface as Node 'fs' module)
              // path: Path  (with similar API Interface as Node 'path' module)
              ...bindings,
              fs: this.wasmFs.fs,
              exit: (code?: number) =>{                  
                console.log("kernel exit signal");
              },
              // hrtime: (time?: [number, number]) -> number
              // to override with seed
              // randomFillSync: (buffer: Buffer, offset?: number, size?: number) -> Buffer
            }
          });
          
        this.wasi = wasi;   
        this.wasiImports = this.wasi.getImports(this.wasmModule);

        this.instance = await WebAssembly.instantiate(this.wasmModule, {
            ...this.wasiImports,
            ...getKernelImports(this)
        });        
        ginstance = this.instance;
        this.wasi.start(this.instance);     
        
        this._peerId = await PeerId.create({ bits: 1024, keyType: 'rsa' });
        this.libp2p = await Libp2p.create({
            peerId: this._peerId,
            addresses: {
              // Add the signaling server address, along with our PeerId to our multiaddrs list
              // libp2p will automatically attempt to dial to the signaling server so that it can
              // receive inbound connections from other peers
              listen: [
                '/dns4/wrtc-star1.par.dwebops.pub/tcp/443/wss/p2p-webrtc-star',
                '/dns4/wrtc-star2.sjc.dwebops.pub/tcp/443/wss/p2p-webrtc-star'
              ]
            },
            modules: {
              transport: [Websockets, WebRTCStar],
              connEncryption: [NOISE],
              streamMuxer: [Mplex],
              peerDiscovery: [Bootstrap],
              dht:KadDHT,
              pubsub:Gossipsub
            },
            config: {
              peerDiscovery: {
                // The `tag` property will be searched when creating the instance of your Peer Discovery service.
                // The associated object, will be passed to the service when it is instantiated.
                [Bootstrap.tag]: {
                  enabled: true,
                  list: [
                    '/dns4/ams-1.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLer265NRgSp2LA3dPaeykiS1J6DifTC88f5uVQKNAd',
                    '/dns4/lon-1.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLMeWqB7YGVLJN3pNLQpmmEk35v6wYtsMGLzSr5QBU3',
                    '/dns4/sfo-3.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLPppuBtQSGwKDZT2M73ULpjvfd3aZ6ha4oFGL1KrGM',
                    '/dns4/sgp-1.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLSafTMBsPKadTEgaXctDQVcqN88CNLHXMkTNwMKPnu',
                    '/dns4/nyc-1.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLueR4xBeUbY9WZ9xGUUxunbKWcrNFTDAadQJmocnWm',
                    '/dns4/nyc-2.bootstrap.libp2p.io/tcp/443/wss/p2p/QmSoLV4Bbm51jM9C4gDYZQ9Cy3U6aXMJDAbzgu2fzaDs64'
                  ]
                },
                dht: {
                  enabled: true,
                  randomWalk: {
                    enabled: true
                  }
                }
              }
            }
          });
    }
    async loadIpfsToFSJSON(fsIpfs){
        if(fsIpfs){
            // read files from ipfs
            const json ={};
            for await (const file of globalIpfsWrapper.ipfs.get(fsIpfs)) {

                if (!file.content) continue;
                let data = ''                
                const localPath = file.path.slice(fsIpfs.length);
                dlog(localPath)
              
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
    fromJSONFixed(vol: any, json: any) {
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
            vol.writeFileSync(filename, (data as any) || "");
          } else {
            // @ts-ignore
            vol.mkdirpBase(filename, 0o777);
          }
        }
      }
    
    async saveIpfsHash(newJson){
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
    wasmWorker(modulebase64, pid, json, onDestroy) {
        // Create an object to later interact with 
        const proxy = {};
        // Keep track of the messages being sent
        // so we can resolve them correctly
        let id = 0;
        let idPromises = {};
        const k=this;
        return new Promise(async (resolve, reject) => {
            // console.log("modulePath",modulePath);
            const worker = new threads.Thread('./EdgeOSWASMWorker.js');
            this.workers[pid] = worker;
            worker.bind('inited', function(methods, stdout, stderr) {
                if(stdout || stderr)
                    dlog("stdout,stderr", stdout, stderr);
                methods.forEach((method) => {
                    proxy[method] = function(...args) {
                        return new Promise((resolve, reject) => {
                            worker.call('call',[{
                                eventData: {
                                    method: method,
                                    arguments: Array.from(args) // arguments is not an array
                                },
                                eventId: id
                            }]);

                            idPromises[id] = { resolve, reject };
                            id++
                        });
                    }
                });
                resolve({proxy,pid});
                return;
            });
            worker.bind('callresult', function({ eventData, eventId } ) {
                if (eventId !== undefined && idPromises[eventId]) {
                    idPromises[eventId].resolve(eventData);
                    delete idPromises[eventId];
                }
            });
            worker.bind('callerror', function({ eventData, eventId } ) {            
                if (eventId !== undefined && idPromises[eventId]) {
                    idPromises[eventId].reject(eventData);
                    delete idPromises[eventId];
                }
            });
            worker.bind('exit', function(code,stdout,stderr) {
                worker.close();
                dlog('process exited', pid, code,stdout,stderr );
                if(onDestroy)
                    onDestroy(code,stdout,stderr);
            });
            worker.bind('log', function({messages}) {
                console.log(...messages);
            });

            // worker.on('message', console.log);
            // worker.on('error', console.error);
            worker.hook("syscall",function({key, args}){
                
                // transform args to wasm            

                try{  
                    dlog("syscall", key);
                    const wargs = convertArgsWasm(args,pid);
                    const method:any = k.instance.exports["edgeos_syscall_"+key];
                    

                    if(!method){
                        console.log('syscall not found:', key);
                        throw new Error('syscall not found: ' + key);
                    }          
                    let result = method(...wargs);
                    result = decodeStr(result,cstrLen(result));
                    return result;
                }
                catch(e){
                    console.log("err",e);
                    throw e;
                }
                
                
            })
            worker.on('exit', (code,stdout,stderr) => {
                if (code !== 0)
                dlog(`Worker stopped with exit code ${code} ${stdout} ${stderr}.`);
            });    
            worker.on("error", function(error) {
                reject(error);
            });
            async function warmpupIpfs(fshash){
                for await (const file of globalIpfsWrapper.ipfs.get(fshash)) {
                    // dlog("warming up",file.path);
                    if (!file.content) {
                        continue;
                    }                    
                    let total = 0;
                    for await (const chunk of file.content) {
                        const len = chunk.length;
                        total += len;
                        // dlog(`${total / (1024 * 1024)} MB`);
                    }
                    // if(!file.content)
                    //     await warmpupIpfs(file.path)
                }
            }
            let p;
            // if(fshash)
            //     p = Promise.resolve(null);
            // else   
                // p = transformWASM(Buffer.from(modulebase64,"base64"));
            // p.then(async (workerBytes)=>{
                if(json.fshash){
                    dlog("warming up fs");
                    await warmpupIpfs(json.fshash);
                }
                worker.call('init',[{
                    // wasm: workerBytes, 
                    imports:definitions,
                    process:json
                }]);
    
            // })

        })
        

    }
}
