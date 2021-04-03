pragma solidity >=0.7.0 <0.9.0;

contract Nexus {
    event Command(
        address indexed from,
        address indexed to,
        string commandJSON
    );

    event Bootloader(
        address indexed _from,
        bytes32 indexed _id,
        uint _value
    );

    struct DSP {
        uint weight; // weight is accumulated by delegation
        bool voted;  // if true, that person already voted
        address dsp; // person delegated to
        uint vote;   // index of the voted proposal
    }


    string public manifestJSON;

    mapping(address => DSP) public dsps;

    // Process[] public processes;

    constructor(string memory json) {
        manifestJSON = json;
       
    }
    
    // pay dsp to charge quota
    // usequota
    // spawn process - add to running processes and shoot event ()
    // set quorum
    // holds snapshots


    // registry of validated wasms
    // ipfs registry for kernel and extensions

    /**
     * @dev Delegate your vote to the voter 'to'.
     * @param to address to which vote is delegated
     */
    function run(address to, string memory commandJSON) public {
        // check quota
        emit Command(msg.sender, to, commandJSON);
    }
    function unlock() public {}
    function lock() public {}

  

    /** 
     * @dev Computes the winning proposal taking all previous votes into account.
     * @return winningProposal_ index of winning proposal in the proposals array
     */
    // function winningProposal() public view
    //         returns (uint winningProposal_)
    // {
        // uint winningVoteCount = 0;
        // for (uint p = 0; p < proposals.length; p++) {
        //     if (proposals[p].voteCount > winningVoteCount) {
        //         winningVoteCount = proposals[p].voteCount;
        //         winningProposal_ = p;
        //     }
        // }
    // }
}
