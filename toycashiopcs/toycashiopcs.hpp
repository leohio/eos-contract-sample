#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/public_key.hpp>
#include <string>
#include <vector>

using namespace eosio;
using std::string;
using std::vector;
typedef uint128_t uuid;
typedef uint64_t id_type;

class pcs : public contract {
    public:
        using contract::contract;

        /**
         * Constructor
        **/

        pcs( name receiver, name code, datastream<const char*> ds ):
    	    contract(receiver, code, ds),
            tokens(receiver, receiver.value),
            bt(receiver, receiver.value),
            bids(receiver, receiver.value),
            pvcount(receiver, receiver.value) {}

        /**
         * Public Function
        **/

        void create(name issuer, string symbol);
        void issue(name to, asset quantity, vector<public_key> subkeys, string name, string memo);
        void transferid(name from, name to, id_type id, string memo);
	    void transfer(name from, name to, asset quantity, string memo);
        void burn(name owner, id_type token_id);
	    void setrampayer(name payer, id_type id);
        void refleshkey(name owner, id_type token_id, public_key subkey);
        void lock(name accuser, id_type token_id, string signature);
        void servebid(name owner, id_type token_id, asset price, string memo);
        void cancelbid(name owner, id_type token_id);
        void buy(name buyer, id_type token_id, asset price, string memo);
        void seturi(name owner, string sym, string uri);
        void setpvid(name claimer, string sym, id_type uri_id, uint64_t count);
        void setpvdata(name claimer, string sym, string uri, uint64_t count);
        void removepvid(name claimer, string sym, id_type uri_id);
        void removepvdata(name claimer, string sym, string uri);
        void receive();

        /**
         * Struct
        **/

        struct account {
            asset balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        struct stats {
            asset supply;
            name issuer;
            uint64_t primary_key() const { return supply.symbol.code().raw(); }
            uint64_t get_issuer() const { return issuer.value; }
        };

        struct token {
            id_type id; // Unique 64 bit identifier,
            public_key subkey; // Ethereum address
            name owner; // token owner
            asset value; // token value e.g. "1 PCS"
	        string tokenName; // token name
            uint8_t active; // whether this token is locked or not

            uint64_t primary_key() const { return id; }
            uint64_t get_owner() const { return owner.value; }
            public_key get_subkey() const { return subkey; }
            asset get_value() const { return value; }
	        uint64_t get_symbol() const { return value.symbol.code().raw(); }
	        string get_name() const { return tokenName; }
            uint8_t get_active() const { return active; }

    	    // generated token global uuid based on token id and
    	    // contract name, passed in the argument
    	    uuid get_global_id(name self) const {
                /// static_cast は型変換を明示
                /// uint64 -> uint128 のキャスト
        		uint128_t self_128 = static_cast<uint128_t>(self.value);
                /// uint64 -> uint128 のキャスト
        		uint128_t id_128 = static_cast<uint128_t>(id);
                /// << は左ビットシフト, | はビット和
        		uint128_t res = (self_128 << 64) | (id_128);
        		return res;
    	    }

    	    string get_unique_name() const {
                /// std::tostring は整数（浮動小数点数）から文字列へのキャスト
        		string unique_name = tokenName + "#" + std::to_string(id);
        		return unique_name;
    	    }
        };

        struct order {
            id_type id;
            asset price; // e.g. "1 EOS"
            name owner;
            asset value; // e.g. "1 PCS"

            uint64_t primary_key() const { return id; }
            asset get_price() const { return price; }
            uint64_t get_owner() const { return owner.value; }
            asset get_value() const { return value; }
            uint64_t get_symbol() const { return value.symbol.code().raw(); }
        };

        struct pvdata {
            id_type id;
            string uri;
            string symbol;
            uint64_t count;

            uint64_t primary_key() const { return id; }
            string get_uri() const { return uri; }
            string get_symbol() const { return symbol; }
            uint64_t get_count() const { return count; }
        };

        struct balance {
            name username;
            asset quantity;

            uint64_t primary_key() const { return username.value; }
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

    	using account_index = eosio::multi_index< "accounts"_n, account >;

        using balance_table = eosio::multi_index< "balance"_n, balance >;

    	using currency_index = eosio::multi_index< "stat"_n, stats,
    	    indexed_by< "byissuer"_n, const_mem_fun< stats, uint64_t, &stats::get_issuer> > >;

    	using token_index = eosio::multi_index< "token"_n, token,
    	    indexed_by< "byowner"_n, const_mem_fun<token, uint64_t, &token::primary_key> >,
            indexed_by< "bysymbol"_n, const_mem_fun<token, uint64_t, &token::get_symbol> > >;

        // 売り板
        using bid_board = eosio::multi_index< "bid"_n, order,
            indexed_by< "byowner"_n, const_mem_fun<order, uint64_t, &order::get_owner> >,
            indexed_by< "bysymbol"_n, const_mem_fun<order, uint64_t, &order::get_symbol> > >;

        using pv_count_data = eosio::multi_index< "pvcount"_n, pvdata,
            indexed_by< "byuriid"_n, const_mem_fun<pvdata, uint64_t, &pvdata::primary_key> >,
            indexed_by< "bycount"_n, const_mem_fun<pvdata, uint64_t, &pvdata::get_count> > >;

    private:
	    token_index tokens;
        balance_table bt;
        bid_board bids;
        pv_count_data pvcount;

        /**
         * Private Function
        **/

        void mint(name owner, name ram_payer, asset value, public_key subkey, string name);
        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
        void sub_supply(asset quantity);
        void add_supply(asset quantity);
        void withdraw(name user, asset quantity, string memo);
};
