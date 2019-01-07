#include "pcs.hpp"

using namespace eosio;
using std::string;
using std::vector;

void pcs::create( name issuer, symbol_code sym ) {
	require_auth( get_self() ); // only create token by contract account
    eosio_assert( issuer != get_self(), "do not create token by contract account" );

	/// Check if issuer account exists
	eosio_assert( is_account( issuer ), "issuer account does not exist");

    eosio_assert( sym != symbol_code("EOS"), "EOS is invalid token symbol");

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency == currency_table.end(), "token with symbol already exists" );

    /// Create new currency
    currency_table.emplace( get_self(), [&]( auto& data ) {
        data.supply = asset{ 0, symbol( sym, 0 ) };
        data.issuer = issuer;
    });
}

void pcs::destroy( symbol_code sym ) {
    require_auth( get_self() ); // only destroy token by contract account

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exists" );

    /// Create new currency
    currency_table.erase( existing_currency );
}

/// subkey を登録しないでトークン発行
void pcs::mint_token( name user, symbol_code sym, name ram_payer ) {
    tokens.emplace( ram_payer, [&]( auto& data ) {
        data.id = tokens.available_primary_key();
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

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = existing_currency->issuer;
    require_auth( issuer );

    /// Ensure valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == existing_currency->supply.symbol, "symbol code or precision mismatch" );

    /// Increase supply and add balance
    add_supply( quantity );
    add_balance( user, quantity, issuer );

    int64_t i = 0;
    for (; i != quantity.amount; ++i ) {
        pcs::mint_token( user, sym, issuer );
    }
}

void pcs::mint_unlock_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer ) {
    tokens.emplace( ram_payer, [&]( auto& data ) {
        data.id = tokens.available_primary_key();
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

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = existing_currency->issuer;
    require_auth( issuer );

    /// valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == existing_currency->supply.symbol, "symbol code or precision mismatch" );

    // Check that number of tokens matches subkey size
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

	/// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure token ID exists
    auto send_token = tokens.find( id );
    eosio_assert( send_token != tokens.end(), "token with specified ID does not exist" );

	/// Ensure owner owns token
    eosio_assert( send_token->owner == from, "sender does not own token with specified ID");

	/// Notify both recipients
    require_recipient( from );
    require_recipient( to );

    /// Transfer NFT from sender to receiver
    tokens.modify( send_token, from, [&]( auto& data ) {
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

    /// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// 所持トークンを1つ探す
    uint64_t id = pcs::find_own_token( from, sym );

	/// Notify both recipients
    require_recipient( from );
	require_recipient( to );

    action(
        permission_level{ from, name("active") }, // このアカウントの権限を用いて
        get_self(), // このコントラクト内にある
        name("transferid"), // このメソッドに
        std::make_tuple( from, to, sym, id, memo ) // 引数をタプルで渡して
    ).send(); // アクションを実行する
}

void pcs::burn( name owner, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    /// Find token to burn
    auto burn_token = tokens.find( token_id );
    eosio_assert( burn_token != tokens.end(), "token with id does not exist" );
    eosio_assert( burn_token->owner == owner, "token not owned by account" );

	symbol_code sym = burn_token->sym;

	/// Remove token from tokens table
    tokens.erase( burn_token );

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
    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    symbol_code sym = target_token->sym;

    /// Notify payer
	require_recipient( owner );

    tokens.modify( target_token, owner, [&](auto& data) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = owner;
        data.sym = sym;
        data.active = 1;
    });

    decrease_balance( owner, sym );
	add_balance( owner, asset{ 1, symbol( sym, 0 ) } , owner );
}

void pcs::lock( name claimer, uint64_t token_id, string data, capi_signature sig ) {
    require_auth( claimer );
    // eosio_assert( claimer != get_self(), "do not lock token by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );

    /// 署名検証
    /// target_token に記載された public key に対応する
    /// private key を知っている人のみがロックの権限をもつ
    capi_public_key pk = target_token->subkey;
    capi_checksum256 digest;
    sha256(&data[0], data.size(), &digest);

    assert_recover_key(&digest, (const char *)&sig, sizeof(sig), (const char *)&pk, sizeof(pk));

    tokens.modify( target_token, get_self(), [&](auto& data) {
        data.active = 0;
    });
}

void pcs::servebid( name owner, uint64_t token_id, asset price, string memo ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not serve bid order by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    /// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// price に指定した asset が EOS であることの確認
    eosio_assert( price.symbol == symbol( "EOS", 4 ), "allow only EOS as price" );

    bids.emplace( owner, [&]( auto& data ) {
        data.id = token_id;
        data.price = price;
        data.owner = owner;
        data.sym = target_token->sym;
    });

    tokens.modify( target_token, get_self(), [&]( auto& data ) {
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
        permission_level{ get_self(), name("active") }, // このアカウントの権限を用いて
        name("eosio.token"), // このコントラクト内にある
        name("transfer"), // このメソッドに
        std::make_tuple( get_self(), to, value, memo ) // 引数をタプルで渡して
    ).send(); // アクションを実行する
}

void pcs::buy( name buyer, uint64_t token_id, string memo ) {
    require_auth( buyer );
    eosio_assert( buyer != get_self(), "does not buy token by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );

    auto bid_order = bids.find( token_id );
    eosio_assert( bid_order != bids.end(), "token with id does not exist" );

    /// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    bids.erase( bid_order );

    tokens.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = buyer;
        data.active = 0; /// lock until reflesh key for safety
    });

    /// トークンの残高を増やす
    add_balance( buyer, asset{ 1, symbol( bid_order->sym, 0 ) }, get_self() );

    /// 売り手に送金
    sub_deposit( buyer, bid_order->price );
    transfer_eos( bid_order->owner, bid_order->price, "executed as the inline action in pcs::buy of " + get_self().to_string() );
}

void pcs::cancelbid( name owner, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not cancel bid order by contract account" );

    auto target_token = tokens.find( token_id );
    auto bid_order = bids.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( bid_order != bids.end(), "order does not exist" );
    eosio_assert( bid_order->owner == owner, "order does not serve by account" );

    bids.erase( bid_order );

    tokens.modify( target_token, owner, [&]( auto& data ) {
        data.owner = owner;
    });

    add_balance( owner, asset{ 1, symbol( bid_order->sym, 0 ) }, owner );
}

void pcs::receive() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data< transfer_args >();
    name from = transfer_data.from;
    name to = transfer_data.to;
    asset quantity = transfer_data.quantity;
    string message = transfer_data.memo;

    eosio_assert( from == get_self() && to != get_self(), "does not allow to transfer from this contract to another" );

    /// Because "receive" action is called when this contract account
    /// is the sender or receiver of "eosio.token::transfer",
    /// so `to == get_self()` is needed.
    /// This contract account do not have balance record,
    /// so `from != get_self()` is needed.
    if ( to == get_self() && from != get_self() ) {
        pcs::add_deposit( from, quantity, get_self() );

        // For example, "buy token#0 in pcstoycashio" match this pattern.
        uint64_t token_id = pcs::get_hex_digit( message );

        if ( message == "buy token#" + std::to_string( token_id ) + " in " + get_self().to_string() ) {
            auto bid_order = bids.find( token_id );
            eosio_assert( bid_order != bids.end(), "order does not exist" );

            pcs::buy( from, token_id, "buy token in pcs::receive of " + get_self().to_string() );
        }
    }
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

    auto balance_data = eosbt.find( owner.value );

    if( balance_data == eosbt.end() ) {
        eosbt.emplace( ram_payer, [&]( auto& data ) {
            data.username = owner;
            data.quantity = quantity;
        });
    } else {
        eosbt.modify( balance_data, get_self(), [&]( auto& data ) {
            data.quantity += quantity;
        });
    }
}

void pcs::sub_deposit( name owner, asset quantity ) {
    eosio_assert( is_account( owner ), "owner account does not exist");
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "must EOS token" );

    auto balance_data = eosbt.find( owner.value );
    eosio_assert( balance_data != eosbt.end(), "your balance data do not exist" );

    asset deposit = balance_data->quantity;
    eosio_assert( deposit.amount >= quantity.amount, "exceed the withdrawable amount" );

    if ( deposit.amount == quantity.amount ) {
        eosbt.erase( balance_data );
    } else if ( deposit.amount > quantity.amount ) {
        eosbt.modify( balance_data, get_self(), [&]( auto& data ) {
            data.quantity -= quantity;
        });
    }
}

uint64_t pcs::find_own_token( name owner, symbol_code sym ) {
    auto token_table = tokens.get_index<name("bysymbol")>();

    auto it = token_table.lower_bound( sym.raw() );

    bool found = false;
    uint64_t token_id = 0;
    for (; it != token_table.end(); ++it ) {
        if ( it->sym == sym && it->owner == owner ) {
            token_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert( found, "token is not found or is not owned by account" );

    return token_id;
}

uint64_t pcs::get_hex_digit( string memo ) {
    eosio_assert( memo.size() <= 256, "too long memo" );
    const char* str = memo.c_str();

    uint8_t flag = 0;
    uint8_t start_flag = 0;
    uint8_t index = 0;
    vector<char> c;
    uint8_t i = 0;
    for (; str[i]; ++i ) {
        if ( start_flag == 1 ) {
            if (
                ( str[i] >= '0' && str[i] <= '9' ) ||
                ( str[i] >= 'a' && str[i] <= 'f' ) ||
                ( str[i] >= 'A' && str[i] <= 'F' )
            ) {
                if (index >= 16) {
                    flag = 0;
                } else {
                    flag = 1;
                    c.push_back( str[i] );
                    index += 1;
                }
            } else {
                if ( index > 0 ) {
                    if ( flag == 0 ) {
                        index = 0;
                        c.clear();
                    } else {
                        c.push_back( '\0' );
                        break;
                    }
                }
                start_flag = 0;
            }
        }

        if ( str[i] == '#' ) {
            start_flag = 1;
        }
    }

    eosio_assert( flag == 1, "token ID is not found" );

    string digit( c.begin(), c.end() );
    return static_cast<uint64_t>( std::stoull(digit, nullptr, 16) );
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
                   (create) /// currency
                   (issue)(transferid)(transfer)(burn) /// token
                   (refleshkey)(lock) /// token
                   (servebid)(buy)(cancelbid) /// bid
                   (withdraw) /// eos
               );
            }
        }
    }
}
