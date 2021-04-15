import { Bootloader} from './Bootloader/NexusBootloader';

declare global {
    interface Window {  electronApi; }
}
var getParams = function (url) {
	var params = {};
	var parser = document.createElement('a');
	parser.href = url;
	var query = parser.search.substring(1);
	var vars = query.split('&');
	for (var i = 0; i < vars.length; i++) {
		var pair = vars[i].split('=');
		params[pair[0]] = decodeURIComponent(pair[1]).split(',');
	}
	return params;
};
const prms = getParams(document.location.href);
const ethEndpoint1 = prms["ethEndpoint1"][0];
const nexusAddress = prms["nexusAddress"][0];
const nexusAbi = require("../wasm/build/Nexus.abi");

const dspAccount = prms["dspAccount"][0];
const bootloader = new Bootloader(
    ethEndpoint1,            
    nexusAddress, 
    nexusAbi,
    nexusAddress,
    dspAccount
);
var appContainer = document.getElementById('appContainer');    
var loader = document.getElementById('loader');    
if(window.electronApi){
	var error = window.console.error,
	log = console.log,
	info = console.info,
	warn = console.warn;
	
	console.log = function () {
		window.electronApi.log(...arguments);
		log.apply(console, arguments);
	};
	console.log('has API!');        
}

bootloader.boot().then(a=>{
	appContainer.removeChild(loader);
	console.log("boot success");
})
