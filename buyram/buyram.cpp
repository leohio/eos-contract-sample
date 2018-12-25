#include <eosiolib/eosio.hpp> // eosio
#include <eosiolib/action.hpp> // inline action
#include "exchange_state.cpp" // rammarket, convert

using namespace eosio;

class helpaccount : public contract {
    public:
        /**
         * Constructor
        **/

        helpaccount( name receiver, name code, datastream<const char*> ds ):
    	    contract::contract( receiver, code, ds ){}

        /**
         * Public Function
        **/

        void buyram( name to, uint32_t bytes );
};

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

/// dispatcher
extern "C" {
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
    if ( code == receiver ) {
      switch( action ) {
        EOSIO_DISPATCH_HELPER( helpaccount, (buyram) );
      }
    }
  }
}
