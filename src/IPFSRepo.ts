import Libp2p from 'libp2p'
import Websockets from 'libp2p-websockets'
import WebRTCStar from 'libp2p-webrtc-star'
import { NOISE } from 'libp2p-noise'
import Mplex from 'libp2p-mplex'
import Bootstrap from 'libp2p-bootstrap'
const PeerId = require('peer-id')
const KadDHT = require('libp2p-kad-dht')
const CID = require('cids');
const IPFS = require('ipfs');
function Deferred() {
    var self = this;
    this.promise = new Promise(function(resolve, reject) {
      self.reject = reject
      self.resolve = resolve
    })
  }
let globalIpfs = null;
var dfd = new Deferred()

let globalIpfsWrapper = {
    ipfs:null,
    ready() {
        if(globalIpfsWrapper.ipfs)
            return Promise.resolve(globalIpfsWrapper.ipfs);        

        return dfd.promise;
   }
}

async function initRepo(){
    
    // const _peerId = await PeerId.create({ bits: 1024, keyType: 'rsa' });
    const ipfs  = await IPFS.create({
        // peerId: _peerId,
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
            dht: KadDHT
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
                }
            }
        }        
    });   
    globalIpfs = ipfs;    
    globalIpfsWrapper.ipfs = ipfs;
    dfd.resolve(globalIpfs);
}


function validatePath(path){
    if (path.indexOf('..') !== -1)
        throw new Error('not allowd to touch upper dirs');
    
    return path.replace(/^\/+/g, '');
}

class ForkableFSHandler {
    ipfs: any;
    key: string;
    initialState: any;
    lastState:any;
    inited: boolean;
    fsPath: string
    constructor(key,state = null){
        if(!globalIpfs)
            throw new Error('ipfs not ready');
        this.ipfs = globalIpfs;        
        this.key = key;
        this.initialState = state;
        this.fsPath = `/channel/${key}`;
    }
    async checkState(){
        this.inited = true;
        if(!this.inited)
            this.loadState(this.initialState)
        // return this.loadState(this.lastState);
    }
    async loadState(stateLink){
        if(await this.flush() != stateLink)
            return;
        await this.ipfs.files.rm(this.fsPath, { recursive: true });
        if(stateLink)
            await this.ipfs.files.cp(`/ipfs/${stateLink}`, this.fsPath, { parents:true });        
        await this.flush();;
    }
    async flush(){
        await this.checkState();
        try{
            await this.ipfs.files.stat(this.fsPath);
        }
        catch(e){
            await this.ipfs.files.mkdir(this.fsPath, {parents:true});
        }
        let flushRes =  (await this.ipfs.files.flush(this.fsPath)).string;
        if(flushRes == 'QmUNLLsPACCz1vLxQVkXqqLX5R1X345qqfHbsf67hvA3Nn')
            flushRes = undefined;
        this.lastState = flushRes;
        return this.lastState;
    }
    async chmod(...args){
        const path = args[0];
        validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.chmod(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async cp(pathFrom, pathTo, options = {}){        
        pathFrom = validatePath(pathFrom);
        pathTo = validatePath(pathTo);
        await this.checkState();
        const res= await this.ipfs.files.cp(`${this.fsPath}/${pathFrom}`, `${this.fsPath}/${pathFrom}`,options);
        await this.flush();
        return res;            

    }
    async import(ipfsHash, pathTo, options = {}){
        pathTo = validatePath(pathTo);
        await this.checkState();
        const res= await this.ipfs.files.cp(`/ipfs/${ipfsHash}`, `${this.fsPath}/${pathTo}`,options);
        await this.flush();
        return res;            

    }
    async mkdir(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.mkdir(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async stat(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.stat(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async rm(...args){
        const path = args[0];
        validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.rm(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async read(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.read(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        return res;                
    }
    async touch(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.touch(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async write(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.write(`${this.fsPath}/${path}`, 
            ...args.slice(1));
        await this.flush();
        return res;            
    
    }
    async mv(...args){
        let path = args[0];
        path = validatePath(path);
        let path2 = args[1];
        path = validatePath(path2);
        await this.checkState();
        const res= await this.ipfs.files.mv(`${this.fsPath}/${path}`,`${this.fsPath}/${path2}`, 
            ...args.slice(2));
        await this.flush();
        return res;            
    }
    async ls(...args){
        let path = args[0];
        path = validatePath(path);
        await this.checkState();
        const res= await this.ipfs.files.ls(`${this.fsPath}/${path}`,
            ...args.slice(1));
        return res;
    }    
}

initRepo().then(a=>console.log('repo inited'));
export {ForkableFSHandler,globalIpfsWrapper};
