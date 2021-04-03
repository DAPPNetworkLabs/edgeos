#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/string.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>

#define LINK_RECEIPT_FLAG 0x80000000
#define PUSH_FOREIGN_MESSAGE_METHOD_ID "7d29a9f0"
#define BYTES_ARRAY_POINTER_PUSH_FOREIGN_MESSAGE "0000000000000000000000000000000000000000000000000000000000000040"

namespace hyperlink {

class [[eosio::contract("ethtokenpeg")]] ethtokenpeg : public eosio::contract {
    public:
        using contract::contract;

        ethtokenpeg( eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds )
        : contract(receiver, code, ds) { }

        struct message {
          bool success;
          eosio::name account;            
          int64_t amount;
          eosio::checksum160 address;
        };

        struct message_payload {
            uint64_t id;
            std::vector<char> data;
        };

        struct state_params {
            uint64_t last_irreversible_block_time;
            uint64_t available_message_id;
            uint64_t available_batch_id;
            uint64_t next_inbound_message_id;
        };

        TABLE token_settings_t {
          eosio::name token_contract;
          eosio::symbol token_symbol;
          uint64_t min_transfer;
          bool transfers_enabled;
          bool can_issue; // true if token is being bridged to this chain, else false 
        };

        typedef eosio::singleton<"config"_n, token_settings_t> token_settings_table;
        typedef eosio::multi_index<"config"_n, token_settings_t> token_settings_table_abi;
 
        TABLE settings_t {

            bool processing_enabled;
            uint64_t last_irreversible_block_time;
            uint64_t available_message_id;
            uint64_t available_batch_id;
            uint64_t next_inbound_message_id;
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings_table;
        typedef eosio::multi_index<"settings"_n, settings_t> settings_table_abi;

        // contains all transfers with given timestamp
        TABLE pending_messages_t {
            std::vector<char> message;
            uint64_t id;
            uint32_t received_block_time;
            uint64_t primary_key()const { return id; }
            uint64_t by_time()const { return received_block_time; }
        };

        //TODO: table for confirmed after consensus
        typedef eosio::multi_index<"pmessages"_n, pending_messages_t, 
        eosio::indexed_by<"bytime"_n, eosio::const_mem_fun<pending_messages_t, uint64_t, &pending_messages_t::by_time>>> pending_messages_table_t;

        typedef eosio::multi_index<"imessages"_n, pending_messages_t, 
        eosio::indexed_by<"bytime"_n, eosio::const_mem_fun<pending_messages_t, uint64_t, &pending_messages_t::by_time>>> inbound_messages_table_t;

        typedef eosio::multi_index<"fmessages"_n, pending_messages_t, 
        eosio::indexed_by<"bytime"_n, eosio::const_mem_fun<pending_messages_t, uint64_t, &pending_messages_t::by_time>>> failed_messages_table_t;

        std::string clean_eth_address(std::string address) {
          // remove initial 0x if there
          if (address[1] == 'x') {
              return address.substr(2);
          }
          return address;
        }

        std::vector<char> HexToBytes(const std::string& hex) {
          std::vector<char> bytes;
          for (unsigned int i = 0; i < hex.length(); i += 2) {
              std::string byteString = hex.substr(i, 2);
              char byte = (char) strtol(byteString.c_str(), NULL, 16);
              bytes.push_back(byte);
          }
          return bytes;
        }

        std::string BytesToHex(std::vector<char> data)
        {
          std::string s(data.size() * 2, ' ');
          char hexmap[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
          for (int i = 0; i < data.size(); ++i) {
              s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
              s[2 * i + 1] = hexmap[data[i] & 0x0F];
          }
          return s;
        }

        eosio::checksum160 HexToAddress(const std::string& hex) {
          auto bytes = HexToBytes(clean_eth_address(hex));
          std::array<uint8_t, 20> arr;
          std::copy_n(bytes.begin(), 20, arr.begin());
          return eosio::checksum160(arr);
        }

        std::string AddressToHex(const eosio::checksum160& address) {
          std::vector<char> bytes;
          auto arr = address.extract_as_byte_array();
          std::copy_n(arr.begin(), 20, bytes.begin());
          return BytesToHex(bytes);
        }

        int64_t reverse(int64_t value) {
          std::vector<char> value_v(8);
          std::vector<char> value_r(8);
          int64_t reversed;

          memcpy(value_v.data(), &value, value_v.size());
          std::reverse_copy(&value_v[0], &value_v[8], &value_r[0]);
          memcpy(&reversed, value_r.data(), value_r.size());
          return reversed;
        }

        message unpack(const std::vector<char>& data) {
          auto unpacked = eosio::unpack<message>(data);
          auto _amount = unpacked.amount;
          unpacked.amount = reverse(_amount);
          return unpacked;
        }

        std::vector<char> pack(const message& data) {
          auto _data = data;
          _data.amount = reverse(_data.amount);
          auto packed = eosio::pack(_data);
          return packed;
        }

        std::string pad_left(std::string data) {
            std::string s(64 - data.size(), '0');
            return s.append(data);
        }
        std::string pad_right(std::string data) {
            auto diff = 64 - (data.size() % 64);
            std::string s(diff, '0');
            return data.append(s);
        }

        std::string data_to_hex_string(std::vector<char> data)
        {
            std::string s(data.size() * 2, ' ');
            char hexmap[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
            for (int i = 0; i < data.size(); ++i) {
                s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
                s[2 * i + 1] = hexmap[data[i] & 0x0F];
            }
            return s;
        }

        template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
            static const char* digits = "0123456789ABCDEF";
            std::string rc(hex_len,'0');
            for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
                rc[i] = digits[(w>>j) & 0x0f];
            return rc;
        }

        // TODO: don't use batched messages table
        //bool handle_receipt(const message_payload& payload, const bool failed = false) {
        bool handle_receipt(std::vector<char> data, uint64_t id, const bool failed = false) {
            auto _self = eosio::name(eosio::current_receiver());
            pending_messages_table_t pending_messages(_self, _self.value);
            auto messageId = id - LINK_RECEIPT_FLAG;
            //auto outgoing = batched_messages.find(messageId);
            auto outgoing = pending_messages.find(messageId);
            if(outgoing != pending_messages.end()) {       
                if(failed) {
                    failed_messages_table_t failed_messages(_self, _self.value);
                    auto failed = failed_messages.find(id);
                    if(failed == failed_messages.end()) {
                        failed_messages.emplace(_self, [&](auto& a){
                            a.message = data;
                            a.received_block_time = eosio::current_time_point().sec_since_epoch();
                        });
                    }
                }
                pending_messages.erase(outgoing);
                return true;
            }    
            return false;
        }

        void push_receipt(std::vector<char> data) {    
            auto _self = eosio::name(eosio::current_receiver());
            settings_table settings_singleton(_self, _self.value);
            settings_t settings = settings_singleton.get_or_default();

            auto message_id = settings.available_message_id;
            auto eth_data = get_eth_data(message_id, data);
            pending_messages_table_t pending_messages(_self, _self.value);
            pending_messages.emplace(_self, [&]( auto& a ){
                a.message = eth_data;
                a.id = message_id;
                a.received_block_time = eosio::current_time_point().sec_since_epoch();
            });

            settings.available_message_id++;
            settings_singleton.set(settings, _self);   
        }

        std::vector<char> get_eth_data(uint64_t message_id, std::vector<char> payload)
        {    
            std::string message_id_string = pad_left(n2hexstr<uint64_t>(message_id));
            std::string payload_string = data_to_hex_string(payload);
            std::string payload_length_string = pad_left(n2hexstr<uint64_t>(payload_string.length() / 2));

            std::string data = std::string(PUSH_FOREIGN_MESSAGE_METHOD_ID) +\
            message_id_string +\
            BYTES_ARRAY_POINTER_PUSH_FOREIGN_MESSAGE +\
            payload_length_string +\
            pad_right(payload_string);

            return std::vector<char>(data.begin(), data.end());
        }

        void pushMessage(std::vector<char> data, uint64_t force_message_id = 0) {    
            auto _self = eosio::name(eosio::current_receiver());
            settings_table settings_singleton(_self, _self.value);
            settings_t settings = settings_singleton.get_or_default();    
            auto message_id = force_message_id;
            if(force_message_id == 0) {
                message_id = settings.available_message_id;
                settings.available_message_id++;
            }
            auto eth_data = get_eth_data(message_id, data);
            pending_messages_table_t pending_messages(_self, _self.value);
            pending_messages.emplace(_self, [&]( auto& a ){
                a.message = eth_data;
                a.id = message_id;
                a.received_block_time = eosio::current_time_point().sec_since_epoch();
            });
            settings_singleton.set(settings, _self);
        }

        void handle_message() {
            auto _self = eosio::name(eosio::current_receiver());
            auto current_block_time = eosio::current_time_point().sec_since_epoch();
            inbound_messages_table_t inbound_messages(_self, _self.value);
            // TODO: by id or time..?
            auto ordered_messages = inbound_messages.get_index<"bytime"_n>();
            auto message = ordered_messages.begin();
            eosio::check(message != ordered_messages.end(), "{abort_service_request}");
            if(message->id < LINK_RECEIPT_FLAG) {
                pushMessage(message_received(message->message), message->id + LINK_RECEIPT_FLAG);
            } else {
                handle_receipt(message->message, message->id);
            }
            ordered_messages.erase(message);
        }

        void setlinkstate (state_params new_state) {
            auto _self = eosio::name(eosio::current_receiver()); 
            require_auth(_self);
            settings_table settings_singleton(_self, _self.value); 
            settings_t settings = settings_singleton.get_or_default(); 
            settings.last_irreversible_block_time = new_state.last_irreversible_block_time; 
            settings.available_message_id = new_state.available_message_id; 
            settings.available_batch_id = new_state.available_batch_id; 
            settings.next_inbound_message_id = new_state.next_inbound_message_id; 
        }

        void transfer(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo) {
          token_settings_table settings_singleton(_self, _self.value);
          token_settings_t settings = settings_singleton.get_or_default();
          
          // validate proper transfer
          if (get_first_receiver() != settings.token_contract || from == _self) {
            return;
          }
          eosio::check(quantity.symbol == settings.token_symbol, "Incorrect symbol");
          eosio::check(quantity.amount >= settings.min_transfer, "Transferred amount is less than minimum required.");
          eosio::check(settings.transfers_enabled, "transfers disabled");
          
          // the memo should contain ONLY the eth address.
          message new_transfer = { true, from, quantity.amount, HexToAddress(memo) };
          auto transfer = pack(new_transfer);

          #ifdef LINK_DEBUG    
            history_table histories(_self, _self.value);
            histories.emplace(_self, [&]( auto& a ){
              a.id = histories.available_primary_key();
              a.data = transfer;
              a.msg = new_transfer;
              a.type = "outgoingMessage";
            });
          #endif
          pushMessage(transfer);
        }

        std::vector<char> message_received(const std::vector<char>& data) {
          token_settings_table settings_singleton(_self, _self.value);
          token_settings_t settings = settings_singleton.get_or_default();
          auto orig_data = data;      
          auto transfer_data = unpack(data);

          std::string memo = "LiquidApps LiquidLink Transfer - Received from Ethereum account 0x";// + AddressToHex(transfer_data.address);

          //TODO: We should check for things like destination account exists, funds are available, a token account exists, etc
          // and then we should be able to return that the message is a failure if those conditions don't pass
          eosio::asset quantity = eosio::asset(transfer_data.amount, settings.token_symbol);    
          if (settings.can_issue) {
            eosio::action(eosio::permission_level{_self, "active"_n}, settings.token_contract, "issue"_n,
              std::make_tuple(eosio::name(transfer_data.account), quantity, memo))
            .send();
          } else {
            eosio::action(eosio::permission_level{_self, "active"_n}, settings.token_contract, "transfer"_n,
              std::make_tuple(_self, eosio::name(transfer_data.account), quantity, memo))
            .send();
          }

          #ifdef LINK_DEBUG  
            history_table histories(_self, _self.value);
            histories.emplace(_self, [&]( auto& a ){
              a.id = histories.available_primary_key();
              a.data = orig_data;
              a.msg = transfer_data;
              a.type = "incomingMessage";
            });
          #endif

          return orig_data;
        } 

        void receipt_received(const std::vector<char>& data) {
          // deserialize original message to get the quantity and original sender to refund
          token_settings_table settings_singleton(_self, _self.value);
          token_settings_t settings = settings_singleton.get_or_default();
          auto transfer_receipt = unpack(data);

          std::string memo = "LiquidApps LiquidLink Transfer Failed - Attempted to send to Ethereum account 0x";// + AddressToHex(transfer_receipt.address);
          eosio::asset quantity = eosio::asset(transfer_receipt.amount, settings.token_symbol);

          if (!transfer_receipt.success) {
            // return locked tokens in case of failure
            if (settings.can_issue) {
              eosio::action(eosio::permission_level{_self, "active"_n}, settings.token_contract, "issue"_n,
                std::make_tuple(eosio::name(transfer_receipt.account), quantity, memo))
              .send();
            } else {
              eosio::action(eosio::permission_level{_self, "active"_n}, settings.token_contract, "transfer"_n,
                std::make_tuple(_self, eosio::name(transfer_receipt.account), quantity, memo))
              .send();
            }

            #ifdef LINK_DEBUG 
              history_table histories(_self, _self.value);
              histories.emplace(_self, [&]( auto& a ){
                a.id = histories.available_primary_key();
                a.data = data;
                a.msg = transfer_receipt;
                a.type = "receiptFailure";
              });
            #endif
          } else {
            #ifdef LINK_DEBUG 
              //TODO: should we burn the tokens or send them to a "holding account"
              history_table histories(_self, _self.value);
              histories.emplace(_self, [&]( auto& a ){
                a.id = histories.available_primary_key();
                a.data = data;
                a.msg = transfer_receipt;
                a.type = "receiptSuccess";
              });
            #endif  
          }
        }

        ACTION init(
          bool processing_enabled,
          eosio::name token_contract,
          eosio::symbol token_symbol,
          uint64_t min_transfer,
          bool transfers_enabled,
          bool can_issue
        )
        {
            eosio::require_auth(_self);
            settings_table settings_singleton(_self, _self.value);
            settings_t settings = settings_singleton.get_or_default();
            settings.processing_enabled = processing_enabled; \
            settings_singleton.set(settings, _self);

            token_settings_table token_settings_singleton(_self, _self.value);
            token_settings_t token_settings = token_settings_singleton.get_or_default();
            token_settings.token_contract = token_contract;
            token_settings.token_symbol = token_symbol;
            token_settings.min_transfer = min_transfer;
            token_settings.can_issue = can_issue;
            token_settings.transfers_enabled = transfers_enabled;
            token_settings_singleton.set(token_settings, _self);
        }

        ACTION pushinbound(std::vector<char> message, uint64_t id) {
            auto _self = eosio::name(eosio::current_receiver()); 
            settings_table settings_singleton(_self, _self.value); 
            settings_t settings = settings_singleton.get_or_default(); 
            eosio::check(settings.processing_enabled, "processing disabled"); 
            // TODO: validate/authorize
            inbound_messages_table_t inbound_messages(_self, _self.value);
            inbound_messages.emplace(_self, [&]( auto& a ){
                a.message = message;
                a.id = id;
                a.received_block_time = eosio::current_time_point().sec_since_epoch();
            });
            settings.next_inbound_message_id++;
            settings_singleton.set(settings, _self);
            handle_message();
        }
};

}