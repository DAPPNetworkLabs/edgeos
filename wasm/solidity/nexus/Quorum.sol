pragma solidity >=0.7.0 <0.9.0;
import "../../../node_modules/@openzeppelin/contracts/access/Ownable.sol";

contract Quorum is Ownable {
    using SafeERC20 for IERC20;    
    uint256 public gasPerTimeUnit = 100;
    IERC20 public token;

    event QuorumChanged(
        address indexed consumer,
        address indexed to,
        uint256 amount
    );

    // event Spawn(
    //     address indexed from,
    //     address indexed to,
    //     string processJSON
    // );

    // event Kill(
    //     address indexed from,
    //     address indexed to,
    //     uint256 processid
    // );    
    
    // struct Process {
    //     string processJSON
    // }

    struct Consumer {
        address owner;
        address[] dsps;
        // Process[] runningProcesses;

    }

    mapping(address => Consumer) public consumers;

    constructor () {
    }

    function regConsumer(address[] dsps, address owner) public {
        Consumer consumer;
        consumer.dsps = dsps;
        consumer.owner = owner;
        consumers[msg.sender] = consumer;
        // emit event
    }

    // function spawn(address _consumer, string memory processJSON) public {
    //     // check owner permissions                
    // }
    
    // function kill(address _consumer, uint256 processId) public {
    //     // check permissions
    // }

    function addDSP(address dsp) public {
        // start existing processes on new DSP
        consumers[msg.sender].dsps = dsps;
        // emit event to existing DSPs
    }

    function setConsumerPermissions(address owner) public {
        consumers[msg.sender].owner = owner;
        // emit event
    }
        
}
