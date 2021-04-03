//SPDX-License-Identifier: UNLICENSED
pragma solidity ^0.8.0;
import "../../node_modules/@openzeppelin/contracts/math/SafeMath.sol";
import "../evmlink/evmlink.sol";
import "../IERC20/IERC20.sol";

contract evmtokenpeg is evmlink {
    using SafeMath for uint256;

    event Refund(uint256 id, address recipient, uint256 amount, bytes reason);
    event Failure(bytes reason);
    event Receipt(address recipient, uint256 amount, bytes reason);
    event OnMessage(
        bool success,
        address recipient,
        uint256 amount,
        address sender
    );
    event OnReceipt(
        bool success,
        address recipient,
        uint256 amount,
        address sender
    );

    IERC20 public tokenContract; // address of token contract
    bool public canIssue; // true if token not native to this chain

    uint256 public maxLockLimit; // the maximum amount of tokens that can be locked in one transaction
    uint256 public maxReleaseLimit; // the maximum amount of tokens that can be released in one transaction
    uint256 public minLimit; // the minimum amount of tokens that can be transferred in one transaction
    uint256 public prevLockLimit; // the lock limit *after* the last transaction
    uint256 public prevReleaseLimit; // the release limit *after* the last transaction
    uint256 public limitIncPerBlock; // how much the limit increases per block
    uint256 public prevLockBlockNumber; // the block number of the last lock transaction
    uint256 public prevReleaseBlockNumber; // the block number of the last release transaction

    constructor(
        address[] memory _owners,
        uint8 _required,
        address _tokenContract,
        bool _canIssue,
        uint256 _maxLockLimit,
        uint256 _maxReleaseLimit,
        uint256 _minLimit,
        uint256 _limitIncPerBlock
    ) evmlink(_owners, _required) {
        require(
            _minLimit <= _maxLockLimit && _minLimit <= _maxReleaseLimit,
            "ERR_INVALID_MIN_LIMIT"
        );
        tokenContract = IERC20(_tokenContract);
        canIssue = _canIssue;

        maxLockLimit = _maxLockLimit;
        maxReleaseLimit = _maxReleaseLimit;
        minLimit = _minLimit;
        limitIncPerBlock = _limitIncPerBlock;

        // previous limit is _maxLimit, and previous block number is current block number
        prevLockLimit = _maxLockLimit;
        prevReleaseLimit = _maxReleaseLimit;
        prevLockBlockNumber = block.number;
        prevReleaseBlockNumber = block.number;
    }

    /**
     * @dev method for calculating current lock limit
     *
     * @return the current maximum limit of tokens that can be locked
     */
    function getCurrentLockLimit() public view returns (uint256) {
        // prevLockLimit + ((currBlockNumber - prevLockBlockNumber) * limitIncPerBlock)
        uint256 currentLockLimit =
            prevLockLimit.add(
                ((block.number).sub(prevLockBlockNumber)).mul(limitIncPerBlock)
            );
        if (currentLockLimit > maxLockLimit) return maxLockLimit;
        return currentLockLimit;
    }

    /**
     * @dev method for calculating current release limit
     *
     * @return the current maximum limit of tokens that can be released
     */
    function getCurrentReleaseLimit() public view returns (uint256) {
        // prevReleaseLimit + ((currBlockNumber - prevReleaseBlockNumber) * limitIncPerBlock)
        uint256 currentReleaseLimit =
            prevReleaseLimit.add(
                ((block.number).sub(prevReleaseBlockNumber)).mul(
                    limitIncPerBlock
                )
            );
        if (currentReleaseLimit > maxReleaseLimit) return maxReleaseLimit;
        return currentReleaseLimit;
    }

    /**
     * @dev allows msg.sender to send tokens to another chain
     *
     * @param amount message
     * @param recipient message
     */
    function sendToken(uint256 amount, address recipient) public {
        // get the current lock limit
        uint256 currentLockLimit = getCurrentLockLimit();

        // verify lock limit
        require(
            amount >= minLimit && amount <= currentLockLimit,
            "ERR_AMOUNT_TOO_HIGH"
        );
        if (canIssue) {
            tokenContract.burnFrom(msg.sender, amount);
            bytes memory message =
                abi.encode(bytes1(0x00), true, recipient, amount, msg.sender);
            pushMessage(message);
        } else {
            tokenContract.transferFrom(msg.sender, address(this), amount);
            bytes memory message =
                abi.encode(bytes1(0x00), true, recipient, amount, msg.sender);
            pushMessage(message);
        }
        // set the previous lock limit and block number
        prevLockLimit = currentLockLimit.sub(amount);
        prevLockBlockNumber = block.number;
    }

    /**
     * @dev allows msg.sender to send tokens to another chain
     *
     * @param amount message
     * @param recipient message
     */
    function mintTokens(
        uint256 amount,
        address recipient,
        bytes memory message
    ) internal {
        // get the current release limit
        uint256 currentReleaseLimit = getCurrentReleaseLimit();

        require(
            amount >= minLimit && amount <= currentReleaseLimit,
            "ERR_AMOUNT_TOO_HIGH"
        );

        // update the previous release limit and block number
        prevReleaseLimit = currentReleaseLimit.sub(amount);
        prevReleaseBlockNumber = block.number;
        bytes memory receipt = message;
        // mark as receipt by first byte
        receipt[0] = 0x01;
        if (canIssue) {
            try tokenContract.mint(recipient, amount) {
                receipt[1] = 0x01;
            } catch (bytes memory failureMessage) {
                receipt[1] = 0x00;
                emit Failure(failureMessage);
            }
        } else {
            try tokenContract.transfer(recipient, amount) {
                receipt[1] = 0x01;
            } catch (bytes memory failureMessage) {
                receipt[1] = 0x00;
                emit Failure(failureMessage);
            }
        }
        pushMessage(receipt);
    }

    /**
     * @dev on message hook, unique implementation per consumer
     *
     * @param _message message
     */
    function onMessage(uint256 id, bytes memory _message) internal override {
        //byte 0, status
        //byte 1-8, eos account
        //byte 9-17, amount
        //byte 17-27 address
        (
            bytes1 messageType,
            bool success,
            address recipient,
            uint256 amount,
            address sender
        ) = abi.decode(_message, (bytes1, bool, address, uint256, address));
        if (messageType == 0x00) {
            mintTokens(amount, recipient, _message);
        } else if (messageType == 0x01) {
            if (!success) {
                tokenContract.mint(sender, amount);
                emit Refund(id, sender, amount, _message);
            }
            emit OnReceipt(success, recipient, amount, sender);
        } else {
            require(false, "unrecognized message type");
        }
        emit OnMessage(success, recipient, amount, sender);
    }
}
