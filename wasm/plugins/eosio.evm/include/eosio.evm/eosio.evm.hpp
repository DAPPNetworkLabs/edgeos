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

// External
#include <intx/base.hpp>
#include <ecc/uECC.c>
#include <keccak256/k.c>
#include <rlp/rlp.hpp>

// EOS
// #include <eosio/eosio.hpp>
// #include <eosio/asset.hpp>
// #include <eosio/crypto.hpp>
// #include <eosio/transaction.hpp>

// Local
#include "constants.hpp"
#include "util.hpp"
#include "exception.hpp"
#include "datastream.hpp"
#include "block.hpp"
#include "transaction.hpp"
#include "context.hpp"
#include "processor.hpp"
#include "program.hpp"
#include "tables.hpp"

namespace eosio_evm {
  class evm  {
  public:

    // evm( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
    //   : contract(receiver, code, ds),
    //     _accounts(receiver, receiver.value) {}

    void raw      ( const eosio::name& ram_payer,
                      const std::vector<int8_t>& tx,
                      const std::optional<eosio::checksum160>& sender);
    void create   ( const eosio::name& account,
                      const std::string& data);
    // void withdraw ( const eosio::name& to,
    //                   const eosio::asset& quantity);

    // void transfer( const eosio::name& from,
    //                const eosio::name& to,
    //                const eosio::asset& quantity,
    //                const std::string& memo );

    // Extra to match ethereum functionality (calls do not modify state and will always assert)
    void call(
      const eosio::name& ram_payer,
      const std::vector<int8_t>& tx,
      const std::optional<eosio::checksum160>& sender
    );

    // Action wrappers
    // using withdraw_action = eosio::action_wrapper<"withdraw"_n, &evm::withdraw>;
    // using transfer_action = eosio::action_wrapper<"transfer"_n, &evm::transfer>;

    // Define account table
    // account_table _accounts;

    #if (TESTING == true)
    void teststatetx(const std::vector<int8_t>& tx, const Env& env);
    void devnewstore(const eosio::checksum160& address, const std::string& key, const std::string value);
    void devnewacct(const eosio::checksum160& address, const std::string balance, const std::vector<uint8_t> code, const uint64_t nonce, const eosio::name& account);
    void printstate(const eosio::checksum160& address);
    void printaccount(const eosio::checksum160& address);
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
    // void sub_balance (const eosio::name& user, const eosio::asset& quantity);
    // void add_balance (const eosio::name& user, const eosio::asset& quantity);
  };
}