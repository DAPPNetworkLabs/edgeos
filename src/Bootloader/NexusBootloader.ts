import { EdgeOSKernel } from '../EdgeOSWASMKernel'; // this will be your custom import
import {IBootloader} from './IBootloader'
import {globalIpfsWrapper} from '../IPFSRepo'
const Web3 = require('web3');
const fs = require('fs');
const uint8ArrayConcat = require('uint8arrays/concat')
const all = require('it-all')

export class Bootloader implements IBootloader{
    kernel: EdgeOSKernel;   
    ethEndpoint: any;
    address: any;
    abi: any;
    topic: any;
    dspAddress: any;
    constructor(ethEndpoint: string, address: any, abi:any,topic, dspAddress) {
        this.ethEndpoint = ethEndpoint;
        this.address = address;
        this.abi = abi;
        this.topic = topic;
        this.dspAddress = dspAddress;
    }
    async ge(){

    }
    async boot(){
        await globalIpfsWrapper.ready();       
        const ipfs:any = globalIpfsWrapper.ipfs;
        if(!ipfs)
            return;

        // read from nexus
        let web3 = new Web3(this.ethEndpoint);
        const contract = new web3.eth.Contract(this.abi, this.address)        
        const manifestJSON = JSON.parse(await contract.methods.OSManifest().call());
        console.log("Bootloader",manifestJSON)
        const kernelWASM = uint8ArrayConcat(await all(ipfs.cat(manifestJSON.kernelWASM)))
        // const initWASM = uint8ArrayConcat(await all(globalIpfsWrapper.ipfs.cat(manifestJSON.initWASM)))

        this.kernel = new EdgeOSKernel({
            kernelWASM,
            // fsipfsHash:"",
            initOpts:{
                wasm:manifestJSON.initWASM,
                dspAddress: this.dspAddress,
                nexus: {
                    ethEndpoint:this.ethEndpoint,
                    address:this.address,
                    topic:this.topic,
                    abi:this.abi,
                }
            }
        }); 
        // manifestJSON
        await this.kernel.init();
    }
    
}
