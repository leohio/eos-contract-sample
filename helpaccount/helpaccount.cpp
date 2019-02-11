/**
 * @environment
 *     eosio.cdt v1.4.1
 * @author mokemokecore
**/

#include <eosiolib/print.h> /* print */
#include <eosiolib/eosio.hpp> /* eosio */
#include <eosiolib/action.hpp> /* inline action */
// #include <eosio/chain/authority.hpp> /* authority struct */
#include "exchange_state.cpp" /* rammarket struct, convert */

using namespace eosio;
using namespace std;
typedef uint16_t weight_type;
// typedef array<char, 33> public_key;

class [[eosio::contract]] helpaccount : public eosio::contract {
    public:
        /**
         * Constructor
        **/

        helpaccount( name receiver, name code, datastream<const char*> ds ):
    	    contract::contract( receiver, code, ds ){}

        /**
         * Public Function
        **/

        [[eosio::action]] void message( string memo );
        [[eosio::action]] void buyram( name to, uint32_t bytes );
        [[eosio::action]] void updateauth( name to, name to_permission, name parent_permission, capi_public_key key, name target, name target_permission );

        /**
         * Struct
        **/

        struct key_weight {
            capi_public_key key;
            weight_type weight;

            // explicit serialization macro is not necessary, used here only to improve compilation time
            EOSLIB_SERIALIZE( key_weight, (key)(weight) )
        };

        struct wait_weight {
            uint32_t wait_sec;
            weight_type weight;

            // explicit serialization macro is not necessary, used here only to improve compilation time
            EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
        };

        struct permission_level_weight {
            permission_level permission;
            weight_type weight;

            // explicit serialization macro is not necessary, used here only to improve compilation time
            EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
        };

        struct authority {
            uint32_t threshold = 0;
            vector<key_weight> keys;
            vector<permission_level_weight> accounts;
            vector<wait_weight> waits;

            // explicit serialization macro is not necessary, used here only to improve compilation time
            EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
        };
};

void helpaccount::message( string memo ) {
    print( "receive message" );
}

void helpaccount::buyram( name to, uint32_t bytes ) {
    eosio_assert( is_account( to ), "'to' account does not exist");

    // cleos get table eosio eosio rammarket
    eosiosystem::rammarket market( name("eosio"), name("eosio").value );

    // find RAMCORE market?
    auto itr = market.find( symbol( "RAMCORE", 4 ).code().raw() );

    // do not pass this assert
    eosio_assert( itr != market.end(), "RAMCORE market not found" );

    // ???
    auto tmp = *itr;
    asset rambytes = asset( bytes, symbol( "RAM", 0 ) );
    const auto amount = tmp.convert( rambytes, symbol( "EOS", 4 ) );

    action(
        permission_level{ get_self(), name("active") }, // このアカウントの権限を用いて
        name("eosio"), // このコントラクト内にある
        name("buyram"), // このメソッドに
        std::make_tuple( get_self(), to, amount ) // 引数をタプルで渡して
    ).send(); // アクションを実行する
}

/*
cleos push action mokemokecore updateauth \
'["mokemokecore", "active", "owner", "EOS7hnP2sZGr2DAwfL5fSZ8btYF99gjmeEBbKftFeWWhG4jSK3zRo", "mokemokecore", "eosio.code"]' \
-p mokemokecore@active
*/

void helpaccount::updateauth(
    name to,
    name to_permission,
    name parent_permission,
    capi_public_key key,
    name target,
    name target_permission
) {
    // name to = "mokemokecore";
    // name to_permission = "active";
    // name parent_permission = "owner";

    uint32_t threshold = 1;

    vector<key_weight> keys;
    // capi_public_key_type key = "EOS7hnP2sZGr2DAwfL5fSZ8btYF99gjmeEBbKftFeWWhG4jSK3zRo";
    weight_type kweight = 1;
    keys.push_back( key_weight{ key, kweight } );

    vector<permission_level_weight> accounts;
    // name target = "mokemokecore";
    // name target_permission = "eosio.code";
    weight_type pweight = 1;
    accounts.push_back( permission_level_weight{ permission_level{ target, target_permission }, pweight } );

    vector<wait_weight> waits;
    // uint32_t wait_sec = 0;
    // weight_type weight = 1;
    // waits.push( wait_weight{ wait_sec, weight } );

    authority auth = authority{ threshold, keys, accounts, waits };

    action(
        permission_level{ to, to_permission }, // このアカウントの権限を用いて
        name("eosio"), // このコントラクト内にある
        name("updateauth"), // このメソッドに
        std::make_tuple( to, to_permission, parent_permission, auth ) // 引数をタプルで渡して
    ).send(); // アクションを実行する
}

/// dispatcher
extern "C" {
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
    if ( code == receiver ) {
      switch( action ) {
        EOSIO_DISPATCH_HELPER( helpaccount, (message)(buyram)(updateauth) );
      }
    }
  }
}
