export const logextensions = {
    "log":async({json, pid, cb, edgeOSKernel})=>{
        if(pid == 0)
            console.log('kernel:',json.message);
        else
            console.log(`${pid}:`,json.message);
        return {"done":true};   
    },
}