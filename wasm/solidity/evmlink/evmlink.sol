//SPDX-License-Identifier: UNLICENSED
pragma solidity >0.6.0;

abstract contract evmlink {
    struct OutboundMessage {
        bytes message;
        uint256 block_num;
    }

    uint256 public available_message_id;
    uint256 public next_incoming_message_id;

    address[] owners;
    uint256 required_sigs;

    // mapping of bridge owners
    mapping(address => bool) public isOwner;

    //mapping (uint256 => uint64) public outbound;
    mapping(uint256 => OutboundMessage) public outbound;

    mapping(bytes32 => mapping(address => bool)) public hasConfirmed;
    mapping(bytes32 => bool) public executedMsg;
    mapping(bytes32 => uint256) public numOfConfirmed;

    constructor(address[] memory _owners, uint256 _required) {
        for (uint256 i = 0; i < _owners.length; i++) {
            require(!isOwner[_owners[i]] && _owners[i] != address(0));
            isOwner[_owners[i]] = true;
        }
        require(_required <= _owners.length);
        owners = _owners;
        required_sigs = _required;
    }

    /**
     * @dev confirms consensus of messages before execution
     *
     * @param theHash message
     */
    function confirmConsensus(bytes32 theHash) internal returns (bool) {
        require(isOwner[msg.sender], "sender not authorized");
        require(
            !(hasConfirmed[theHash][msg.sender]),
            "sender already confirmed"
        );
        hasConfirmed[theHash][msg.sender] = true;
        numOfConfirmed[theHash] += 1;
        if (numOfConfirmed[theHash] >= required_sigs && !executedMsg[theHash]) {
            executedMsg[theHash] = true;
            return true;
        }
        return false;
    }

    /**
     * @dev modifies consensus parameters
     *
     * @param id unique id
     * @param _owners new owners
     * @param required new required threshold
     */
    function modifyConsensus(
        uint256 id,
        address[] memory _owners,
        uint256 required
    ) public {
        bytes32 dataHash = keccak256(abi.encodePacked(id, _owners, required));
        if (!confirmConsensus(dataHash)) {
            return;
        }
        for (uint256 i = 0; i < owners.length; i++) {
            isOwner[owners[i]] = false;
        }
        for (uint256 i = 0; i < _owners.length; i++) {
            require(!isOwner[_owners[i]] && _owners[i] != address(0));
            isOwner[_owners[i]] = true;
        }
        require(required <= _owners.length);
        owners = _owners;
        required_sigs = required;
    }

    /**
     * @dev view function which returns Messages only if 12 blocks have passed
     *
     * @param messageId the message id to retrieve
     */
    function getOutboundMessage(uint256 messageId)
        public
        view
        returns (bytes memory message, uint256 id)
    {
        OutboundMessage memory requestedMessage = outbound[messageId];
        if (
            requestedMessage.block_num > 0 &&
            block.number > (requestedMessage.block_num + uint256(12))
        ) {
            return (requestedMessage.message, messageId);
        }
        bytes memory emptyMessage;
        return (emptyMessage, 0);
    }

    /**
     * @dev handling the pushing of messages from other chains
     *
     * @param _message the message to push
     */
    function pushInboundMessage(uint256 id, bytes memory _message) external {
        bytes32 dataHash = keccak256(abi.encodePacked(id, _message));
        if (!confirmConsensus(dataHash)) {
            return;
        }
        onMessage(id, _message);
        // TODO: check this logic - require consecutive?
        require(id == next_incoming_message_id, "non consecutive pushed ids");
        next_incoming_message_id++;
    }

    /**
     * @dev handling the pushing of local messages
     *
     * @param _message the message to push
     */
    function pushMessage(bytes memory _message) internal {
        OutboundMessage memory message =
            OutboundMessage(_message, block.number);
        outbound[available_message_id] = message;
        available_message_id++;
    }

    /**
     * @dev on message hook, unique implementation per consumer
     *
     * @param id uint256
     * @param _message message
     */
    function onMessage(uint256 id, bytes memory _message) internal virtual;
}
