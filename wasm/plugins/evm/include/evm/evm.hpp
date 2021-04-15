#pragma once

// Standard.
#include <optional>
#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include "../../../processes/include//libedgeos.h"

#include "wasm_evm.h"
// External
#include <intx/base.hpp>
#include <ecc/uECC.c>
#include <keccak256/k.c>
#include <sha256/sha256.c>
#include <rlp/rlp.hpp>
#include "util.hpp"

// Local
#include "constants.hpp"
#include "exception.hpp"
#include "datastream.hpp"
#include "block.hpp"
#include "transaction.hpp"
#include "context.hpp"
#include "processor.hpp"
#include "program.hpp"
#include "tables.hpp"

namespace wasm_evm {




  class evm  {
  public:

    evm( ) {} // pass state

    void raw      ( const std::vector<int8_t>& tx,
                      const std::optional<wasm_evm::checksum160>& sender);
    void create   ( const wasm_evm::name& account,
                      const std::string& data);
    // void withdraw ( const wasm_evm::name& to,
    //                   const wasm_evm::asset& quantity);

    // void transfer( const wasm_evm::name& from,
    //                const wasm_evm::name& to,
    //                const wasm_evm::asset& quantity,
    //                const std::string& memo );

    // Extra to match ethereum functionality (calls do not modify state and will always assert)
    void call(
      const std::vector<int8_t>& tx,
      const std::optional<wasm_evm::checksum160>& sender
    );

    // Action wrappers
    // using withdraw_action = wasm_evm::action_wrapper<"withdraw"_n, &evm::withdraw>;
    // using transfer_action = wasm_evm::action_wrapper<"transfer"_n, &evm::transfer>;

    // Define account table
    // account_table _accounts;

    #if (TESTING == true)
    void teststatetx(const std::vector<int8_t>& tx, const Env& env);
    void devnewstore(const wasm_evm::checksum160& address, const std::string& key, const std::string value);
    void devnewacct(const wasm_evm::checksum160& address, const std::string balance, const std::vector<uint8_t> code, const uint64_t nonce, const wasm_evm::name& account);
    void printstate(const wasm_evm::checksum160& address);
    void printaccount(const wasm_evm::checksum160& address);
    void testtx(const std::vector<int8_t>& tx);
    void printtx(const std::vector<int8_t>& tx);
    void clearall () {
    //   require_auth(get_self());

    //   account_table db(get_self(), get_self().value);
    //   auto itr = db.end();
    //   while(db.begin() != itr){
    //     itr = db.erase(--itr);
    //   }

    //   for(int i = 0; i < 25; i++) {
    //     account_state_table db2(get_self(), i);
    //     auto itr = db2.end();
    //     while(db2.begin() != itr){
    //       itr = db2.erase(--itr);
    //     }
    //   }
    }
    #endif

  private:
    // EOS Transfer
    // void sub_balance (const wasm_evm::name& user, const wasm_evm::asset& quantity);
    // void add_balance (const wasm_evm::name& user, const wasm_evm::asset& quantity);
  };
}