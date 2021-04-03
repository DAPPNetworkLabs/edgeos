#pragma once


namespace hyperlink {
    
const std::vector<char> hashDataV(const std::vector<char> &data){ 
  auto buffer = data; 
  char* c = (char*) malloc(buffer.size()+1); 
  memcpy(c, buffer.data(), buffer.size()); 
  c[buffer.size()] = 0; 
  //capi_checksum256 *hash_val = (capi_checksum256 *) malloc(32); 
  auto hash_val = eosio::sha256(c, buffer.size()); 
  char * placeholder = (char*) malloc(32);
  memcpy(placeholder , hash_val.data(), 32 );
  std::vector<char> hash_ret = std::vector<char>(placeholder,placeholder + 32); 
  return hash_ret; 
} 
static std::vector<char> data_to_ipfsmultihash(const std::vector<char> &data) { 
    // fn - 0x12  
    // size - 0x20  
    // hash - 32 
    std::vector<char> res; 
    res.push_back(0x01);
    res.push_back(0x55);
    res.push_back(0x12);
    res.push_back(0x20);
    std::vector<char> hash = hashDataV(data); 
    auto it = hash.begin(); 
    while (it != hash.end()) 
        res.push_back(*(it++)); 
    return res; 
} 
static bool is_equal(const std::vector<char> &a, const std::vector<char> &b){  
    return memcmp((void *)&a, (const void *)&b, a.size()) == 0; 
} 
static void assert_ipfsmultihash(std::vector<char> data, std::vector<char> hash) {
    std::vector<char> calcedhash = data_to_ipfsmultihash(data);
    eosio::check(is_equal(calcedhash,hash),"hashes not equel");
}
static eosio::checksum256 ipfsmultihash_to_key256(const std::vector<char> &ipfshash) {
    // skip 4 bytes 
    std::vector<char> multiHashPart(ipfshash.begin() + 4, ipfshash.end());
    uint64_t * p64 = (uint64_t*) malloc(32);
    memcpy(p64 , multiHashPart.data(), 32 );

    return eosio::checksum256::make_from_word_sequence<uint64_t>(p64[0], p64[1], p64[2], p64[3]);
}
static eosio::checksum256 data_to_key256(const std::vector<char> &data) {
    return ipfsmultihash_to_key256(data_to_ipfsmultihash(data));
}
    
}