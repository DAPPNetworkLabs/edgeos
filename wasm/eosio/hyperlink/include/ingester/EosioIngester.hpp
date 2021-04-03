#pragma once
#include "Ingester.hpp"
#include <digesters/sha256.hpp>
#include <verifiers/eosio.hpp>

namespace hyperlink {

class EosioIngester : public Ingester {
   public:
      EosioIngester(const eosio::name& receiver) : Ingester(receiver) {}

      void Ingest(   const std::vector<char>& signer,
                     const std::vector<char>& signature,
                     const std::vector<char>& eventHash ) 
      {         
         Verifier::Verify(signer,signature,eventHash);
         Ingester::Ingest(signer,signature,eventHash);
      }

      void Digest(   const std::vector<char>& eventData ) 
      {
         Ingester::Digest(Digester::Digest(eventData), eventData);
      }
};
}