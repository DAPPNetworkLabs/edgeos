import {globalIpfsWrapper} from '../IPFSRepo';
const uint8ArrayConcat = require('uint8arrays/concat')
const uint8ArrayString = require('uint8arrays/to-string')
const all = require('it-all')

export const ipfsextensions={
    "ipfs_loadfs":async ({ json, pid, cb, edgeOSKernel})=>{
            return await edgeOSKernel.loadIpfsToFSJSON(json.hash);
        
    },
    "ipfs_savefs":async ({ json, pid, cb, edgeOSKernel})=>{
            return await edgeOSKernel.saveIpfsHash(json);
        
    },
    "ipfs_read":async({ json, pid, cb, edgeOSKernel})=>{
            return {
                bytes:uint8ArrayString(
                uint8ArrayConcat(await all(globalIpfsWrapper.ipfs.cat(json.hash)))
                ,"base64"
                )
            }
        
    },
    "ipfs_write":async ({ json, pid, cb, edgeOSKernel})=>{
        
    },

}
