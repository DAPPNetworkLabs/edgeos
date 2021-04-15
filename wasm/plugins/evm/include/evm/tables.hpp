// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace wasm_evm {
  struct  Account {
    Address address;
    uint64_t nonce;
    std::vector<uint8_t> code;
    uint256_t balance;

    Account () = default;
    Account (uint256_t _address): address(_address) {}
    uint256_t get_address() const { return address; };
    uint256_t get_balance() const { return balance; };
    uint64_t get_nonce() const { return nonce; };
    std::vector<uint8_t> get_code() const { return code; };
    bool is_empty() const { return nonce == 0 && balance == 0 && code.size() == 0; };


    #if (TESTING == true)
    void print() const {
      wasm_evm::print("\n---Acc Info Start-----");
      // wasm_evm::print("\nAddress ", address);
      // wasm_evm::print("\nIndex " + std::to_string(index));
      // wasm_evm::print("\nEOS Account " + account.to_string());
      wasm_evm::print("\nBalance " + intx::to_string(balance));
      wasm_evm::print("\nCode " +  bin2hex(code));
      wasm_evm::print("\nNonce " + std::to_string(nonce));
      wasm_evm::print("\n---Acc Info End---\n");
    }
    #endif /* TESTING */

    // EOSLIB_SERIALIZE(Account, (index)(address)(account)(nonce)(code)(balance));
  };
inline int writeFile(std::string path, std::string value){
    std::ofstream out(path);
    out << value;
    out.close();
    return 0;
}

inline std::string readFile(std::string path){
  std::ifstream t(path);
  std::string str;

t.seekg(0, std::ios::end);   
str.reserve(t.tellg());
t.seekg(0, std::ios::beg);

str.assign((std::istreambuf_iterator<char>(t)),
            std::istreambuf_iterator<char>());

  return str;
}

  inline Account* get_account_by_address(const Address& address){
    
  }
  inline void set_account(const Address& address, const Account& a){
    
  }
  inline void erase_account(const Address& address){
    
  }
  inline uint256_t* get_account_state(const Address& address, const uint256_t& key){
    
  }
  inline void set_account_state(const Address& address, const uint256_t& key, const uint256_t& value){
    
  }
  inline void erase_account_state(const Address& address, const uint256_t& key){
    
  }
  inline void erase_account_state_all(const Address& address){
    
  }

inline void to_json(nlohmann::json& j, const Account& a)
  {
    j["address"] = intx::to_string(a.address);
    j["balance"] = intx::to_string(a.balance);
    j["nonce"] = a.nonce;
    j["code"] = bin2hex(a.code);
  }

  inline void from_json(const nlohmann::json& j, Account& a)
  {
    if (j.find("address") != j.end())
    {
      a.address = intx::from_string<uint256_t>(j["address"].get<std::string>());
    }

    if (j.find("balance") != j.end())
    {
      a.balance = intx::from_string<uint256_t>(j["balance"].get<std::string>());
    }

    if (j.find("nonce") != j.end())
    {
      a.nonce = j["nonce"].get<uint64_t>();
    }

    if (j.find("code") != j.end())
    {
      a.code = hex2bin(j["code"].get<std::string>());
    }
  }
  // struct AccountState {
  //   uint64_t index;
  //   wasm_evm::checksum256 key;
  //   bigint::checksum256 value;

  //   uint64_t primary_key() const { return index; };
  //   wasm_evm::checksum256 by_key() const { return key; };

  //   // EOSLIB_SERIALIZE(AccountState, (index)(key)(value));
  // };

  // typedef wasm_evm::multi_index<"account"_n, Account,
  //   wasm_evm::indexed_by<wasm_evm::name("byaddress"), wasm_evm::const_mem_fun<Account, wasm_evm::checksum256, &Account::by_address>>,
  //   wasm_evm::indexed_by<wasm_evm::name("byaccount"), wasm_evm::const_mem_fun<Account, uint64_t, &Account::get_account_value>>
  // > account_table;
  // typedef wasm_evm::multi_index<"accountstate"_n, AccountState,
  //   wasm_evm::indexed_by<wasm_evm::name("bykey"), wasm_evm::const_mem_fun<AccountState, wasm_evm::checksum256, &AccountState::by_key>>
  // > account_state_table;
}