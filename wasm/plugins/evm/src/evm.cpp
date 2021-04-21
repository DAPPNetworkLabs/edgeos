#include <evm/evm.hpp>
#include "account.cpp"
#include "copy.cpp"
#include "transfer.cpp"
#include "instructions.cpp"
#include "processor.cpp"
#include "stack.cpp"
#include "testing.cpp"

#include "precompiles/blake2b.cpp"
#include "precompiles/bncurve.cpp"
#include "precompiles/ecrecover.cpp"
#include "precompiles/expmod.cpp"
#include "precompiles/identity.cpp"
#include "precompiles/index.cpp"
#include "precompiles/ripemd160.cpp"
#include "precompiles/sha256.cpp"


namespace wasm_evm
{

void evm::create (
  const wasm_evm::name& account,
  const std::string& data
) {
  // Check that account is authorized
  // require_auth(account);

  // // Encode using RLP(account.value, data)
  // auto accounts_byaccount = _accounts.get_index<wasm_evm::name("byaccount")>();
  // auto existing_account = accounts_byaccount.find(account.value);
  // wasm_evm::check(existing_account == accounts_byaccount.end(), "an EVM account is already linked to this EOS account.");

  // // Encode using RLP, Hash and get right-most 160 bits (Address)
  // const auto rlp_encoding = rlp::encode(account.value, data);
  // wasm_evm::checksum160 address_160 = toChecksum160( keccak_256(rlp_encoding) );

  // // Create user account
  // auto address_256        = pad160(address_160);
  // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
  // auto existing_address   = accounts_byaddress.find(address_256);
  // wasm_evm::check(existing_address == accounts_byaddress.end(), "an EVM account with this address already exists.");
  // // _accounts.emplace(account, [&](auto& a) {
  // //   a.index   = _accounts.available_primary_key();
  // //   a.address = address_160;
  // //   a.nonce   = 1;
  // //   a.account = account;
  // //   a.balance = 0;
  // // });

  // // Print out the address
  // wasm_evm::print(address_160);
}

void evm::raw(
  const std::vector<int8_t>& tx,
  const std::optional<wasm_evm::checksum160>& sender
) {
  // Create transaction
  auto transaction = EthereumTransaction(tx);

  // // Index by address
  // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
  // decltype(accounts_byaddress.begin()) caller;
  Account* caller;
  // // The “R” and “S” values of the transaction are 0 (EOS SPECIAL)
  if (transaction.is_zero())
  {
    // Ensure sender exists
    wasm_evm::check(sender.has_value(), "Invalid Transaction: no signature in transaction and no sender address was provided.");
    // caller = accounts_byaddress.find(pad160(*sender));
    caller = get_account_by_address(checksum160ToAddress(*sender));
    wasm_evm::check(caller != NULL, "Invalid Transaction: sender address does not exist (without signature).");

    // Set transaction sender
    transaction.sender = *sender;
  }
  // The “R” and “S” values of the transaction are NOT 0
  else
  {
    // EC RECOVERY    
    caller = get_account_by_address(checksum160ToAddress(transaction.get_sender()));

    // Ensure signer exists
    wasm_evm::check(caller != NULL, "Invalid Transaction: sender address does not exist (with signature).");
  }

  // // Check account nonce
  wasm_evm::check(caller->get_nonce() == transaction.nonce, "Invalid Transaction: incorrect nonce, received " + to_string(transaction.nonce) + " expected " + std::to_string(caller->get_nonce()));
  caller->nonce +=1;

  // // Check balance
  wasm_evm::check(caller->get_balance() >= transaction.value, "Invalid Transaction: Sender balance too low for value specified");

  // Added for tests and configurability
  #if (CHARGE_SENDER_FOR_GAS == true)
  auto gas_cost = transaction.gas_price * transaction.gas_limit;
  wasm_evm::check(gas_cost <= caller->get_balance(), "Invalid Transaction: Sender balance too low to pay for gas");
  wasm_evm::check(gas_cost >= 0, "Invalid Transaction: Gas cannot be negative");
  caller->balance -=gas_cost;
  #endif
  set_account(caller->get_address(), *caller);
  // /**
  //  * Savepoint: All operations in processor could be reverted
  //  *
  //  * CANT USE wasm_evm::check()
  //  */
  Processor(transaction, this).process_transaction(*caller);

  // // Added for tests
  #if (CHARGE_SENDER_FOR_GAS == true)
    auto gas_used = transaction.gas_refunds > (transaction.gas_used / 2)
                      ? transaction.gas_used - (transaction.gas_used / 2)
                      : transaction.gas_used - transaction.gas_refunds;
    auto gas_used_cost = gas_used * transaction.gas_price;
    auto gas_unused = transaction.gas_limit - gas_used;
    auto gas_unused_cost = gas_unused * transaction.gas_price;    
    caller->balance += gas_unused_cost;
    set_account(caller->get_address(), caller);
  #endif
}

/**
 * Will always assert, replicates Ethereum Call functionality
 */
void evm::call(
  const std::vector<int8_t>& tx,
  const std::optional<wasm_evm::checksum160>& sender
) {
  auto transaction = EthereumTransaction(tx);
    wasm_evm::checksum256 sender_256 = pad160(transaction.get_sender());

  // Find caller
  auto caller = Account();

  if (sender.has_value()) {
    caller = *get_account_by_address(checksum160ToAddress(sender.value()));
  }

  // Create processor and find result
  auto processor = Processor(transaction, this);
  auto result = processor.initialize_call(caller);

  // Print Receipt
  wasm_evm::print(bin2hex(result.output));

  // IMPORTANT, DO NOT REMOVE
  wasm_evm::check(false, "");
}

} // namepsace wasm_evm

wasm_evm::evm * evm;
int main(int argc, const char **argv){
    // restore snapshot
    const char * prm0 = argv[1];
    const char * prm1 = argv[2];
    const char * prm2 = argv[3];    
    evm = new wasm_evm::evm();
    wasm_evm::checksum160 sender;
    
    elog("evm process running: " + std::string(prm0));
    std::string ps = std::string(prm1);
    // elog("ps size: " + std::to_string(ps.size()));
    std::vector<uint8_t> trx = wasm_evm::hex2bin(ps);
    // elog("trx size: " + std::to_string(trx.size()));

    const std::vector<int8_t>& trxv = std::vector<signed char>(trx.begin(),trx.end());
    // elog("trxv size: " + std::to_string(trxv.size()));
    evm->raw(trxv, std::nullopt);
    // auto transaction = wasm_evm::EthereumTransaction(trxv);
    // auto s = wasm_evm::bin2hex(transaction.to);
    // elog("trx to: 0x" + s);    
    // evm->raw(, std::nullopt);

    return 0;
}