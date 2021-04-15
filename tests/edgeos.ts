// import { Bootloader } from '../src/Bootloader/NexusBootloader'; // this will be your custom import
import { expect } from 'chai';
const proc = require('child_process');
const Web3 = require('web3');
const contract = require('truffle-contract');
const fs = require('fs');
const path = require('path');
let ethEndpoint1 = 'ws://selfhost:8545';
import {globalIpfsWrapper} from '../src/IPFSRepo'
const electron = require('electron');
const web3Evm1 = new Web3(ethEndpoint1);
// let testAddressEvm1;
// let bootloader:Bootloader;
let i =0;
async function deployNexusContracts(manifestJSON:any, masterAccountEvm1) {
    const nexusAbi = JSON.parse(
        fs.readFileSync(
            path.resolve('./wasm/build/Nexus.abi')));
    const nexusBin = 
        fs.readFileSync(
            path.resolve('./wasm/build/Nexus.bin'));
    const tokenJson = require('@openzeppelin/contracts/build/contracts/ERC20PresetMinterPauser.json');
    

    const tokenBin = tokenJson.bytecode;
    const tokenAbi = tokenJson.abi;
    const tokenContractEvm1 = contract({
        abi: tokenAbi,
        unlinked_binary: tokenBin
      });
    tokenContractEvm1.setProvider(web3Evm1.currentProvider);
    const deployedTokenEvm1 = await tokenContractEvm1.new('Test Token', 'TST', {
        from: masterAccountEvm1,
        gas: '5000000'
    });
    const minterRole = await deployedTokenEvm1.MINTER_ROLE();

    await deployedTokenEvm1.grantRole(minterRole, masterAccountEvm1, {
        from: masterAccountEvm1,
        gas: '5000000'
      });

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
      deployedTokenEvm1.address,      
    {
      from: masterAccountEvm1,
      gas: '5000000'
    });

    // setInterval(()=>{
    //     deployedNexus.run(masterAccountEvm1,JSON.stringify({
    //         "command":`hello from chain (${i++})`
    //     }), {
    //         from: masterAccountEvm1,
    //         gas: '5000000'
    //     });       
    
    // },3000);
    return {
      nexus: deployedNexus,
      token: deployedTokenEvm1
    };
  }
  function getData(rawData) {
    let lastIdx = 0;
    let data = [];
    while (true) {
      let tsIdx = rawData.indexOf("data: ", lastIdx);
      if (tsIdx == -1) break;
      let openIdx = rawData.indexOf("{", tsIdx);
      let parentheses = ["{"];
      let curIdx = openIdx + 1;
      while (parentheses.length) {
        const cur = rawData.charAt(curIdx);
        if (cur == "}") parentheses.pop();
        if (cur == "{") parentheses.push("{");
        curIdx++;
      }
      lastIdx = curIdx;
      data.push(rawData.substring(tsIdx, curIdx));
    }
    return data.join('\n') + '\n';
  }
describe('EdgeOS initialization tests', () => {     
    let deployedContracts;
    let masterAccountEvm1;
    let dspAccount;
    let consumerAccount;
    before('Nexus Bootloader', async function() {
        this.timeout(50000);

        //do something

        // testAddressEvm1 = availableAccountsEvm1[1];
        const initWASM = fs.readFileSync('./wasm/build/dsp_init.wasm');
        const kernelWASM = fs.readFileSync('./wasm/build/kernel.wasm');
        await globalIpfsWrapper.ready();
        const ipfs:any = globalIpfsWrapper.ipfs;
        if(!ipfs)
            return;
        const manifestJSON = {
            kernelWASM:(await ipfs.add(kernelWASM)).cid.toString(),
            initWASM:(await ipfs.add(initWASM)).cid.toString(),
        }    
        const availableAccountsEvm1 = await web3Evm1.eth.getAccounts();
        masterAccountEvm1 = availableAccountsEvm1[0];
        dspAccount = availableAccountsEvm1[1];
        consumerAccount = availableAccountsEvm1[2];
        deployedContracts = await deployNexusContracts(manifestJSON, masterAccountEvm1);
        // await deployedContracts.token.mint(consumerAccount, '1000000', {
        //     from: masterAccountEvm1,
        //     gas: '5000000'
        // });
        // deployedContracts.nexus.regDSP({
        //     from: dspAccount,
        //     gas: '5000000'
        // });
        
        
        const elecPath = path.resolve(__dirname, '../electron/dist/main.bundle.js');
        // const ipfsHash = fs.readFileSync(path.resolve(__dirname, '../dsp-libs/packages/apps/electron/.env'), 'utf-8').trim().split("=")[1];
        const queryString = `?ethEndpoint1=${ethEndpoint1}&nexusAddress=${deployedContracts.nexus.address}&dspAccount=${dspAccount}`;
        // bootloader = new Bootloader(
        //     ethEndpoint1,            
        //     deployedContracts.nexus.address, 
        //     deployedContracts.nexus.abi,
        //     deployedContracts.nexus.address,
        //     dspAccount
        // );
        console.log("electron=",elecPath);
        const child1 = proc.spawn(electron, [elecPath, '--no-sandbox'], { env: {
             ...process.env, 
             QUERY: queryString } });
        child1.stdout.on('data', (rawData) => {
        let data = rawData.toString('utf-8');
          console.log(data.trim());
          fs.appendFileSync('edgedsp1.log', rawData);
        });
        child1.stderr.on('data', (rawData) => {
            let data = rawData.toString('utf-8');
            console.log(data.trim());
            fs.appendFileSync('edgedsp1.log', rawData);
          });
        child1.on('error', (error) => {
          const msg = `process errored with msg ${error}\n`;
          console.log(msg);
          fs.appendFileSync('edgedsp1.log', msg);
        });
        child1.on('exit', (code) => {
          const msg = `process exited with code ${code}\n`;
          console.log(msg);
          fs.appendFileSync('edgedsp1.log', msg);
        });
        
        console.log(``);
        // await bootloader.boot();
    }),
    it('Kernel ready', async () => {   
        

        // expect(await bootloader.kernel.wasmFs.getStdOut()).to.eq("Hello World!");
    });
    it('Blockchain', async () => {   
        const ipfs:any = globalIpfsWrapper.ipfs;

        const testWasm = fs.readFileSync('./wasm/build/blockchain.wasm');
        const hash = (await ipfs.add(testWasm)).cid.toString();

        // setInterval(a=>{
            const processJson = JSON.stringify({
                "hash": hash,
                "fshash": "",
                "args": ["test"],
                "command": ``
            })
            deployedContracts.nexus.spawn(
                dspAccount,
                processJson, {
                from: consumerAccount,
                gas: '5000000'
            });      
        // }, 2000);
              
    });
    it('Bridge', async () => {   
        const ipfs:any = globalIpfsWrapper.ipfs;

        const testWasm = fs.readFileSync('./wasm/build/bridge.wasm');
        const hash = (await ipfs.add(testWasm)).cid.toString();

        // setInterval(a=>{
            const processJson = JSON.stringify({
                "hash": hash,
                "fshash": "",
                "args": ["chainA", "chainB"],
                "command": ``
            })
            deployedContracts.nexus.spawn(
                dspAccount,
                processJson, {
                from: consumerAccount,
                gas: '5000000'
            });      
        // }, 2000);
              
    });
    it('EVM Process', async () => {   
        const ipfs:any = globalIpfsWrapper.ipfs;

        const testWasm = fs.readFileSync('./wasm/build/evm.wasm');
        const hash = (await ipfs.add(testWasm)).cid.toString();

        // setInterval(a=>{
            const processJson = JSON.stringify({
                "hash": hash,
                "fshash": "",
                "args": ["hello","f86e820da4851c67c449b3825208947bee0c6d5132e39622bdb6c0fc9f16b350f0945388d02ab486cedc00008026a088b505227fac9bf1b340b4fa0f052d96fca8c101f6d5eab5e2f4d838299b3c24a063770b61e9e8b8f8657d71509cc7484ca52b8e46a12b1423d3d20d1ce2c31415"],
                "command": ``
            })
            deployedContracts.nexus.spawn(
                dspAccount,
                processJson, {
                from: consumerAccount,
                gas: '5000000'
            });      
        // }, 2000);
              
    });
});
