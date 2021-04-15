// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#include <evm/evm.hpp>

namespace wasm_evm
{
  void Processor::precompile_sha256()
  {
    // Charge gas
    auto gas_cost = GP_SHA256 + (GP_SHA256_WORD * num_words(ctx->input.size()));
    bool error = use_gas(gas_cost);
    if (error) return;

    // Execute
    
    wasm_evm::checksum256 checksum = wasm_evm::sha256(
      const_cast<char*>(reinterpret_cast<const char*>(ctx->input.data())),
      ctx->input.size()
    );
    auto result = checksum.extract_as_byte_array();

    // Return the result.
    precompile_return({ result.begin(), result.end() });
  }
} // namespace wasm_evm
