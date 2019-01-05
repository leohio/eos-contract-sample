#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp> // asset, symbol
#include <eosiolib/crypto.h> // assert_recover_key
#include <eosiolib/action.hpp> // inline action

using namespace eosio;
using std::string;
using std::vector;

class [[eosio::contract]] pcs : public eosio::contract {
    public:
        /**
         * Constructor
        **/

        pcs( name receiver, name code, datastream<const char*> ds ):
    	    contract::contract( receiver, code, ds ),
            tokens( receiver, receiver.value ),
            eosbt( receiver, receiver.value ),
            bids( receiver, receiver.value ) {}

        /**
         * Public Function
        **/

        [[eosio::action]] void       create( name issuer, symbol_code sym );
        [[eosio::action]] void      destroy( name claimer, symbol_code sym );
        [[eosio::action]] void        issue( name user, symbol_code sym, vector<capi_public_key> subkeys, string memo );
        [[eosio::action]] void   transferid( name from, name to, symbol_code sym, uint64_t id, string memo );
	    [[eosio::action]] void     transfer( name from, name to, symbol_code sym, string memo );
        [[eosio::action]] void         burn( name owner, uint64_t token_id );
	    [[eosio::action]] void  setrampayer( name payer, uint64_t token_id );
        [[eosio::action]] void   refleshkey( name owner, uint64_t token_id, capi_public_key subkey );
        [[eosio::action]] void         lock( name accuser, uint64_t token_id, string data, capi_signature sig );
        [[eosio::action]] void     servebid( name owner, uint64_t token_id, asset price, string memo );
        [[eosio::action]] void          buy( name buyer, uint64_t token_id, string memo );
        [[eosio::action]] void   sendandbuy( name buyer, uint64_t token_id, string memo );
        [[eosio::action]] void    cancelbid( name owner, uint64_t token_id );
        [[eosio::action]] void resisteruris( name user, symbol_code sym, vector<string> uris );
        [[eosio::action]] void      setpvid( name claimer, symbol_code sym, uint64_t uri_id, uint64_t count );
        [[eosio::action]] void    setpvdata( name claimer, symbol_code sym, string uri, uint64_t count );
        [[eosio::action]] void   removepvid( name claimer, symbol_code sym, uint64_t uri_id );
        [[eosio::action]] void removepvdata( name claimer, symbol_code sym, string uri );
        [[eosio::action]] void     setoffer( name payer, symbol_code sym, string uri, asset price );
        [[eosio::action]] void  acceptoffer( name manager, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void  removeoffer( name payer, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void  stopcontent( name manager, symbol_code sym, uint64_t content_id );
        [[eosio::action]] void  dropcontent( name manager, symbol_code sym, uint64_t content_id );
        [[eosio::action]] void      receive();

        /**
         * Struct
        **/

        struct [[eosio::table]] account {
            asset balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        // statistics of currency
        struct [[eosio::table]] stats {
            asset supply;
            name issuer;
            uint64_t primary_key() const { return supply.symbol.code().raw(); }
            uint64_t get_issuer() const { return issuer.value; }
        };

        struct [[eosio::table]] token {
            uint64_t id;
            capi_public_key subkey;
            name owner;
            symbol_code sym;
            uint8_t active;

            uint64_t primary_key() const { return id; }
            uint64_t get_owner() const { return owner.value; }
            capi_public_key get_subkey() const { return subkey; }
	        uint64_t get_symbol() const { return sym.raw(); }
            uint8_t get_active() const { return active; }

    	    // generated token global uint128_t based on token id and
    	    // contract name, passed in the argument
    	    uint128_t get_global_id( name self ) const {
                // static_cast は型変換を明示
                // uint64 -> uint128 のキャスト
        		uint128_t self_128 = static_cast<uint128_t>(self.value);
                // uint64 -> uint128 のキャスト
        		uint128_t id_128 = static_cast<uint128_t>(id);
                // << は左ビットシフト, | はビット和
        		uint128_t res = (self_128 << 64) | (id_128);
        		return res;
    	    }

    	    string get_unique_name() const {
        		string unique_name = sym.to_string() + "#" + std::to_string(id);
        		return unique_name;
    	    }
        };

        // dex order board
        struct [[eosio::table]] order {
            uint64_t id;
            asset price;
            name owner;
            symbol_code sym;

            uint64_t primary_key() const { return id; }
            asset get_price() const { return price; }
            uint64_t get_owner() const { return owner.value; }
            uint64_t get_symbol() const { return sym.raw(); }
        };

        struct [[eosio::table]] offer {
            uint64_t id;
            asset price;
            name payer;
            string uri;

            uint64_t primary_key() const { return id; }
            asset get_price() const { return price; }
            uint64_t get_payer() const { return payer.value; }
            string get_uri() const { return uri; }
        };

        struct [[eosio::table]] content {
            uint64_t id;
            asset price;
            name payer;
            string uri;
            uint64_t timestamp;
            uint8_t active;

            uint64_t primary_key() const { return id; }
            asset get_price() const { return price; }
            uint64_t get_payer() const { return payer.value; }
            string get_uri() const { return uri; }
            uint64_t get_timestamp() const { return timestamp; }
            uint8_t get_active() const { return active; }
        };

        struct [[eosio::table]] pvcount {
            uint64_t id;
            string uri;
            uint64_t count;

            uint64_t primary_key() const { return id; }
            string get_uri() const { return uri; }
            uint64_t get_count() const { return count; }
        };

        struct [[eosio::table]] balance {
            name username;
            asset quantity;

            uint64_t primary_key() const { return username.value; }
            uint64_t get_quantity() const { return quantity.amount; }
        };

        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        /**
         * Multi Index
        **/

    	using account_index = eosio::multi_index< name("accounts"), account >;

        using balance_index = eosio::multi_index< name("balance"), balance >;

    	using currency_index = eosio::multi_index< name("currency"), stats,
    	    indexed_by< name("byissuer"), const_mem_fun< stats, uint64_t, &stats::get_issuer> > >;

    	using token_index = eosio::multi_index< name("token"), token,
    	    indexed_by< name("byowner"), const_mem_fun<token, uint64_t, &token::primary_key> >,
            indexed_by< name("bysymbol"), const_mem_fun<token, uint64_t, &token::get_symbol> > >;

        using bid_index = eosio::multi_index< name("bid"), order,
            indexed_by< name("byowner"), const_mem_fun<order, uint64_t, &order::get_owner> >,
            indexed_by< name("bysymbol"), const_mem_fun<order, uint64_t, &order::get_symbol> > >;

        using pv_count_index = eosio::multi_index< name("pvcount"), pvcount,
            indexed_by< name("byuriid"), const_mem_fun<pvcount, uint64_t, &pvcount::primary_key> >,
            indexed_by< name("bycount"), const_mem_fun<pvcount, uint64_t, &pvcount::get_count> > >;

        using offer_index = eosio::multi_index< name("offer"), offer >;

        using content_index = eosio::multi_index< name("content"), content,
            indexed_by< name("bytime"), const_mem_fun<content, uint64_t, &content::get_timestamp> > >;

    private:
	    token_index tokens;
        balance_index eosbt; // table of eos balance
        bid_index bids;

        /**
         * Private Function
        **/

        void mint_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer );
        uint64_t get_hex_digit( string memo );
        void transfer_eos( name to, asset value, string memo );
        void set_uri( name user, symbol_code sym, string uri );
        void sub_eos_balance( name owner, asset quantity );
        void add_eos_balance( name owner, asset quantity, name ram_payer );
        void sub_balance( name owner, symbol_code sym );
        void add_balance( name owner, symbol_code sym, name ram_payer );
        void sub_supply( symbol_code sym, uint64_t value );
        void add_supply( symbol_code sym, uint64_t value );
        void sub_deposit( name user, asset quantity );
        uint64_t find_own_token( name owner, symbol_code sym );
        uint64_t find_pvdata_by_uri( symbol_code sym, string uri );
        uint64_t find_offer_by_uri( symbol_code sym, string uri );
};
