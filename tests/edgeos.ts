import { Bootloader } from '../src/Bootloader/NexusBootloader'; // this will be your custom import
import { expect } from 'chai';
const Web3 = require('web3');
const contract = require('truffle-contract');
const fs = require('fs');
const path = require('path');
let ethEndpoint1 = 'ws://127.0.0.1:8545';
import {globalIpfsWrapper} from '../src/IPFSRepo'

const web3Evm1 = new Web3(ethEndpoint1);
// let testAddressEvm1;
let bootloader:Bootloader;
let i =0;
async function deployNexusContracts(manifestJSON:any) {
    const nexusAbi = JSON.parse(
        fs.readFileSync(
            path.resolve('./wasm/build/Nexus.abi')));
    const nexusBin = 
        fs.readFileSync(
            path.resolve('./wasm/build/Nexus.bin'));
    const availableAccountsEvm1 = await web3Evm1.eth.getAccounts();
    const masterAccountEvm1 = availableAccountsEvm1[0];
    // const evm1Signer1 = availableAccountsEvm1[2];
    // const evm1Signer2 = availableAccountsEvm1[3];
    // const evm1Signers = [evm1Signer1, evm1Signer2];
    const nexusContract = contract({
      abi: nexusAbi,
      unlinked_binary: nexusBin
    });
    nexusContract.setProvider(web3Evm1.currentProvider);

    const deployedNexus = await nexusContract.new(
      JSON.stringify(manifestJSON),
    {
      from: masterAccountEvm1,
      gas: '5000000'
    });
    
    setInterval(()=>{
        deployedNexus.run(masterAccountEvm1,JSON.stringify({
            "command":`hello from chain (${i++})`
        }), {
            from: masterAccountEvm1,
            gas: '5000000'
        });       
    
    },3000);
    return {
      nexus: deployedNexus,
    };
  }
  
describe('EdgeOS initialization tests', () => {     
    before('Nexus Bootloader', async function() {
        this.timeout(50000);

        //do something
        const availableAccountsEvm1 = await web3Evm1.eth.getAccounts();
        // testAddressEvm1 = availableAccountsEvm1[1];
        const initWASM = fs.readFileSync('./wasm/build/initprocess.wasm');
        const kernelWASM = fs.readFileSync('./wasm/build/kernel.wasm');
        await globalIpfsWrapper.ready();
        const ipfs:any = globalIpfsWrapper.ipfs;
        if(!ipfs)
            return;
        const manifestJSON = {
            kernelWASM:(await ipfs.add(kernelWASM)).cid.toString(),
            initWASM:(await ipfs.add(initWASM)).cid.toString(),
        }    
        const deployedContracts = await deployNexusContracts(manifestJSON);
        bootloader = new Bootloader(
            ethEndpoint1, 
            deployedContracts.nexus.address, 
            deployedContracts.nexus.abi,
            deployedContracts.nexus.address);
        
        await bootloader.boot();
    }),
    it('Kernel ready', async () => {   
        
        
        expect(await bootloader.kernel.wasmFs.getStdOut()).to.eq("Hello World!");
    });
    it('Nexus', async () => {   

              
    });
});
