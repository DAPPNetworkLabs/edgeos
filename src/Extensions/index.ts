
import {web3extensions} from './web3'
import {ipfsextensions} from './ipfs'
import {libp2pextensions} from './libp2p'
import {networkextensions} from './network'
import {kmsextensions} from './kms'
import {leveldbextension} from './leveldb'
import {osextensions} from './os'
import {logextensions} from './log'
import {eosioextensions} from './eosio'

export const extensions={
    ...osextensions,
    ...logextensions,
    ...web3extensions,
    ...ipfsextensions,
    ...libp2pextensions,
    ...networkextensions,
    ...kmsextensions,
    ...leveldbextension,
    ...eosioextensions
}
