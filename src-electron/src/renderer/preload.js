const {
    contextBridge,
    ipcRenderer
} = require("electron");
console.log('preload');
ipcRenderer.send('log', {message:"preloaded"});
// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld(
    "electronApi", {
        log: (data) => {
            // whitelist channels         
            console.log('log',data);       
            ipcRenderer.send('log', data);
        },
        getKey: (keyName, func) => {
            ipcRenderer.send('getKey', {keyName});                
            ipcRenderer.on('getKey', (event, ...args) => func(...args));
        }
    }
);
