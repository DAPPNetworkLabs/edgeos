import { EdgeOSKernel } from '../EdgeOSWASMKernel'; // this will be your custom import

export interface IBootloader{
    kernel: EdgeOSKernel;   
    boot():Promise<void>;
}