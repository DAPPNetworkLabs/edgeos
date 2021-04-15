#include <evm/evm.hpp>

#if (TESTING == true)
namespace wasm_evm {
  void evm::testtx(const std::vector<int8_t>& tx) {
    // require_auth(get_self());
    // auto transaction = EthereumTransaction(tx, get_self());
    // wasm_evm::print(R"({"hash":")", transaction.hash, R"(", "sender":")", transaction.get_sender(), R"("})");
  }

  void evm::printtx(const std::vector<int8_t>& tx) {
    // require_auth(get_self());
    // auto transaction = EthereumTransaction(tx, get_self());
    // transaction.get_sender();
    // transaction.printhex();
  }

  void evm::devnewacct(const wasm_evm::checksum160& address, const std::string balance, const std::vector<uint8_t> code, const uint64_t nonce, const wasm_evm::name& account) {
    // require_auth(get_self());

    // Create account
    auto address_256        = pad160(address);
    // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
    // auto existing_address   = accounts_byaddress.find(address_256);
    // wasm_evm::check(existing_address == accounts_byaddress.end(), "An account already exists with this address");

    auto ubalance = intx::from_string<uint256_t>(balance);
    wasm_evm::check(ubalance >= 0, "Balance cannot be negative");
    // _accounts.emplace(get_self(), [&](auto& a) {
    //   a.index   = _accounts.available_primary_key();
    //   a.address = address;
    //   a.account = account;
    //   a.balance = ubalance;
    //   a.nonce   = nonce;
    //   a.code    = code;
    // });
  }

  void evm::devnewstore(const wasm_evm::checksum160& address, const std::string& key, const std::string value) {
    // require_auth(get_self());

    // Get account
    // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
    // auto existing_address   = accounts_byaddress.find(pad160(address));
    // wasm_evm::check(existing_address != accounts_byaddress.end(), "address does not exist");

    // Key value
    auto checksum_key   = toChecksum256(intx::from_string<uint256_t>(key));
    auto checksum_value = intx::from_string<uint256_t>(value);

    // Store KV
    // account_state_table accounts_states(get_self(), existing_address->index);
    // auto accounts_states_bykey = accounts_states.get_index<wasm_evm::name("bykey")>();
    // auto account_state         = accounts_states_bykey.find(checksum_key);
    // accounts_states.emplace(get_self(), [&](auto& a) {
    //     a.index   = accounts_states.available_primary_key();
    //     a.key     = checksum_key;
    //     a.value   = checksum_value;
    // });
  }

  void evm::teststatetx(const std::vector<int8_t>& tx, const Env& env) {
    // require_auth(get_self());

    // Set block from env
    set_current_block(env);

    // Execute transaction
    // raw(get_self(), tx, std::nullopt);
  }

  void evm::printaccount(const wasm_evm::checksum160& address) {
    // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
    // auto existing_address   = accounts_byaddress.find(pad160(address));

    // wasm_evm::print("{");
    // if (existing_address != accounts_byaddress.end()) {
    //   auto code = bin2hex(existing_address->get_code());
    //   code = code.length() % 2 == 0 ? "0x" + code : "0x0" + code;

    //   auto nonce = intx::hex(intx::from_string<uint256_t>(std::to_string(existing_address->get_nonce())));
    //   nonce = nonce.length() % 2 == 0 ? "0x" + nonce : "0x0" + nonce;

    //   auto balance = intx::hex(existing_address->get_balance());
    //   balance = balance.length() % 2 == 0 ? "0x" + balance : "0x0" + balance;

    //   wasm_evm::print("\"code\":\"", code, "\",");
    //   wasm_evm::print("\"nonce\":\"", nonce, "\",");
    //   wasm_evm::print("\"balance\":\"", balance, "\"");
    // }
    // wasm_evm::print("}");
  }

  void evm::printstate(const wasm_evm::checksum160& address) {
    wasm_evm::print("{");

    // Get account
    // auto accounts_byaddress = _accounts.get_index<wasm_evm::name("byaddress")>();
    // auto existing_address   = accounts_byaddress.find(pad160(address));

    // if (existing_address != accounts_byaddress.end()) {
    //   // Get scoped state table for account
    //   account_state_table accounts_states(get_self(), existing_address->index);
    //   auto itr = accounts_states.begin();
    //   while(itr != accounts_states.end()){
    //     std::string key = intx::hex(checksum256ToValue(itr->key));
    //     std::string value = intx::hex(itr->value);

    //     if (key.length() % 2 == 0) {
    //       key = "0x" + key;
    //     } else {
    //       key = "0x0" + key;
    //     }

    //     if (value.length() % 2 == 0) {
    //       value = "0x" + value;
    //     } else {
    //       value = "0x0" + value;
    //     }

    //     wasm_evm::print("\"" + key + "\":\"" + value + "\"");

    //     itr++;

    //     if (itr != accounts_states.end()) {
    //       wasm_evm::print(",");
    //     }
    //   }
    // }

    wasm_evm::print("}");
  }
}
#endif