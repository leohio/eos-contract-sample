#include "pcs.hpp"

using namespace eosio;
using std::string;
using std::vector;

void pcs::create( name issuer, symbol_code sym ) {
    require_auth( issuer );
    eosio_assert( issuer != get_self(), "do not create token by contract account" );

    /// Check if issuer account exists
    eosio_assert( is_account( issuer ), "issuer account does not exist");

    eosio_assert( sym != symbol_code("EOS"), "EOS is invalid token symbol");

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    currency_index currency_table( get_self(), sym.raw() );
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data == currency_table.end(), "token with symbol already exists" );

    /// Create new currency
    currency_table.emplace( get_self(), [&]( auto& data ) {
        data.supply = asset{ 0, symbol( sym, 0 ) };
        data.issuer = issuer;
    });
}

void pcs::destroy( symbol_code sym ) {
    /// only destroy token by contract account
    require_auth( get_self() );

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    currency_index currency_table( get_self(), sym.raw() );
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exists" );
    eosio_assert( currency_data->supply == asset{ 0, symbol(sym, 0) }, "who has this token exists" );

    /// Create new currency
    currency_table.erase( currency_data );
}

/// subkey を登録しないでトークン発行
void pcs::mint_token( name user, symbol_code sym, name ram_payer ) {
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_table.available_primary_key();
        data.owner = user;
        data.sym = sym;
        data.active = 0;
    });
}

void pcs::issue( name user, asset quantity, string memo ) {
    eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token by contract account" );

    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = currency_data->issuer;
    require_auth( issuer );

    /// Ensure valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == currency_data->supply.symbol, "symbol code or precision mismatch" );

    /// Increase supply and add balance
    add_supply( quantity );
    add_balance( user, quantity, issuer );

    int64_t i = 0;
    for (; i != quantity.amount; ++i ) {
        pcs::mint_token( user, sym, issuer );
    }
}

void pcs::mint_unlock_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer ) {
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_table.available_primary_key();
        data.subkey = subkey;
        data.owner = user;
        data.sym = sym;
        data.active = 1;
    });
}

void pcs::issueunlock( name user, asset quantity, vector<capi_public_key> subkeys, string memo ) {
	eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token by contract account" );

    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol code" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = currency_data->issuer;
    require_auth( issuer );

    /// valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == currency_data->supply.symbol, "symbol code or precision mismatch" );

    /// Check that number of tokens matches subkey size
    eosio_assert( quantity.amount == subkeys.size(), "mismatch between number of tokens and subkeys provided" );

    /// Increase supply and add balance
    add_supply( quantity );
    add_balance( user, quantity, issuer );

    for ( auto const& subkey: subkeys ) {
        pcs::mint_unlock_token( user, sym, subkey, issuer );
    }
}

void pcs::transferid( name from, name to, uint64_t id, string memo ) {
    /// Ensure authorized to send from account
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( from != get_self(), "do not transfer token from contract account" );
    eosio_assert( to != get_self(), "do not transfer token to contract account" );

    /// Ensure 'to' account exists
    eosio_assert( is_account( to ), "to account does not exist");

	/// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure token ID exists
    auto send_token = token_table.find( id );
    eosio_assert( send_token != token_table.end(), "token with specified ID does not exist" );

	/// Ensure owner owns token
    eosio_assert( send_token->owner == from, "sender does not own token with specified ID");

	/// Notify both recipients
    require_recipient( from );
    require_recipient( to );

    /// Transfer NFT from sender to receiver
    token_table.modify( send_token, from, [&]( auto& data ) {
        data.owner = to;
    });

    /// Change balance of both accounts
    decrease_balance( from, send_token->sym );
    add_balance( to, asset{ 1, symbol( send_token->sym, 0 ) } , from );
}

void pcs::transfer( name from, name to, symbol_code sym, string memo ) {
    /// Ensure authorized to send from account
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( from != get_self(), "do not transfer token from contract account" );
    eosio_assert( to != get_self(), "do not transfer token to contract account" );

    /// Ensure 'to' account exists
    eosio_assert( is_account( to ), "to account does not exist");

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// 所持トークンを1つ探す
    uint64_t id = pcs::find_own_token( from, sym );

	/// Notify both recipients
    require_recipient( from );
	require_recipient( to );

    action(
        permission_level{ from, name("active") },
        get_self(),
        name("transferid"),
        std::make_tuple( from, to, id, memo )
    ).send();
}

void pcs::burn( name owner, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    /// Find token to burn
    auto burn_token = token_table.find( token_id );
    eosio_assert( burn_token != token_table.end(), "token with id does not exist" );
    eosio_assert( burn_token->owner == owner, "token not owned by account" );

	symbol_code sym = burn_token->sym;

	/// Remove token from token_table
    token_table.erase( burn_token );

    /// Lower balance from owner
    decrease_balance( owner, sym );

    /// Lower supply from currency
    decrease_supply( sym );
}

/// subkey を変更し lock 解除を行う
void pcs::refleshkey( name owner, uint64_t token_id, capi_public_key subkey ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "do not reflesh key by contract account" );

    /// Find token to burn
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    symbol_code sym = target_token->sym;

    /// Notify payer
	require_recipient( owner );

    token_table.modify( target_token, owner, [&](auto& data) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = owner;
        data.sym = sym;
        data.active = 1;
    });

    decrease_balance( owner, sym );
	add_balance( owner, asset{ 1, symbol( sym, 0 ) } , owner );
}

// void pcs::lock( name claimer, uint64_t token_id, string data, capi_signature sig ) {
//     require_auth( claimer );
//     // eosio_assert( claimer != get_self(), "do not lock token by contract account" );
//
//     auto target_token = token_table.find( token_id );
//     eosio_assert( target_token != token_table.end(), "token with id does not exist" );
//
//     /// 署名検証
//     /// target_token に記載された public key に対応する
//     /// private key を知っている人のみがロックの権限をもつ
//     capi_public_key pk = target_token->subkey;
//     capi_checksum256 digest;
//     sha256(&data[0], data.size(), &digest);
//
//     assert_recover_key(&digest, (const char *)&sig, sizeof(sig), (const char *)&pk, sizeof(pk));
//
//     token_table.modify( target_token, get_self(), [&](auto& data) {
//         data.active = 0;
//     });
// }

void pcs::servebid( name owner, uint64_t token_id, asset price, string memo ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not serve bid order by contract account" );

    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// price に指定した asset が EOS であることの確認
    eosio_assert( price.symbol == symbol( "EOS", 4 ), "allow only EOS as price" );

    sell_order_table.emplace( owner, [&]( auto& data ) {
        data.id = token_id;
        data.price = price;
        data.owner = owner;
        data.sym = target_token->sym;
    });

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = get_self(); /// 所有者をコントラクトアカウントに
    });

    /// Lower balance from owner
    decrease_balance( owner, target_token->sym );
}

void pcs::transfer_eos( name to, asset value, string memo ) {
    eosio_assert( is_account( to ), "to account does not exist" );
    eosio_assert( value.symbol == symbol( "EOS", 4 ), "symbol of value is not EOS" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    action(
        permission_level{ get_self(), name("active") },
        name("eosio.token"),
        name("transfer"),
        std::make_tuple( get_self(), to, value, memo )
    ).send();
}

void pcs::buy( name user, uint64_t token_id, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );

    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "token with id does not exist" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    sell_order_table.erase( sell_order );

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = user;
        data.active = 0; /// lock until reflesh key for safety
    });

    /// トークンの残高を増やす
    add_balance( user, asset{ 1, symbol( sell_order->sym, 0 ) }, get_self() );

    /// 売り手に送金
    sub_deposit( user, sell_order->price );
    transfer_eos( sell_order->owner, sell_order->price, "executed as the inline action in pcs::buy of " + get_self().to_string() );
}

void pcs::buyandunlock( name user, uint64_t token_id, capi_public_key subkey, string memo ) {
    buy( user, token_id, memo );
    refleshkey( user, token_id, subkey );
}

/// あらかじめ user にこのコントラクトの eosio.code permission をつけておかないと実行できない。
void pcs::sendandbuy( name user, uint64_t token_id, capi_public_key subkey, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "token with id does not exist" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto deposit_data = deposit_table.find( user.value );
    asset deposit;
    if ( deposit_data == deposit_table.end() ) {
        deposit = asset{ 0, symbol("EOS", 4) };
    } else {
        deposit = deposit_data->quantity;
    }

    asset price = sell_order->price;
    eosio_assert( price.symbol == symbol("EOS", 4), "symbol of price must be EOS" );
    eosio_assert( price.amount > 0, "price must be positive" );

    if ( deposit.amount < price.amount ) {
        /// 不足分をデポジット
        asset quantity = price - deposit;
        string transfer_memo = "send EOS to buy token";
        action(
            permission_level{ user, name("active") },
            name("eosio.token"),
            name("transfer"),
            std::make_tuple( user, get_self(), quantity, transfer_memo )
        ).send();
    }

    string buy_memo = "call pcs::buy as inline action in " + get_self().to_string();
    action(
        permission_level{ user, name("active") },
        get_self(),
        name("buyandunlock"),
        std::make_tuple( user, token_id, subkey, buy_memo )
    ).send();
}

void pcs::cancelbid( name owner, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not cancel bid order by contract account" );

    auto target_token = token_table.find( token_id );
    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );
    eosio_assert( sell_order->owner == owner, "order does not serve by account" );

    sell_order_table.erase( sell_order );

    token_table.modify( target_token, owner, [&]( auto& data ) {
        data.owner = owner;
    });

    add_balance( owner, asset{ 1, symbol( sell_order->sym, 0 ) }, owner );
}

void pcs::receive() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data< transfer_args >();
    name from = transfer_data.from;
    name to = transfer_data.to;
    asset quantity = transfer_data.quantity;
    string message = transfer_data.memo;

    /// Because "receive" action is called when this contract account
    /// is the sender or receiver of "eosio.token::transfer",
    /// so `to == get_self()` is needed.
    /// This contract account do not have balance record,
    /// so `from != get_self()` is needed.
    if ( to == get_self() && from != get_self() ) {
        pcs::add_deposit( from, quantity, get_self() );

        vector<string> sbc = split_by_comma( message );
        string contract_name = get_self().to_string();

        if ( sbc[0] == contract_name && sbc[1] == "buy" && sbc.size() == 3 ) {
            uint64_t token_id = static_cast<uint64_t>( std::stoull(sbc[2]) );

            auto sell_order = sell_order_table.find( token_id );
            eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );

            string buy_memo = "buy token in pcs::receive of " + contract_name;

            /// unlock までやりたいならば、 string -> capi_public_key の変換関数が必要
            buy( from, token_id, buy_memo );
        }
    } /*else if ( from == get_self() && to != get_self() ) {
        /// TODO: このコントラクトの EOS 残高を check
    }*/
}

void pcs::withdraw( name user, asset quantity, string memo ) {
    require_auth( user );
    pcs::sub_deposit( user, quantity );
    pcs::transfer_eos( user, quantity, memo );
}

void pcs::add_balance( name owner, asset quantity, name ram_payer ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

	account_index account_table( get_self(), owner.value );
    auto account_data = account_table.find( quantity.symbol.code().raw() );

    if ( account_data == account_table.end() ) {
        account_table.emplace( ram_payer, [&]( auto& data ) {
            data.balance = quantity;
        });
    } else {
        account_table.modify( account_data, get_self(), [&]( auto& data ) {
            data.balance += quantity;
        });
    }
}

void pcs::decrease_balance( name owner, symbol_code sym ) {
	account_index account_table( get_self(), owner.value );
    auto account_data = account_table.find( sym.raw() );
    eosio_assert( account_data != account_table.end(), "no balance object found" );

    asset balance = account_data->balance;

    eosio_assert( balance.amount >= 1, "overdrawn balance" );

    if ( balance.amount == 1 ) {
        account_table.erase( account_data );
    } else {
        asset burnt_quantity = asset{ 1, symbol( sym, 0 ) };

        account_table.modify( account_data, owner, [&]( auto& data ) {
            data.balance -= burnt_quantity;
        });
    }
}

void pcs::add_supply( asset quantity ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

    symbol_code sym = quantity.symbol.code();

    currency_index currency_table( get_self(), sym.raw() );
    auto current_currency = currency_table.find( sym.raw() );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply += quantity;
    });
}

void pcs::decrease_supply( symbol_code sym ) {
    currency_index currency_table( get_self(), sym.raw() );
    auto current_currency = currency_table.find( sym.raw() );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply -= asset{ 1, symbol( sym, 0 ) };
    });
}

void pcs::add_deposit( name owner, asset quantity, name ram_payer ) {
    eosio_assert( is_account( owner ), "owner account does not exist" );
    eosio_assert( is_account( ram_payer ), "ram_payer account does not exist" );
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "must EOS token" );

    auto deposit_data = deposit_table.find( owner.value );

    if( deposit_data == deposit_table.end() ) {
        deposit_table.emplace( ram_payer, [&]( auto& data ) {
            data.owner = owner;
            data.quantity = quantity;
        });
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.quantity += quantity;
        });
    }
}

void pcs::sub_deposit( name owner, asset quantity ) {
    eosio_assert( is_account( owner ), "owner account does not exist");
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "must EOS token" );

    auto deposit_data = deposit_table.find( owner.value );
    eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

    asset deposit = deposit_data->quantity;
    eosio_assert( deposit.amount >= quantity.amount, "exceed the withdrawable amount" );

    if ( deposit.amount == quantity.amount ) {
        deposit_table.erase( deposit_data );
    } else if ( deposit.amount > quantity.amount ) {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.quantity -= quantity;
        });
    }
}

uint64_t pcs::find_own_token( name owner, symbol_code sym ) {
    auto token_table2 = token_table.get_index<name("bysymbol")>();

    auto it = token_table2.lower_bound( sym.raw() );

    bool found = false;
    uint64_t token_id = 0;
    for (; it != token_table2.end(); ++it ) {
        if ( it->sym == sym && it->owner == owner ) {
            token_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert( found, "token is not found or is not owned by account" );

    return token_id;
}

vector<string> pcs::split_by_comma( string str ) {
    eosio_assert( str.size() <= 256, "too long str" );
    const char* c = str.c_str();

    uint8_t i = 0;
    vector<char> char_list;
    string segment;
    vector<string> split_list;

    for (; c[i]; ++i ) {
        eosio_assert( ' ' <= c[i] && c[i] <= '~', "str only contains between 0x20 and 0x7e" );

        if ( c[i] == ',' ) {
            split_list.push_back( string( char_list.begin(), char_list.end() ) );
            char_list.clear();
        } else {
            char_list.push_back( c[i] );
        }
    }

    split_list.push_back( string( char_list.begin(), char_list.end() ) );

    return split_list;
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
               EOSIO_DISPATCH_HELPER( pcs,
                   (create)(destroy) /// currency
                   (issue)(transferid)(transfer)(burn) /// token
                   (refleshkey) /// token
                   (servebid)(buy)(buyandunlock)(sendandbuy)(cancelbid) /// bid
                   (withdraw) /// eos
               );
            }
        }
    }
}
