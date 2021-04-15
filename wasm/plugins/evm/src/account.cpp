// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#include <evm/evm.hpp>

namespace wasm_evm {
  // Internal transfers only, returns true if error
  bool Processor::transfer_internal(const Address& from, const Address& to, const uint256_t& amount) {
    if (amount == 0) {
      return false;
    }

    // Index
    // auto accounts_byaddress = contract->_accounts.get_index<wasm_evm::name("byaddress")>();
    auto from_account = get_account_by_address(from);

    auto to_account = get_account_by_address(to);
    // // Addresses and accounts
    // auto from_256     = toChecksum256(from);
    // auto to_256       = toChecksum256(to);
    // auto from_account = accounts_byaddress.find(from_256);
    // auto to_account   = accounts_byaddress.find(to_256);

    // Amount verification
    if (amount < 0) {
      transaction.errors.push_back("transfer amount must not be negative");
      return true;
    }

    // From balance verification
    if (from_account == NULL) {
      transaction.errors.push_back("account does not have a balance");
      return true;
    }
    if (amount > from_account->get_balance()) {
      transaction.errors.push_back("account balance too low");
      return true;
    }

    if(to_account == NULL){
      auto [new_account, error] = create_account(to);
      if (error) {
        transaction.errors.push_back("Error creating new address to transfer value");
        return true;
      } else {
        to_account = &((Account&)new_account);
      }
    }
    // Reflect state
    from_account->balance -= amount;
    to_account->balance += amount;
    set_account(from,*from_account);
    set_account(to,*to_account);

    // Modification record
    transaction.add_modification({ SMT::TRANSFER, 0, from, to, amount, 0 });

    // Return false for no error
    return false;
  }

  // Only used by CREATE instruction to increment nonce of contract
  void Processor::increment_nonce(const Address& address) {
    auto account = get_account_by_address(address);

    // auto accounts_byaddress = contract->_accounts.get_index<wasm_evm::name("byaddress")>();
    // auto existing_address = accounts_byaddress.find(toChecksum256(address));
    if (account != NULL) {
      account->nonce += 1;
      set_account(address,*account);
      transaction.add_modification({ SMT::INCREMENT_NONCE, 0, address, account->get_nonce() - 1, 0, account->get_nonce()});
    }
  }

  // Only used for reverting
  void Processor::decrement_nonce(const Address& address) {
    auto account = get_account_by_address(address);
    if (account != NULL) {
      account->nonce -= 1;
      set_account(address,*account);
    }
  }

  // Used to push result of top level create and instruction CREATE
  void Processor::set_code(const Address& address, const std::vector<uint8_t>& code) {
    if (code.empty()) return;
    auto account = get_account_by_address(address);
    if (account != NULL) {
      account->code = code;
      set_account(address,*account);
    }
  }

  // Only used while reverting
  void Processor::remove_code(const Address& address) {
    auto account = get_account_by_address(address);
    if (account != NULL) {
      account->code = {};
      set_account(address,*account);
    }
  }

  // Returns an empty account if not found
  const Account& Processor::get_account(const Address& address)
  {
    auto account = get_account_by_address(address);

    if (account == NULL)
    {
      static const auto empty_account = Account(address);
      return empty_account;
    }
    else
    {
      return *account;
    }
  }

  // Returns [account, error], error is true if account already exists
  Processor::AccountResult Processor::create_account(
    const Address& address,
    const bool& is_contract // default false
  )
  {
    auto account = get_account_by_address(address);

    // If account already exists
    if (account != NULL) {
      // If it has nonce or non-empty code -> ERROR
      if (account->get_nonce() > 0 || !account->get_code().empty())
      {
        static const auto empty_account = Account(address);
        return { empty_account, true };
      }
      // If it doesn't have nonce or non-empty code -> increment nonce if contract and return
      // We also kill all storage, as it may be remnant of CREATE2
      else
      {
        if (is_contract) {
          kill_storage(address);

          account->nonce += 1;
          set_account(address,*account);
          transaction.add_modification({ SMT::INCREMENT_NONCE, 0, address, account->get_nonce() - 1, 0, account->get_nonce()});
        }
        
        return { *account, false };
      }
    }

    // Initial Nonce is 1 for contracts and 0 for accounts (no code)
    uint64_t nonce = is_contract ? 1 : 0;

    // Create address if it does not exists    
    Account a = Account();
    a.nonce   = nonce;
    a.address = address;
    a.balance = 0;

    set_account(address,a);
    // Add modification record
    transaction.add_modification({ SMT::CREATE_ACCOUNT, 0, address, 0, 0, 0 });
    return { a, false };
  }

  // Only used when reverting
  void Processor::remove_account(const Address& address)
  {
    // Find address
    auto account = get_account_by_address(address);

    if (account != NULL) {
      erase_account(address);
    }
  }

  /**
   * Used for self-destruct
   */
  void Processor::selfdestruct(const Address& address)
  {
    auto account = get_account_by_address(address);

    if (account != NULL) {
      // Kill storage first
      kill_storage(address);

      // Make account empty
      Account a = Account();
      a.nonce   = 0;
      a.address = address;
      a.balance = 0;

      set_account(address,a);
    }
  }
  // TODO need to use table indirection instead to save for future processing.
  void Processor::kill_storage(const Address& address)
  {
    erase_account_state_all(address);
  }

  // Returns original state
  void Processor::storekv(const Address& address, const uint256_t& key, const uint256_t& value) {
    // // Get scoped state table for account state
    // account_state_table accounts_states(contract->get_self(), address_index);
    // auto accounts_states_bykey = accounts_states.get_index<wasm_evm::name("bykey")>();
    // auto checksum_key          = toChecksum256(key);
    // auto account_state         = accounts_states_bykey.find(checksum_key);
    uint256_t* account_state =    get_account_state(address, key);

    #if (PRINT_STATE == true)
    wasm_evm::print(
      "\n\nStore KV for address index ", address,
      "\nKey: ", intx::hex(key),
      "\nValue: ", intx::hex(value),
      "\nFound: ", account_state != NULL,
      "\nValue is not 0: ", value != 0, "\n"
    );
    if (account_state != NULL) {
      wasm_evm::print("\nOld Value:", intx::hex(account_state->value));
    }
    #endif
    // Key found
    if (account_state != NULL)
    {
      transaction.add_modification({ SMT::STORE_KV, address, key, *account_state, 0, value });
      
      if (value == 0)
      {        
        erase_account_state(address, key);
      }
      else
      {
        set_account_state(address, key, value);
      }
    }
    // Key not found and new value exists
    else if (value != 0)
    {
      transaction.add_modification({ SMT::STORE_KV, address, key, 0, 0, value });
      set_account_state(address, key, value);
    }
  }

  uint256_t Processor::loadkv(const Address& address, const uint256_t& key) {
    // // Get scoped state table for account
    // account_state_table accounts_states(contract->get_self(), address_index);
    // auto accounts_states_bykey = accounts_states.get_index<wasm_evm::name("bykey")>();
    // const auto checksum_key    = toChecksum256(key);
    // auto account_state         = accounts_states_bykey.find(checksum_key);
    uint256_t* account_state = get_account_state(address, key);

    #if (PRINT_STATE == true)
    wasm_evm::print("\n\nLoad KV for address index ", address,
                 "\nKey: ", intx::hex(key),
                 "\nFound: ", account_state != accounts_states_bykey.end(), "\n");
    if (account_state != accounts_states_bykey.end()) {
      wasm_evm::print("\nValue: ", intx::hex(account_state->value), "\n");
    }
    #endif

    // // Value
    auto current_value = account_state != NULL ? *account_state : 0;
    // // Place into original storage
    transaction.emplace_original(address, key, current_value);

    return current_value;
  }
} // namespace wasm_evm
