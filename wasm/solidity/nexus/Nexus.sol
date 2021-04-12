pragma solidity >=0.7.0 <0.9.0;
import "../../../node_modules/@openzeppelin/contracts/access/Ownable.sol";
import "../../../node_modules/@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";

contract Nexus is Ownable {
    using SafeERC20 for IERC20;    
    uint256 public gasPerTimeUnit = 100;
    IERC20 public token;

    event BoughtGas(
        address indexed consumer,
        address indexed dsp,
        uint256 amount
    );

    event SoldGas(
        address indexed consumer,
        address indexed dsp,
        uint256 amount
    );
    event ClaimedGas(
        address indexed consumer,
        address indexed dsp,
        uint256 amount
    );
    
    event UsedGas(
        address indexed consumer,
        address indexed dsp,
        uint256 amount
    );
    event Spawn(
        address indexed consumer,
        address indexed dsp,
        string processJSON,
        uint256 id
    );

    event Kill(
        address indexed consumer,
        address indexed dsp,
        uint256 id
    );

    event DSPStatusChanged(
        address indexed dsp,
        bool active
    );



    struct PerConsumerDSPEntry {
        uint256 amount;
        uint256 lastUsage; 
        uint256 claimable;
        mapping(uint256 => bool) processes;
        uint256 processesCount;
        uint256 lastProcessId;
    }

    struct RegisteredDSP {
        bool active;
    }
    

    string public OSManifest;
    mapping(address => RegisteredDSP) public registeredDSPs;
    mapping(address => mapping(address => PerConsumerDSPEntry)) public dspData;


    constructor (
        string memory manifest,
        address _tokenContract
        ) {
        OSManifest = manifest;
        token = IERC20(_tokenContract);
    }
    
    // function setQuorum(address[] dsps) public {
    //     consumerData[msg.sender].dsps = dsps;        
    // }

    // function setConsumerPermissions(address[] dsps) public {
    //     consumerData[msg.sender].owner = dsps;
    // }

    // holds snapshots
    function buyGasFor(
        uint256 _amount,
        address _consumer,
        address _dsp
    ) public {
        if(!registeredDSPs[_dsp].active){
            // block new buys
        }
        
        token.safeTransferFrom(msg.sender, address(this), _amount);
        // increase reserve for user->dsp
        applyUsage(_consumer, _dsp);
        
        dspData[_consumer][_dsp].amount += _amount;
        emit BoughtGas(_consumer, _dsp, _amount);
    }
    function sellGas(
        uint256 _amountToSell,
        address _dsp
    ) public {
        address _consumer = msg.sender;                 
        applyUsage(_consumer, _dsp);
        // decrease reserve for user->dsp
        if(_amountToSell > dspData[_consumer][_dsp].amount){
            // throw
        }
        dspData[_consumer][_dsp].amount -= _amountToSell;
        token.safeTransferFrom(address(this),_consumer, _amountToSell);
        emit SoldGas(_consumer, _dsp, _amountToSell);
    }
    function applyUsage(
        address _consumer,
        address _dsp
    ) internal {
        uint256 prevAmount = dspData[_consumer][_dsp].amount;
        uint256 newAmount = getAvailableGas(_consumer,_dsp);
        dspData[_consumer][_dsp].amount = newAmount;
        uint256 used = prevAmount - newAmount;
        dspData[_consumer][_dsp].claimable += used;
        dspData[_consumer][_dsp].lastUsage = block.timestamp;
        emit UsedGas(_consumer, _dsp, used);
    }
    function claimFor(
        address _consumer,
        address _dsp
    ) public {                
        applyUsage(_consumer, _dsp);        
        uint256 claimableAmount = dspData[_consumer][_dsp].claimable;
        if(claimableAmount == 0){
            // throw
        }
        token.safeTransferFrom(address(this), _dsp, claimableAmount);
        emit ClaimedGas(_consumer, _dsp, claimableAmount);
    }

    function getUsedGas(
        address _consumer,
        address _dsp
    ) public view returns (uint256) {        
        uint256 lastUsage = dspData[_consumer][_dsp].lastUsage;
        uint256 passedTime = (block.timestamp - lastUsage);
        uint256 processesCount = dspData[_consumer][_dsp].processesCount;        
        return processesCount * (passedTime * gasPerTimeUnit);
    }
    function getAvailableGas(
        address _consumer,
        address _dsp
    ) public view returns (uint256){
        uint256 used = this.getUsedGas(_consumer,_dsp);
        uint256 amount = dspData[_consumer][_dsp].amount;
        if(used > amount)
            return 0;
        return amount - used;
    }


        


    // registry of validated wasms
    // ipfs registry for kernel and extensions
    function setOS(string memory manifest) public onlyOwner { // 
        OSManifest = manifest;
    }


    function spawn(address _dsp, string memory processJSON) public {
        address _consumer = msg.sender;
        if(!registeredDSPs[_dsp].active){
            // block new spawns
        }
        applyUsage(_consumer, _dsp);
        dspData[_consumer][_dsp].processesCount++;
        uint256 pid = ++dspData[_consumer][_dsp].lastProcessId;
        dspData[_consumer][_dsp].processes[pid] = true;
        emit Spawn(_consumer, _dsp, processJSON, pid);
    }
    
    function kill(address _dsp, uint256 processId) public {
        address _consumer = msg.sender;
        applyUsage(_consumer, _dsp);        
        if(!dspData[_consumer][_dsp].processes[processId]){
            // not up
            // throw 
        }
        delete dspData[_consumer][_dsp].processes[processId];
        dspData[_consumer][_dsp].processesCount--;

        emit Kill(_consumer, _dsp, processId);
    }
    
    function regDSP() public {
        address _dsp = msg.sender;
        registeredDSPs[_dsp].active = true;
        emit DSPStatusChanged(_dsp, true);
    }
    
    function deprecateDSP() public {
        address _dsp = msg.sender;
        registeredDSPs[_dsp].active = false;
        emit DSPStatusChanged(_dsp, false);
    }
    
}
