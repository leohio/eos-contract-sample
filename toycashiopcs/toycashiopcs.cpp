#include "toycashiopcs.hpp"
#include <eosiolib/action.hpp>

using namespace eosio;
using std::string;
using std::vector;

void pcs::create( name issuer, string sym ) {
	require_auth( _self );
    eosio_assert( issuer != _self, "do not create token by contract account" );

	// Check if issuer account exists
	eosio_assert( is_account( issuer ), "issuer account does not exist");

    // Valid symbol
    asset supply(0, symbol( symbol_code( sym.c_str() ), 0) );

    auto symbol = supply.symbol;
    eosio_assert( symbol.is_valid(), "invalid symbol name" );

    // Check if currency with symbol already exists
	auto symbol_name = symbol.code().raw();
    currency_index currency_table( _self, symbol_name );
    auto existing_currency = currency_table.find( symbol_name );
    eosio_assert( existing_currency == currency_table.end(), "token with symbol already exists" );

    // Create new currency
    currency_table.emplace( _self, [&]( auto& currency ) {
        currency.supply = supply;
        currency.issuer = issuer;
    });
}

void pcs::issue( name to, asset quantity, vector<public_key> subkeys, string tkn_name, string memo) {
	eosio_assert( is_account( to ), "to account does not exist");
    eosio_assert( to != _self, "do not issue token by contract account" );

    auto symbol = quantity.symbol;
    eosio_assert( symbol.is_valid(), "invalid symbol name" );
    eosio_assert( symbol.precision() == 0, "quantity must be a whole number" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
	eosio_assert( tkn_name.size() <= 32, "name has more than 32 bytes" );

    // Ensure currency has been created
    auto symbol_name = symbol.code().raw();
    currency_index currency_table( _self, symbol_name );
    auto existing_currency = currency_table.find( symbol_name );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );
    const auto& st = *existing_currency;

    // Ensure have issuer authorization and valid quantity
    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( symbol == st.supply.symbol, "symbol precision mismatch" );

    // Increase supply
	add_supply( quantity );

    // Check that number of tokens matches number of subkeys
    eosio_assert( quantity.amount == subkeys.size(), "mismatch between number of tokens and subkeys provided" );

    // Mint pcs
    for (auto const& subkey: subkeys) {
        mint( to, st.issuer, asset{1, symbol}, subkey, tkn_name);
    }

    // Add balance to account
    add_balance( to, quantity, st.issuer );
}

void pcs::transferid( name from, name to, id_type id, string memo ) {
    // Ensure authorized to send from account
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( from != _self, "do not transfer token from contract account" );
    eosio_assert( to != _self, "do not transfer token to contract account" );

    // Ensure 'to' account exists
    eosio_assert( is_account( to ), "to account does not exist");

	// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    // Ensure token ID exists
    auto send_token = tokens.find( id );
    eosio_assert( send_token != tokens.end(), "token with specified ID does not exist" );

	// Ensure owner owns token
    eosio_assert( send_token->owner == from, "sender does not own token with specified ID");

	const auto& st = *send_token;

	// Notify both recipients
    require_recipient( from );
    require_recipient( to );

    // Transfer NFT from sender to receiver
    tokens.modify( send_token, from, [&]( auto& token ) {
        token.owner = to;
    });

    // Change balance of both accounts
    sub_balance( from, st.value );
    add_balance( to, st.value, from );
}

void pcs::transfer( name from, name to, asset quantity, string memo ) {
    // Ensure authorized to send from account
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( from != _self, "do not transfer token from contract account" );
    eosio_assert( to != _self, "do not transfer token to contract account" );

    // Ensure 'to' account exists
    eosio_assert( is_account( to ), "to account does not exist");

    // Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	eosio_assert( quantity.amount == 1, "cannot transfer quantity, not equal to 1" );

	auto symbl = tokens.get_index<"bysymbol"_n>();

	auto it = symbl.lower_bound(quantity.symbol.code().raw());

	bool found = false;
	id_type id = 0;
	for(; it!=symbl.end(); ++it){

		if( it->value.symbol == quantity.symbol && it->owner == from) {
			id = it->id;
			found = true;
			break;
		}
	}

	eosio_assert(found, "token is not found or is not owned by account");

	// Notify both recipients
    require_recipient( from );
	require_recipient( to );

	SEND_INLINE_ACTION( *this, transferid, {from, "active"_n}, {from, to, id, memo} );
}

void pcs::mint( name owner, name ram_payer, asset value, public_key subkey, string tkn_name) {
    // set uri with creator paying for RAM
    tokens.emplace( ram_payer, [&]( auto& token ) {
        token.id = tokens.available_primary_key();
        token.subkey = subkey;
        token.owner = owner;
        token.value = value;
        token.tokenName = tkn_name;
        token.active = 1;
    });
}

void pcs::setrampayer(name payer, id_type id) {
	require_auth( payer );

	// Ensure token ID exists
	auto payer_token = tokens.find( id );
	eosio_assert( payer_token != tokens.end(), "token with specified ID does not exist" );

	// Ensure payer owns token
	eosio_assert( payer_token->owner == payer, "payer does not own token with specified ID");

	const auto& st = *payer_token;

	// Notify payer
	require_recipient( payer );

	// Set owner as a RAM payer
	tokens.modify(payer_token, payer, [&](auto& token){
		token.id = st.id;
		token.subkey = st.subkey;
		token.owner = st.owner;
		token.value = st.value;
		token.tokenName = st.tokenName;
        token.active = st.active;
	});

	sub_balance( payer, st.value );
	add_balance( payer, st.value, payer );
}

void pcs::refleshkey( name owner, id_type token_id, public_key subkey ) {
    require_auth( owner );
    eosio_assert( owner != _self, "do not reflesh key by contract account" );

    // Find token to burn
    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    tokens.modify(target_token, owner, [&](auto& token){
        token.subkey = subkey;
        token.active = 1; // lock 解除をついでに行う
    });

    require_recipient( owner );
}

void pcs::lock( name accuser, id_type token_id, string signature) {
    require_auth( accuser );
    eosio_assert( accuser != _self, "do not burn token by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );

    // 署名検証
    // target_token に記載された public key に対応する
    // private key を知っている人のみがロックの権限をもつ
    string data = "data which will sign by subkey";
    public_key pk = target_token->subkey;
    capi_checksum256 digest;
    sha256(&data[0], data.size(), &digest);
    // (const char *)&signature, sizeof(signature)
    // この関数がアウト
    assert_recover_key(&digest, &signature[0], signature.size(), (const char *)&pk, sizeof(pk));

    tokens.modify( target_token, accuser, [&](auto& token) {
        token.active = 0;
    });
}

void pcs::burn( name owner, id_type token_id ) {
    require_auth( owner );
    eosio_assert( owner != _self, "does not burn token by contract account" );

    // Find token to burn
    auto burn_token = tokens.find( token_id );
    eosio_assert( burn_token != tokens.end(), "token with id does not exist" );
    eosio_assert( burn_token->owner == owner, "token not owned by account" );

	asset burnt_supply = burn_token->value;

	// Remove token from tokens table
    tokens.erase( burn_token );

    // Lower balance from owner
    sub_balance( owner, burnt_supply );

    // Lower supply from currency
    sub_supply( burnt_supply );
}

void pcs::servebid(name owner, id_type token_id, asset price, string memo) {
    require_auth( owner );
    eosio_assert( owner != _self, "does not serve bid order by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    // Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    // price に指定した asset が EOS であることの確認
    eosio_assert( price.symbol == symbol( "EOS", 4 ), "allow only EOS as price" );

    bids.emplace( owner, [&]( auto& order ) {
        order.id = token_id;
        order.price = price;
        order.owner = owner;
        order.value = target_token->value;
    });

    tokens.modify( target_token, owner, [&]( auto& token ) {
        token.owner = _self; // 所有者をコントラクトアカウントに
    });

    // Lower balance from owner
    sub_balance( owner, target_token->value );
}

void pcs::cancelbid(name owner, id_type token_id) {
    require_auth( owner );
    eosio_assert( owner != _self, "does not cancel bid order by contract account" );

    auto target_token = tokens.find( token_id );
    auto bid_order = bids.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( bid_order != bids.end(), "order does not exist" );
    eosio_assert( bid_order->owner == owner, "order does not serve by account" );

    bids.erase( bid_order );

    tokens.modify( target_token, owner, [&]( auto& token ) {
        token.owner = owner;
    });

    add_balance( owner, bid_order->value , owner );
}

void pcs::receive() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data< transfer_args >();
    name from_account = transfer_data.from;
    name to_account = transfer_data.to;

    /// Because "transfereos" action is called when this contract account
    /// is the sender or receiver of "eosio.token::transfer",
    /// so `to_account == get_self()` is needed.
    /// This contract account do not have balance record,
    /// so `from_account != get_self()` is needed.
    if ( to_account == get_self() && from_account != get_self() ) {
        asset quantity = transfer_data.quantity;

        auto balance_data = bt.find( from_account.value );

        if( balance_data == bt.end() ) {
            bt.emplace( get_self(), [&]( auto& data ) {
                data.username = from_account;
                data.quantity = quantity;
            });
        } else {
            asset deposit = balance_data->quantity;

            eosio_assert(
                deposit.symbol == quantity.symbol,
                "different symbol or precision mismatch from your token deposited"
            );

            eosio_assert(
                deposit.amount + quantity.amount > deposit.amount,
                "since occurred overflow, revert the state"
            );

            /// modify the record of balance_data
            /// ram payer is this contract account
            bt.modify( balance_data, get_self(), [&]( auto& data ) {
                data.quantity = deposit + quantity;
            });
        }
    }/* else if ( from_account == get_self() && to_account != get_self() ) {
        asset quantity = transfer_data.quantity;
        symbol token_symbol = quantity.symbol;
        asset owned_token = ???;
        asset deposit_sum = ???;

        eosio_assert(
            deposit_sum > owned_token - quantity,
            "please leave deposit"
        );
    }*/
}

void pcs::buy(name buyer, id_type token_id, asset price, string memo) {
    require_auth( buyer );
    eosio_assert( buyer != _self, "does not buy token by contract account" );

    auto target_token = tokens.find( token_id );
    auto bid_order = bids.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( bid_order != bids.end(), "token with id does not exist" );

    // Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    // 支払い
    withdraw( bid_order->owner, price, "executed as the inline action pcs::buy of " + get_self().to_string() );

    bids.erase( bid_order );

    tokens.modify( target_token, buyer, [&]( auto& token ) {
        token.owner = buyer;
        token.active = 0; // lock until reflesh key for safety
    });

    add_balance( buyer, bid_order->value , buyer );
}

void pcs::seturi(name owner, string sym, string uri) {
    require_auth( owner );
    symbol token_symbol = symbol( sym.c_str(), 0 );

    // symbol として正しいか確認
    eosio_assert( token_symbol.is_valid(), "invalid symbol name" );

    // Check uri size and print
    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );

    pvcount.emplace( owner, [&]( auto& data ) {
        data.id = pvcount.available_primary_key();
        data.uri = uri;
        data.symbol = sym;
        data.count = 0;
    });
}

void pcs::setpvdata(name claimer, string sym, id_type uri_id, uint64_t count) {
    // コントラクトアカウントのみが呼び出せる
    require_auth( _self );

    symbol token_symbol = symbol( sym.c_str(), 0);

    // symbol として正しいか確認
    eosio_assert( token_symbol.is_valid(), "invalid symbol name" );

    // すでに指定した uri に関する pv のデータがあることを確認
    auto pv_data = pvcount.find( uri_id );
    eosio_assert( pv_data != pvcount.end() ,"this uri is not exist in the pv count table" );

    // overflow 対策
    eosio_assert( pv_data->count + count > pv_data->count, "pv count overflow! so revert state." );

    // uri と sym の組み合わせが一致するか確認
    eosio_assert( pv_data->symbol == sym, "this uri is already used by an another community." );

    pvcount.modify( pv_data, claimer, [&]( auto& data ) {
        data.count = data.count + count;
    });
}

void pcs::removepvdata( id_type uri_id ) {
    // コントラクトアカウントのみが呼び出せる
    require_auth( _self );

    auto pv_data = pvcount.find( uri_id );

    if ( pv_data != pvcount.end() ) {
        pvcount.erase( pv_data );
    }
}

void pcs::sub_balance( name owner, asset value ) {
	account_index from_acnts( _self, owner.value );
    const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
    eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

    if ( from.balance.amount == value.amount ) {
        from_acnts.erase( from );
    } else {
       from_acnts.modify( from, owner, [&]( auto& a ) {
           a.balance -= value;
       });
    }
}

void pcs::add_balance( name owner, asset value, name ram_payer ) {
	account_index to_accounts( _self, owner.value );
    auto to = to_accounts.find( value.symbol.code().raw() );

    if ( to == to_accounts.end() ) {
        to_accounts.emplace( ram_payer, [&]( auto& a ) {
            a.balance = value;
        });
    } else {
        to_accounts.modify( to, _self, [&]( auto& a ) {
            a.balance += value;
        });
    }
}

void pcs::sub_supply( asset quantity ) {
	auto symbol_name = quantity.symbol.code().raw();
    currency_index currency_table( _self, symbol_name );
    auto current_currency = currency_table.find( symbol_name );

    currency_table.modify( current_currency, _self, [&]( auto& currency ) {
        currency.supply -= quantity;
    });
}

void pcs::add_supply( asset quantity ) {
    auto symbol_name = quantity.symbol.code().raw();
    currency_index currency_table( _self, symbol_name );
    auto current_currency = currency_table.find( symbol_name );

    currency_table.modify( current_currency, name(0), [&]( auto& currency ) {
        currency.supply += quantity;
    });
}

void pcs::withdraw( name user, asset quantity, string memo ) {
    auto balance_data = bt.find( user.value );

    eosio_assert(
        balance_data != bt.end(),
        "do not exist your data"
    );

    asset deposit = balance_data->quantity;

    eosio_assert(
        deposit.symbol == quantity.symbol,
        "different symbol or precision mismatch from your token deposited"
    );

    eosio_assert(
        deposit.amount >= quantity.amount,
        "exceed the withdrawable amount"
    );

    if ( deposit.amount == quantity.amount ) {
        bt.erase( balance_data );
    } else if ( deposit.amount > quantity.amount ) {
        /// modify the record of balance_data
        /// ram payer is this contract account
        bt.modify( balance_data, get_self(), [&]( auto& data ) {
            data.quantity = deposit - quantity;
        });
    }

    action(
        permission_level{ get_self(), name("active") }, // このアカウントの権限を用いて
        name("eosio.token"), // このコントラクト内にある
        name("transfer"), // このメソッドに
        std::make_tuple( get_self(), user, quantity, memo ) // 引数をタプルで渡して
    ).send(); // アクションを実行する
}

/// dispatcher
extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ){
        if ( code == name("eosio.token").value ) {
            if ( action == name("transfer").value ) {
                /// When one call eosio.token::transfer action,
                /// receiveeos::transfereos action is also executed.
                execute_action( name(receiver), name(code), &pcs::receive );
            }
        } else if ( code == receiver ) {
            switch( action ) {
               EOSIO_DISPATCH_HELPER( pcs, (create)(issue)(transfer)(transferid)(setrampayer)(refleshkey)(lock)(burn)(servebid)(cancelbid)(buy)(seturi)(setpvdata)(removepvdata) );
            }
        }
    }
}
