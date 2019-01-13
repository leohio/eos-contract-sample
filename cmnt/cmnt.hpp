/**
 * @environment
 *     eosio.cdt v1.3.2
**/

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp> /* asset, symbol */
// #include <eosiolib/crypto.h> /* assert_recover_key */
#include <eosiolib/action.hpp> /* inline action */

using namespace eosio;
using std::string;
using std::vector;

class [[eosio::contract]] cmnt : public eosio::contract {
    public:
        /**
         * Constructor
        **/

        cmnt( name receiver, name code, datastream<const char*> ds ):
    	    contract::contract( receiver, code, ds ),
            token_table( receiver, receiver.value ),
            deposit_table( receiver, receiver.value ),
            total_deposit_table( receiver, receiver.value ),
            currency_table( receiver, receiver.value ),
            sell_order_table( receiver, receiver.value ),
            contents_table( receiver, receiver.value ),
            pv_rate_table( receiver, receiver.value ) {}

        /**
         * Public Function
        **/

        [[eosio::action]] void       create( name issuer, symbol_code sym );
        [[eosio::action]] void      destroy( symbol_code sym );
        [[eosio::action]] void        issue( name user, asset quantity, string memo );
        [[eosio::action]] void  issueunlock( name user, asset quantity, vector<capi_public_key> subkeys, string memo );
        [[eosio::action]] void       obtain( name user, symbol_code sym, string memo );
        [[eosio::action]] void transferbyid( name from, name to, uint64_t id, string memo );
	    [[eosio::action]] void     transfer( name from, name to, symbol_code sym, string memo );
        [[eosio::action]] void     burnbyid( name owner, uint64_t token_id );
        [[eosio::action]] void         burn( name owner, asset quantity );
        [[eosio::action]] void   refleshkey( name owner, uint64_t token_id, capi_public_key subkey );
        [[eosio::action]] void         lock( name claimer, uint64_t token_id, string data, capi_signature sig );
        [[eosio::action]] void     sellbyid( name owner, uint64_t token_id, asset price );
        [[eosio::action]] void issueandsell( asset quantity, asset price, string memo );
        [[eosio::action]] void          buy( name user, uint64_t token_id, string memo );
        [[eosio::action]] void buyandunlock( name user, uint64_t token_id, capi_public_key subkey, string memo );
        [[eosio::action]] void   sendandbuy( name user, uint64_t token_id, capi_public_key subkey, string memo );
        [[eosio::action]] void   cancelbyid( name owner, uint64_t token_id );
        [[eosio::action]] void       cancel( name owner, asset quantity );
        [[eosio::action]] void   cancelburn( name owner, asset quantity );
        [[eosio::action]] void     withdraw( name user, asset quantity, string memo );
        [[eosio::action]] void     setoffer( name provider, symbol_code sym, string uri, asset price );
        [[eosio::action]] void  acceptoffer( name manager, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void  removeoffer( name provider, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void   addpvcount( uint64_t content_id, uint64_t pv_count );
        [[eosio::action]] void updatepvrate();
        [[eosio::action]] void resetpvcount( uint64_t content_id );
        [[eosio::action]] void stopcontents( name manager, uint64_t content_id );
        [[eosio::action]] void dropcontents( name manager, uint64_t content_id );
        [[eosio::action]] void      receive();

        /**
         * Struct
        **/

        struct [[eosio::table]] account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        struct [[eosio::table]] deposit {
            name owner;
            asset quantity;

            uint64_t primary_key() const { return owner.value; }
            uint64_t get_quantity() const { return quantity.amount; }
        };

        struct [[eosio::table]] currency {
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
            uint64_t get_price() const { return price.amount; }
            uint64_t get_owner() const { return owner.value; }
            uint64_t get_symbol() const { return sym.raw(); }
        };

        struct [[eosio::table]] offer {
            uint64_t id;
            asset price;
            name provider;
            string uri;

            uint64_t primary_key() const { return id; }
            asset get_price() const { return price; }
            uint64_t get_provider() const { return provider.value; }
            string get_uri() const { return uri; }
        };

        struct [[eosio::table]] content {
            uint64_t id;
            symbol_code sym;
            asset price;
            name provider;
            string uri;
            uint64_t count;
            uint64_t accepted; // acceptoffer 時の timestamp
            uint8_t active;

            uint64_t primary_key() const { return id; }
            uint64_t get_symbol() const { return sym.raw(); }
            asset get_price() const { return price; }
            uint64_t get_provider() const { return provider.value; }
            string get_uri() const { return uri; }
            uint64_t get_count() const { return count; }
            uint64_t get_accepted() const { return accepted; }
            uint8_t get_active() const { return active; }
        };

        struct [[eosio::table]] pvcount {
            uint64_t timestamp;
            uint64_t count;

            uint64_t primary_key() const { return timestamp; }
            uint64_t get_count() const { return count; }
        };

        struct [[eosio::table]] pvrate {
            uint64_t timestamp;
            float_t rate;

            uint64_t primary_key() const { return timestamp; }
            float_t get_rate() const { return rate; }
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

        using deposit_index = eosio::multi_index< name("deposit"), deposit >;

        using total_deposit_index = eosio::multi_index< name("totaldeposit"), account >;

    	using currency_index = eosio::multi_index< name("currency"), currency,
    	    indexed_by< name("byissuer"), const_mem_fun<currency, uint64_t, &currency::get_issuer> > >;

        using token_index = eosio::multi_index< name("token"), token,
    	    indexed_by< name("byowner"), const_mem_fun<token, uint64_t, &token::primary_key> >,
            indexed_by< name("bysymbol"), const_mem_fun<token, uint64_t, &token::get_symbol> > >;

        using sell_order_index = eosio::multi_index< name("sellorder"), order,
            indexed_by< name("byowner"), const_mem_fun<order, uint64_t, &order::get_owner> >,
            indexed_by< name("bysymbol"), const_mem_fun<order, uint64_t, &order::get_symbol> > >;

        using offer_index = eosio::multi_index< name("offer"), offer >;

        using content_index = eosio::multi_index< name("contents"), content,
            indexed_by< name("bytime"), const_mem_fun<content, uint64_t, &content::get_accepted> >,
            indexed_by< name("bysymbol"), const_mem_fun<content, uint64_t, &content::get_symbol> > >;

        using pv_count_index = eosio::multi_index< name("pv"), pvcount >;

        using pv_rate_index = eosio::multi_index< name("pvrate"), pvrate >;

    private:
	    token_index token_table;
        deposit_index deposit_table; // table of eos balance
        total_deposit_index total_deposit_table;
        currency_index currency_table;
        sell_order_index sell_order_table;
        content_index contents_table;
        pv_rate_index pv_rate_table;

        /**
         * Private Function
        **/

        uint64_t mint_token( name user, symbol_code sym, name ram_payer );
        uint64_t mint_unlock_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer );
        void add_sell_order( name owner, uint64_t token_id, asset price );
        void transfer_eos( name to, asset value, string memo );
        void add_balance( name owner, asset quantity, name ram_payer );
        void decrease_balance( name owner, symbol_code sym );
        void add_supply( asset quantity );
        void decrease_supply( symbol_code sym );
        void add_deposit( name owner, asset quantity, name ram_payer );
        void sub_deposit( name owner, asset quantity );
        uint64_t find_own_token( name owner, symbol_code sym );
        uint64_t find_own_order( name owner, symbol_code sym );
        uint64_t find_pvdata_by_uri( symbol_code sym, string uri );
        uint64_t find_offer_by_uri( symbol_code sym, string uri );
        uint64_t get_sum_of_pv_count( uint64_t contents_id );
        vector<string> split_by_comma( string memo );
};
