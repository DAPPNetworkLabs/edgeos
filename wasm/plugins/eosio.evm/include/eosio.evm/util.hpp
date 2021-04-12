// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) 2020 Syed Jafri. All rights reserved.
// Licensed under the MIT License..

#pragma once

#include "constants.hpp"
namespace eosio {
  typedef std::string name;
  // typedef std::string checksum160;
  // typedef std::string checksum256;


  
   /// @cond IMPLEMENTATIONS
  static inline void check(bool condition, std::string str){

  }
  static inline void print(std::string str){

  }
   template<size_t Size>
   class fixed_bytes;

   template<size_t Size>
   bool operator ==(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator !=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator >(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator <(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator >=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator <=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   /// @endcond

    /**
     *  @defgroup fixed_bytes Fixed Size Byte Array
     *  @ingroup core
     *  @ingroup types
     *  @brief Fixed size array of bytes sorted lexicographically
     */

   /**
    *  Fixed size byte array sorted lexicographically
    *
    *  @ingroup fixed_bytes
    *  @tparam Size - Size of the fixed_bytes object
    */
   template<size_t Size>
   class fixed_bytes {
      private:

         template<bool...> struct bool_pack;
         template<bool... bs>
         using all_true = std::is_same< bool_pack<bs..., true>, bool_pack<true, bs...> >;

         template<typename Word, size_t NumWords>
         static void set_from_word_sequence(const Word* arr_begin, const Word* arr_end, fixed_bytes<Size>& key)
         {
            auto itr = key._data.begin();
            word_t temp_word = 0;
            const size_t sub_word_shift = 8 * sizeof(Word);
            const size_t num_sub_words = sizeof(word_t) / sizeof(Word);
            auto sub_words_left = num_sub_words;
            for( auto w_itr = arr_begin; w_itr != arr_end; ++w_itr ) {
               if( sub_words_left > 1 ) {
                   temp_word |= static_cast<word_t>(*w_itr);
                   temp_word <<= sub_word_shift;
                   --sub_words_left;
                   continue;
               }

               eosio::check( sub_words_left == 1, "unexpected error in fixed_bytes constructor" );
               temp_word |= static_cast<word_t>(*w_itr);
               sub_words_left = num_sub_words;

               *itr = temp_word;
               temp_word = 0;
               ++itr;
            }
            if( sub_words_left != num_sub_words ) {
               if( sub_words_left > 1 )
                  temp_word <<= 8 * (sub_words_left-1);
               *itr = temp_word;
            }
         }

      public:
typedef unsigned __int128 uint128_t;
typedef __int128 int128_t;

         typedef uint128_t word_t;

         /**
          * Get number of words contained in this fixed_bytes object. A word is defined to be 16 bytes in size
          */

         static constexpr size_t num_words() { return (Size + sizeof(word_t) - 1) / sizeof(word_t); }

         /**
          * Get number of padded bytes contained in this fixed_bytes object. Padded bytes are the remaining bytes
          * inside the fixed_bytes object after all the words are allocated
          */
         static constexpr size_t padded_bytes() { return num_words() * sizeof(word_t) - Size; }

         /**
         * Default constructor to fixed_bytes object which initializes all bytes to zero
         */
         constexpr fixed_bytes() : _data() {}

         /**
         * Constructor to fixed_bytes object from std::array of num_words() word_t types
         *
         * @param arr    data
         */
         fixed_bytes(const std::array<word_t, num_words()>& arr)
         {
           std::copy(arr.begin(), arr.end(), _data.begin());
         }

         /**
         * Constructor to fixed_bytes object from std::array of Word types smaller in size than word_t
         *
         * @param arr - Source data
         */
         template<typename Word, size_t NumWords,
                  typename Enable = typename std::enable_if<std::is_integral<Word>::value &&
                                                             std::is_unsigned<Word>::value &&
                                                             !std::is_same<Word, bool>::value &&
                                                             std::less<size_t>{}(sizeof(Word), sizeof(word_t))>::type >
         fixed_bytes(const std::array<Word, NumWords>& arr)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(Word)) * sizeof(Word),
                           "size of the backing word size is not divisible by the size of the array element" );
            static_assert( sizeof(Word) * NumWords <= Size, "too many words supplied to fixed_bytes constructor" );

            set_from_word_sequence<Word, NumWords>(arr.data(), arr.data() + arr.size(), *this);
         }

         /**
         * Constructor to fixed_bytes object from fixed-sized C array of Word types smaller in size than word_t
         *
         * @param arr - Source data
         */
         template<typename Word, size_t NumWords,
                  typename Enable = typename std::enable_if<std::is_integral<Word>::value &&
                                                             std::is_unsigned<Word>::value &&
                                                             !std::is_same<Word, bool>::value &&
                                                             std::less<size_t>{}(sizeof(Word), sizeof(word_t))>::type >
         fixed_bytes(const Word(&arr)[NumWords])
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(Word)) * sizeof(Word),
                           "size of the backing word size is not divisible by the size of the array element" );
            static_assert( sizeof(Word) * NumWords <= Size, "too many words supplied to fixed_bytes constructor" );

            set_from_word_sequence<Word, NumWords>(arr, arr + NumWords, *this);
         }

         /**
         *  Create a new fixed_bytes object from a sequence of words
         *
         *  @tparam FirstWord - The type of the first word in the sequence
         *  @tparam Rest - The type of the remaining words in the sequence
         *  @param first_word - The first word in the sequence
         *  @param rest - The remaining words in the sequence
         */
         template<typename FirstWord, typename... Rest>
         static
         fixed_bytes<Size>
         make_from_word_sequence(typename std::enable_if<std::is_integral<FirstWord>::value &&
                                                          std::is_unsigned<FirstWord>::value &&
                                                          !std::is_same<FirstWord, bool>::value &&
                                                          sizeof(FirstWord) <= sizeof(word_t) &&
                                                          all_true<(std::is_same<FirstWord, Rest>::value)...>::value,
                                                         FirstWord>::type first_word,
                                 Rest... rest)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(FirstWord)) * sizeof(FirstWord),
                           "size of the backing word size is not divisible by the size of the words supplied as arguments" );
            static_assert( sizeof(FirstWord) * (1 + sizeof...(Rest)) <= Size, "too many words supplied to make_from_word_sequence" );

            fixed_bytes<Size> key;
            std::array<FirstWord, 1+sizeof...(Rest)> arr{{ first_word, rest... }};
            set_from_word_sequence<FirstWord, 1+sizeof...(Rest)>(arr.data(), arr.data() + arr.size(), key);
            return key;
         }

         /**
          * Get the contained std::array
          */
         const auto& get_array()const { return _data; }

         /**
          * Get the underlying data of the contained std::array
          */
         auto data() { return _data.data(); }

         /// @cond INTERNAL

         /**
          * Get the underlying data of the contained std::array
          */
         auto data()const { return _data.data(); }

         /// @endcond

         /**
          * Get the size of the contained std::array
          */
         auto size()const { return _data.size(); }


         /**
          * Extract the contained data as an array of bytes
          *
          * @return - the extracted data as array of bytes
          */
         std::array<uint8_t, Size> extract_as_byte_array()const {
            std::array<uint8_t, Size> arr;

            const size_t num_sub_words = sizeof(word_t);

            auto arr_itr  = arr.begin();
            auto data_itr = _data.begin();

            for( size_t counter = _data.size(); counter > 0; --counter, ++data_itr ) {
               size_t sub_words_left = num_sub_words;

               auto temp_word = *data_itr;
               if( counter == 1 ) { // If last word in _data array...
                  sub_words_left -= padded_bytes();
                  temp_word >>= 8*padded_bytes();
               }
               for( ; sub_words_left > 0; --sub_words_left ) {
                  *(arr_itr + sub_words_left - 1) = static_cast<uint8_t>(temp_word & 0xFF);
                  temp_word >>= 8;
               }
               arr_itr += num_sub_words;
            }

            return arr;
         }

         /**
          * Prints fixed_bytes as a hexidecimal string
          *
          * @param val to be printed
          */
         inline void print()const {
            auto arr = extract_as_byte_array();
            printhex(static_cast<const void*>(arr.data()), arr.size());
         }

         /// @cond OPERATORS

         friend bool operator == <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator != <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator > <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator < <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator >= <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator <= <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         /// @endcond

      private:

         std::array<word_t, num_words()> _data;
    };

  /// @cond IMPLEMENTATIONS
   using checksum160 = fixed_bytes<20>;
   using checksum256 = fixed_bytes<32>;
   using checksum512 = fixed_bytes<64>;
   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 == c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator ==(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data == c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 != c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator !=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data != c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 > c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator >(const fixed_bytes<Size>& c1, const fixed_bytes<Size>& c2) {
      return c1._data > c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 < c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator <(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data < c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 >= c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator >=(const fixed_bytes<Size>& c1, const fixed_bytes<Size>& c2) {
      return c1._data >= c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 <= c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator <=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data <= c2._data;
   }


   /**
    *  Serialize a fixed_bytes into a stream
    *
    *  @brief Serialize a fixed_bytes
    *  @param ds - The stream to write
    *  @param d - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
   template<typename DataStream, size_t Size>
   inline DataStream& operator<<(DataStream& ds, const fixed_bytes<Size>& d) {
      auto arr = d.extract_as_byte_array();
      ds.write( (const char*)arr.data(), arr.size() );
      return ds;
   }

   /**
    *  Deserialize a fixed_bytes from a stream
    *
    *  @brief Deserialize a fixed_bytes
    *  @param ds - The stream to read
    *  @param d - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
   template<typename DataStream, size_t Size>
   inline DataStream& operator>>(DataStream& ds, fixed_bytes<Size>& d) {
      std::array<uint8_t, Size> arr;
      ds.read( (char*)arr.data(), arr.size() );
      d = fixed_bytes<Size>( arr );
      return ds;
   }

}
namespace eosio_evm
{
  /**
   * Conversions
   */
  static inline std::string bin2hex(const std::vector<uint8_t>& bin)
  {
    std::string res;
    const char hex[] = "0123456789abcdef";
    for(auto byte : bin) {
      res += hex[byte >> 4];
      res += hex[byte & 0xf];
    }

    return res;
  }

  template<unsigned N, typename T>
  static inline std::string bin2hex(const std::array<T, N>& bin)
  {
    std::string res;
    const char hex[] = "0123456789abcdef";
    for(auto byte : bin) {
      res += hex[byte >> 4];
      res += hex[byte & 0xf];
    }

    return res;
  }

  inline constexpr bool is_precompile(uint256_t address) {
    return address >= 1 && address <= 65535;
  }

  inline constexpr int64_t num_words(uint64_t size_in_bytes)
  {
    return (static_cast<int64_t>(size_in_bytes) + (WORD_SIZE - 1)) / WORD_SIZE;
  }

  template <typename T>
  static T shrink(uint256_t i)
  {
    return static_cast<T>(i & std::numeric_limits<T>::max());
  }

  inline std::array<uint8_t, 32u> toBin(const Address& address)
  {
    std::array<uint8_t, 32> address_bytes = {};
    intx::be::unsafe::store(address_bytes.data(), address);
    return address_bytes;
  }

  inline const std::array<uint8_t, 32u> fromChecksum160(const eosio::checksum160 input)
  {
    std::array<uint8_t, 32U> output = {};
    auto input_bytes = input.extract_as_byte_array();
    std::copy(std::begin(input_bytes), std::end(input_bytes), std::begin(output) + 12);
    return output;
  }

  inline eosio::checksum160 toChecksum160(const std::array<uint8_t, 32u>& input)
  {
    std::array<uint8_t, 20> output = {};
    std::copy(std::begin(input) + 12, std::end(input), std::begin(output));
    return eosio::checksum160(output);
  }

  inline eosio::checksum256 toChecksum256(const Address& address)
  {
    return eosio::checksum256( toBin(address) );
  }

  static inline eosio::checksum256 pad160(const eosio::checksum160 input)
  {
    return eosio::checksum256( fromChecksum160(input) );
  }

  static inline Address checksum160ToAddress(const eosio::checksum160& input) {
    const std::array<uint8_t, 32u>& checksum = fromChecksum160(input);
    return intx::be::unsafe::load<uint256_t>(checksum.data());
  }
  static inline eosio::checksum160 addressToChecksum160(const Address& input) {
    return toChecksum160( toBin(input) );
  }

  // Do not use for addresses, only key for Account States
  static inline uint256_t checksum256ToValue(const eosio::checksum256& input) {
    std::array<uint8_t, 32U> output = {};
    auto input_bytes = input.extract_as_byte_array();
    std::copy(std::begin(input_bytes), std::end(input_bytes), std::begin(output));

    return intx::be::unsafe::load<uint256_t>(output.data());
  }

  /**
   * Keccak (SHA3) Functions
   */
  inline void keccak_256(
    const unsigned char* input,
    unsigned int inputByteLen,
    unsigned char* output)
  {
    // Ethereum started using Keccak and called it SHA3 before it was finalised.
    SHA3_CTX context;
    keccak_init(&context);
    keccak_update(&context, input, inputByteLen);
    keccak_final(&context, output);
  }

  using KeccakHash = std::array<uint8_t, 32u>;

  inline KeccakHash keccak_256(const uint8_t* begin, size_t byte_len)
  {
    KeccakHash h;
    keccak_256(begin, byte_len, h.data());
    return h;
  }

  inline KeccakHash keccak_256(const std::string& s)
  {
    return keccak_256((const uint8_t*)s.data(), s.size());
  }

  inline KeccakHash keccak_256(const std::vector<uint8_t>& v)
  {
    return keccak_256(v.data(), v.size());
  }

  template <size_t N>
  inline KeccakHash keccak_256(const std::array<uint8_t, N>& a)
  {
    return keccak_256(a.data(), N);
  }

  /**
   * RLP
   */
  inline Address generate_address(const Address& sender, uint256_t nonce) {
    // RLP encode and keccak hash
    const auto rlp_encoding = rlp::encode(sender, nonce);
    std::array<uint8_t, 32u> buffer = keccak_256(rlp_encoding);

    // Copy right 160 bits from keccak buffer
    uint8_t right_160[32] = {};
    memcpy(right_160 + 12u, buffer.data() + 12u, 20u);

    // Return as address
    return intx::be::load<uint256_t>(right_160);
  };

  inline Address generate_address2(const Address& sender, const uint256_t& salt, const std::vector<uint8_t>& init_code) {
    // Sender as bytes
    auto sender_bytes = toBin(sender);

    // Code hash
    std::array<uint8_t, 32u> code_hash = keccak_256(init_code);

    // Salt array
    uint8_t salt_bytes[32] = {};
    intx::be::store(salt_bytes, salt);

    // Concatenate
    std::array<uint8_t, 85u> final = {0xff};
    std::copy(std::begin(sender_bytes) + 12, std::end(sender_bytes), std::begin(final) + 1);
    std::copy(salt_bytes, salt_bytes + 32, std::begin(final) + 21);
    std::copy(std::begin(code_hash), std::end(code_hash), std::begin(final) + 53);

    // Final hash
    auto final_hash = keccak_256(final);

    // Copy right 160 bits from keccak buffer
    uint8_t right_160[32] = {};
    memcpy(right_160 + 12u, final_hash.data() + 12u, 20u);

    // Return as address
    return intx::be::load<uint256_t>(right_160);
  };


} // namespace eosio_evm
