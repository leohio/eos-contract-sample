#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp> /* asset, symbol */
// #include <eosiolib/crypto.h> /* assert_recover_key */
#include <eosiolib/action.hpp> /* unack_action_data, require_recipent, require_auth, action */

using namespace eosio;
using std::string;
using std::vector;

class [[eosio::contract]] cmnty : public eosio::contract {
    public:
        /**
         * Constructor
        **/

        cmnty( name receiver, name code, datastream<const char*> ds ):
    	    contract::contract( receiver, code, ds ),
            currency_table( receiver, receiver.value ),
            total_deposit_table( receiver, receiver.value ),
            global_table( receiver, receiver.value )
        {
            // auto global_data = global_table.find( 0 );
            // if ( global_data == global_table.end() ) {
            //     uint64_t now = current_time();
            //     global_table.emplace( get_self(), [&]( auto& data ) {
            //         data.last_modified = now;
            //         data.pvcount = 0;
            //         data.pvrate = 0;
            //         data.active = 1;
            //     });
            // }
        }

        /**
         * Public Function
        **/

        struct pv_data_str {
            symbol_code sym;
            uint64_t contents_id;
            uint64_t pv_count;
        };

        [[eosio::action]] void       create( name issuer, symbol_code sym );
        [[eosio::action]] void      destroy( symbol_code sym );
        [[eosio::action]] void        issue( name user, asset quantity, string memo );
        // [[eosio::action]] void  issueunlock( name user, asset quantity, vector<capi_public_key> subkeys, string memo );
        [[eosio::action]] void transferbyid( name from, name to, symbol_code sym, uint64_t token_id, string memo );
	    [[eosio::action]] void     transfer( name from, name to, symbol_code sym, string memo );
        [[eosio::action]] void     burnbyid( symbol_code sym, uint64_t token_id );
        [[eosio::action]] void         burn( name owner, asset quantity );
        [[eosio::action]] void   refleshkey( symbol_code sym, uint64_t token_id, capi_public_key subkey );
        // [[eosio::action]] void         lock( name claimer, uint64_t token_id, string data, capi_signature sig );
        [[eosio::action]] void addsellobyid( symbol_code sym, uint64_t token_id, asset price, string memo );
        [[eosio::action]] void addsellorder( name seller, asset quantity, asset price, string memo );
        [[eosio::action]] void issueandsell( asset quantity, asset price, string memo );
        [[eosio::action]] void buyfromorder( name buyer, symbol_code sym, uint64_t token_id, string memo );
        // [[eosio::action]] void buyandunlock( name buyer, symbol_code sym, uint64_t token_id, capi_public_key subkey, string memo );
        // [[eosio::action]] void   sendandbuy( name buyer, symbol_code sym, uint64_t token_id, capi_public_key subkey, string memo );
        [[eosio::action]] void cancelsobyid( symbol_code sym, uint64_t token_id );
        [[eosio::action]] void  cancelsello( name seller, asset quantity );
        [[eosio::action]] void cancelsoburn( name seller, asset quantity );
        [[eosio::action]] void  addbuyorder( name buyer, symbol_code sym, asset price, string memo );
        [[eosio::action]] void  selltoorder( symbol_code sym, uint64_t token_id, uint64_t order_id, string memo );
        [[eosio::action]] void cancelbobyid( name buyer, symbol_code sym, uint64_t order_id );
        [[eosio::action]] void   setmanager( symbol_code sym, uint64_t manager_token_id, vector<uint64_t> manager_token_list, vector<uint64_t> ratio_list, uint64_t others_ratio );
        [[eosio::action]] void     withdraw( name user, asset value, string memo );
        [[eosio::action]] void     setoffer( name provider, symbol_code sym, string uri, asset price, string memo );
        [[eosio::action]] void  acceptoffer( name manager, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void  rejectoffer( name manager, symbol_code sym, uint64_t offer_id, string memo );
        [[eosio::action]] void  removeoffer( name provider, symbol_code sym, uint64_t offer_id );
        [[eosio::action]] void   addpvcount( vector<pv_data_str> pv_data_list );
        // [[eosio::action]] void resetpvcount( uint64_t content_id );
        // [[eosio::action]] void  stopcontent( name manager, uint64_t content_id );
        // [[eosio::action]] void startcontent( name manager, uint64_t content_id );
        // [[eosio::action]] void  dropcontent( name manager, uint64_t content_id );
        // [[eosio::action]] void  resetpvrate();
        // [[eosio::action]] void deletepvrate( uint64_t timestamp );
        [[eosio::action]] void      moveeos( name from, name to, asset quantity, string memo );

        /**
         * Struct
        **/

        struct [[eosio::table]] account {
            asset balance;

            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        struct [[eosio::table]] global {
            uint64_t last_modified; // latest modified timestamp
            uint64_t pvcount; // global pv count
            float_t pvrate; // latest pv rate
            uint8_t active;

            uint64_t primary_key() const { return 0; }
        };

        struct [[eosio::table]] currency {
            asset supply;
            name issuer;
            uint64_t established;
            asset borderprice;
            uint64_t pvcount; // community pv count
            uint64_t numofmanager;
            float_t othersratio;
            uint8_t active;

            uint64_t primary_key() const { return supply.symbol.code().raw(); }
            uint64_t get_issuer() const { return issuer.value; }
            uint64_t get_newer() const { return established; }
            uint64_t get_borderprice() const { return static_cast<uint64_t>(borderprice.amount); }
            uint64_t get_pv_count() const { return pvcount; }
        };

        struct [[eosio::table]] token {
            uint64_t id;
            capi_public_key subkey;
            name owner;
            uint8_t active;

            uint64_t primary_key() const { return id; }
            uint64_t get_owner() const { return owner.value; }

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

    	    string get_unique_name( symbol_code sym ) const {
        		string unique_name = sym.to_string() + "#" + std::to_string(id);
        		return unique_name;
    	    }
        };

        struct [[eosio::table]] community_manager {
            uint64_t manager;
            float_t ratio;

            uint64_t primary_key() const { return manager; }
            float_t get_ratio() const { return ratio; }
        };

        struct [[eosio::table]] order {
            uint64_t id;
            asset price;
            name user;

            uint64_t primary_key() const { return id; }
            uint64_t get_price() const { return static_cast<uint64_t>(price.amount); }
            uint64_t get_owner() const { return user.value; }
        };

        struct [[eosio::table]] offer {
            uint64_t id;
            asset price;
            name provider;
            string uri;

            uint64_t primary_key() const { return id; }
            uint64_t get_price() const { return static_cast<uint64_t>(price.amount); }
            uint64_t get_provider() const { return provider.value; }
        };

        struct [[eosio::table]] content {
            uint64_t id;
            // symbol_code sym;
            asset price; // offer price
            name provider;
            string uri;
            uint64_t pvcount;
            uint64_t accepted; // acceptoffer 時の timestamp
            uint8_t active;

            uint64_t primary_key() const { return id; }
            // uint64_t get_symbol() const { return sym.raw(); }
            uint64_t get_price() const { return static_cast<uint64_t>(price.amount); }
            uint64_t get_provider() const { return provider.value; }
            uint64_t get_pv_count() const { return pvcount; }
            uint64_t get_accepted() const { return accepted; }
        };

        struct [[eosio::table]] pvrate {
            uint64_t timestamp;
            float_t rate;

            uint64_t primary_key() const { return timestamp; }
            float_t get_rate() const { return rate; }
        };

        /**
         * Multi Index
        **/

    	using account_index = eosio::multi_index< name("accounts"), account >;

        using deposit_index = eosio::multi_index< name("deposit"), account >;

        using total_deposit_index = eosio::multi_index< name("totaldeposit"), account >;

        using global_index = eosio::multi_index< name("global"), global >;

    	using currency_index = eosio::multi_index< name("currency"), currency,
            indexed_by< name("byissuer"), const_mem_fun<currency, uint64_t, &currency::get_issuer> >,
            indexed_by< name("bytime"), const_mem_fun<currency, uint64_t, &currency::get_newer> >,
            indexed_by< name("byprice"), const_mem_fun<currency, uint64_t, &currency::get_borderprice> >,
            indexed_by< name("bypv"), const_mem_fun<currency, uint64_t, &currency::get_pv_count> > >;

        using token_index = eosio::multi_index< name("token"), token,
            indexed_by< name("byowner"), const_mem_fun<token, uint64_t, &token::get_owner> > >;

        using community_manager_index = eosio::multi_index< name("manager"), community_manager >;

        using sell_order_index = eosio::multi_index< name("sellorder"), order,
            indexed_by< name("byprice"), const_mem_fun<order, uint64_t, &order::get_price> >,
            indexed_by< name("byowner"), const_mem_fun<order, uint64_t, &order::get_owner> > >;

        using buy_order_index = eosio::multi_index< name("buyorder"), order,
            indexed_by< name("byprice"), const_mem_fun<order, uint64_t, &order::get_price> >,
            indexed_by< name("byowner"), const_mem_fun<order, uint64_t, &order::get_owner> > >;

        using offer_index = eosio::multi_index< name("offer"), offer,
            indexed_by< name("byprice"), const_mem_fun<offer, uint64_t, &offer::get_price> >,
            indexed_by< name("byprovider"), const_mem_fun<offer, uint64_t, &offer::get_provider> > >;

        using contents_index = eosio::multi_index< name("contents"), content,
            indexed_by< name("byprice"), const_mem_fun<content, uint64_t, &content::get_price> >,
            indexed_by< name("byprovider"), const_mem_fun<content, uint64_t, &content::get_provider> >,
            indexed_by< name("bypvcount"), const_mem_fun<content, uint64_t, &content::get_pv_count> >,
            indexed_by< name("bytime"), const_mem_fun<content, uint64_t, &content::get_accepted> > >;

        // using pv_count_index = eosio::multi_index< name("contentspv"), pvcount >;

        // using cmnty_pv_count_index = eosio::multi_index< name("communitypv"), pvcount >;

        // using global_pv_count_index = eosio::multi_index< name("globalpv"), pvcount >;

        // using pv_rate_index = eosio::multi_index< name("pvrate"), pvrate >;

    private:
        /**
         *  Multi Index Table
        **/

        total_deposit_index total_deposit_table;
        global_index global_table;
        currency_index currency_table;
        // buy_order_index buy_order_table;
        // contents_index contents_table;
        // global_pv_count_index global_pv_count_table;
        // pv_rate_index pv_rate_table;

        /**
         * Private Function
        **/

        /// Execute Inline Action

        void _transfer_eos( name to, asset value, string memo );

        /// Modify Table

        void _create_token( name issuer, symbol_code sym );
        uint64_t _issue_token( name to, symbol_code sym );
        void _transfer_token( name from, name to, symbol_code sym, uint64_t id );
        void _burn_token( name owner, symbol_code sym, uint64_t token_id );
        void _set_subkey( name owner, symbol_code sym, uint64_t token_id, capi_public_key subkey );
        void _lock_token( symbol_code sym, uint64_t token_id );
        void _add_sell_order( name from, symbol_code sym, uint64_t token_id, asset price );
        void _sub_sell_order( name to, symbol_code sym, uint64_t token_id );
        uint64_t _add_buy_order( name from, symbol_code sym, asset price );
        void _sub_buy_order( name to, symbol_code sym, uint64_t order_id );
        void _update_pv_rate( symbol_code sym, uint64_t timestamp, asset new_offer_price );
        void _add_pv_count( symbol_code sym, uint64_t contents_id, uint64_t pv_count );
        void _add_contents( name ram_payer, symbol_code sym, asset price, name provider, string uri, uint64_t timestamp );
        void _add_balance( name owner, asset quantity, name ram_payer );
        void _sub_balance( name owner, asset quantity );
        void _add_supply( asset quantity );
        void _sub_supply( asset quantity );
        void _add_deposit( name owner, asset quantity );
        void _sub_deposit( name owner, asset quantity );
        void _increment_member( name user, symbol_code sym );
        void _decrement_member( name user, symbol_code sym );

        /// Assertion

        void assert_positive_quantity_of_nft( asset quantity );
        void assert_non_negative_eos( asset price );

        /// Search Table

        uint64_t find_own_token( name owner, symbol_code sym );
        uint64_t find_own_sell_order( name owner, symbol_code sym );
        // uint64_t find_pvdata_by_uri( symbol_code sym, string uri );
        // uint64_t find_offer_by_uri( symbol_code sym, string uri );
        // uint64_t get_global_pv_count();
        asset get_cmnty_offer_reward( symbol_code sym, uint64_t ago );
        asset get_border_price( symbol_code sym );
        // uint64_t get_cmnty_pv_count( symbol_code sym, uint64_t ago );


        /// Others

        vector<string> split_by_space( string memo );
};
