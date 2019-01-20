/**
 * @environment
 *     eosio.cdt v1.3.2
**/

#include "cmnt.hpp"

using namespace eosio;
using std::string;
using std::vector;

void cmnt::create( name issuer, symbol_code sym ) {
	require_auth( issuer ); // only create token by contract account
    eosio_assert( issuer != get_self(), "do not create token by contract account" );

	/// Check if issuer account exists
	eosio_assert( is_account( issuer ), "issuer account does not exist");

    eosio_assert( sym != symbol_code("EOS"), "EOS is invalid token symbol");

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data == currency_table.end(), "token with symbol already exists" );

    /// Create new currency
    currency_table.emplace( get_self(), [&]( auto& data ) {
        data.supply = asset{ 0, symbol(sym, 0) };
        data.issuer = issuer;
        data.minimumprice = asset{ 1, symbol("EOS", 4) };
        data.numofmember = 0;
        data.numofmanager = 1;
        data.othersratio = 0;
        data.pvcount = 0;
    });

    /// Issue token to issuer
    asset quantity = asset{ 1, symbol(sym, 0) };
    add_supply( quantity );
    add_balance( issuer, quantity, issuer );
    uint64_t token_id = mint_token( issuer, sym, issuer );

    /// Resister community manager
    community_index community_table( get_self(), sym.raw() );
    community_table.emplace( get_self(), [&]( auto& data ) {
        data.manager = token_id;
        data.ratio = 1;
    });
}

// void cmnt::destroy( symbol_code sym ) {
//     require_auth( get_self() ); // only destroy token by contract account
//
//     /// Valid symbol
//     eosio_assert( sym.is_valid(), "invalid symbol name" );
//
//     /// Check if currency with symbol already exists
//     auto currency_data = currency_table.find( sym.raw() );
//     eosio_assert( currency_data != currency_table.end(), "token with symbol does not exists" );
//     eosio_assert( currency_data->supply == asset{ 0, symbol(sym, 0) }, "who has this token exists" );
//
//     /// Delete currency
//     currency_table.erase( currency_data );
//
//     /// Delete community
//     community_index community_table( get_self(), sym.raw() );
//     auto it = community_table.begin();
//     for (; it != community_table.end();) {
//         community_table.erase( it++ );
//     }
// }

/// subkey を登録しないでトークン発行
uint64_t cmnt::mint_token( name user, symbol_code sym, name ram_payer ) {
    increment_member( user, sym );

    token_index token_table( get_self(), sym.raw() );
    uint64_t token_id = token_table.available_primary_key();
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_id;
        // subkey は初期値のまま
        data.owner = user;
        data.active = 0;
    });

    return token_id;
}

void cmnt::issue( name user, asset quantity, string memo ) {
    eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token for contract account" );

    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = currency_data->issuer;
    require_auth( issuer );

    /// Ensure valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == currency_data->supply.symbol, "symbol code or precision mismatch" );

    add_supply( quantity );
    add_balance( user, quantity, issuer );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = mint_token( user, sym, issuer );
    }
}

uint64_t cmnt::mint_unlock_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer ) {
    increment_member( user, sym );

    token_index token_table( get_self(), sym.raw() );
    uint64_t token_id = token_table.available_primary_key();
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = user;
        data.active = 1;
    });

    return token_id;
}

void cmnt::issueunlock( name user, asset quantity, vector<capi_public_key> subkeys, string memo ) {
	eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token by contract account" );

    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol code" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    name issuer = currency_data->issuer;
    require_auth( issuer );

    /// valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == currency_data->supply.symbol, "symbol code or precision mismatch" );

    // Check that number of tokens matches subkey size
    eosio_assert( quantity.amount == subkeys.size(), "mismatch between number of tokens and subkeys provided" );

    /// Increase supply and add balance
    add_supply( quantity );
    add_balance( user, quantity, issuer );

    uint64_t token_id;
    for ( auto const& subkey: subkeys ) {
        token_id = mint_unlock_token( user, sym, subkey, issuer );
    }
}

void cmnt::transferbyid( name from, name to, symbol_code sym, uint64_t id, string memo ) {
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
    token_index token_table( get_self(), sym.raw() );
    auto send_token = token_table.find( id );
    eosio_assert( send_token != token_table.end(), "token with specified ID does not exist" );

	/// Ensure owner owns token
    eosio_assert( send_token->owner == from, "sender does not own token with specified ID");

	/// Notify both recipients
    require_recipient( from );
    require_recipient( to );

    increment_member( to, sym );
    add_balance( to, asset{ 1, symbol(sym, 0) } , from );

    /// Transfer NFT from sender to receiver
    token_table.modify( send_token, from, [&]( auto& data ) {
        data.owner = to;
    });

    decrement_member( from, sym );
    sub_balance( from, asset{ 1, symbol(sym, 0) } );
}

void cmnt::transfer( name from, name to, symbol_code sym, string memo ) {
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
    uint64_t token_id = find_own_token( from, sym );

	/// Notify both recipients
    require_recipient( from );
	require_recipient( to );

    action(
        permission_level{ from, name("active") },
        get_self(),
        name("transferbyid"),
        std::make_tuple( from, to, token_id, memo )
    ).send();
}

void cmnt::burnbyid( name owner, symbol_code sym, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    /// Find token to burn
    token_index token_table( get_self(), sym.raw() );
    auto token_data = token_table.find( token_id );
    eosio_assert( token_data != token_table.end(), "token with id does not exist" );
    eosio_assert( token_data->owner == owner, "token not owned by account" );

    community_index community_table( get_self(), sym.raw() );
    auto community_data = community_table.find( token_id );
    eosio_assert( community_data == community_table.end(), "manager should not delete my token" );

	/// Remove token from tokens table
    token_table.erase( token_data );

    decrement_member( owner, sym );

    /// Lower balance from owner
    sub_balance( owner, asset{ 1, symbol(sym, 0) } );

    /// Lower supply from currency
    sub_supply( asset{ 1, symbol(sym, 0) } );
}

void cmnt::burn( name owner, asset quantity ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "quantity must be positive" );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist" );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = find_own_token( owner, sym );
        burnbyid( owner, sym, token_id );
    }
}

/// subkey を変更し lock 解除を行う
void cmnt::refleshkey( name owner, symbol_code sym, uint64_t token_id, capi_public_key subkey ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "do not reflesh key by contract account" );

    /// Find token to burn
    token_index token_table( get_self(), sym.raw() );
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    /// Notify payer
	require_recipient( owner );

    token_table.modify( target_token, owner, [&](auto& data) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = owner;
        data.active = 1;
    });

    /// RAM を所有者に負担させる
    sub_balance( owner, asset{ 1, symbol(sym, 0) } );
	add_balance( owner, asset{ 1, symbol(sym, 0) } , owner );
}

// void cmnt::lock( name claimer, uint64_t token_id, string data, capi_signature sig ) {
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

void cmnt::add_sell_order( name owner, symbol_code sym, uint64_t token_id, asset price ) {
    token_index token_table( get_self(), sym.raw() );
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = get_self();
    });

    sell_order_index sell_order_table( get_self(), sym.raw() );
    sell_order_table.emplace( owner, [&]( auto& data ) {
        data.id = token_id;
        data.price = price;
        data.owner = owner;
    });
}

void cmnt::sellbyid( name owner, symbol_code sym, uint64_t token_id, asset price ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not serve bid order by contract account" );

    token_index token_table( get_self(), sym.raw() );
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    /// price に指定した asset が EOS であることの確認
    eosio_assert( price.symbol == symbol("EOS", 4), "allow only EOS as price" );

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol do not exists" );
    eosio_assert( price >= currency_data->minimumprice, "price is lower than minimum selling price" );

    add_sell_order( owner, sym, token_id, price );

    decrement_member( owner, sym );
    sub_balance( owner, asset{ 1, symbol(sym, 0) } );
}

void cmnt::sell( name owner, asset quantity, asset price ) {
    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure valid quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity of NFT" );
    eosio_assert( quantity.symbol == currency_data->supply.symbol, "symbol code or precision mismatch" );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = find_own_token( owner, sym );
        token_index token_table( get_self(), sym.raw() );
        auto target_token = token_table.find( token_id );
        add_sell_order( owner, sym, token_id, price );
        decrement_member( owner, sym );
    }

    /// Lower balance from owner
    sub_balance( owner, quantity );
}

void cmnt::issueandsell( asset quantity, asset price, string memo ) {
    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
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

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = mint_token( issuer, sym, issuer );
        add_sell_order( issuer, sym, token_id, price );
    }
}

void cmnt::transfer_eos( name to, asset value, string memo ) {
    eosio_assert( is_account( to ), "to account does not exist" );
    eosio_assert( value.symbol == symbol("EOS", 4), "symbol of value must be EOS" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    action(
        permission_level{ get_self(), name("active") },
        name("eosio.token"),
        name("transfer"),
        std::make_tuple( get_self(), to, value, memo )
    ).send();
}

void cmnt::buy( name user, symbol_code sym, uint64_t token_id, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    token_index token_table( get_self(), sym.raw() );
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );

    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );

    name seller = sell_order->owner;
    asset price = sell_order->price;

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    sell_order_table.erase( sell_order );

    add_balance( user, asset{ 1, symbol(sym, 0) }, get_self() );
    increment_member( user, sym );

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = user;
        data.active = 0; /// lock until reflesh key for safety
    });

    eosio_assert( price.amount >= 0, "token price should not be negative" );

    if ( price.amount > 0 ) {
        deposit_index deposit_table( get_self(), user.value );
        auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
        eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );
        eosio_assert( deposit_data->balance >= price, "you do not have enough deposit" );

        /// 売り手に送金
        sub_deposit( user, price );
        transfer_eos( sell_order->owner, price, "executed as the inline action in cmnt::buyfromdex of " + get_self().to_string() );
    }
}

void cmnt::buyandunlock( name user, symbol_code sym, uint64_t token_id, capi_public_key subkey, string memo ) {
    buy( user, sym, token_id, memo );
    refleshkey( user, sym, token_id, subkey );
}

/// あらかじめ user にこのコントラクトの eosio.code permission をつけておかないと実行できない。
void cmnt::sendandbuy( name user, symbol_code sym, uint64_t token_id, capi_public_key subkey, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "token with id does not exist" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    deposit_index deposit_table( get_self(), user.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
    asset deposit = asset{ 0, symbol("EOS", 4) };

    if ( deposit_data != deposit_table.end() ) {
        deposit = deposit_data->balance;
    }

    asset price = sell_order->price;
    eosio_assert( price.symbol == symbol("EOS", 4), "symbol of price must be EOS" );
    eosio_assert( price.amount > 0, "price must be positive" );

    if ( deposit.amount < price.amount ) {
        asset quantity = price - deposit; // 不足分をデポジット
        string transfer_memo = "send EOS to buy token";
        action(
            permission_level{ user, name("active") },
            name("eosio.token"),
            name("transfer"),
            std::make_tuple( user, get_self(), quantity, transfer_memo )
        ).send();
    }

    string buy_memo = "call cmnt::buy as inline action in " + get_self().to_string();
    action(
        permission_level{ user, name("active") },
        get_self(),
        name("buyandunlock"),
        std::make_tuple( user, sym, token_id, subkey, buy_memo )
    ).send();
}

void cmnt::cancelbyid( name owner, symbol_code sym, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not cancel bid order by contract account" );

    token_index token_table( get_self(), sym.raw() );
    auto target_token = token_table.find( token_id );
    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );
    eosio_assert( sell_order->owner == owner, "order does not serve by account" );

    sell_order_table.erase( sell_order );

    increment_member( owner, sym );
    add_balance( owner, asset{ 1, symbol(sym, 0) }, owner );

    token_table.modify( target_token, owner, [&]( auto& data ) {
        data.owner = owner;
    });
}

void cmnt::cancel( name owner, asset quantity ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "quantity must be positive" );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist" );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = find_own_order( owner, sym );
        cancelbyid( owner, sym, token_id );
    }
}

void cmnt::cancelburn( name owner, asset quantity ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "quantity must be positive" );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist" );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = find_own_order( owner, sym );
        cancelbyid( owner, sym, token_id );
        burnbyid( owner, sym, token_id );
    }
}

// void cmnt::reserve( name user, symbol sym, asset price ) {
//     require_auth( user );
//
//     /// price に指定した asset が EOS であることの確認
//     eosio_assert( price.symbol == symbol("EOS", 4), "allow only EOS as price" );
//
//     auto currency_data = currency_table.find( sym.raw() );
//     eosio_assert( currency_data != currency_table.end(), "token with symbol do not exists" );
//     eosio_assert( price >= currency_data->minimumprice, "price is lower than minimum selling price" );
//
//     buy_order_table.emplace( user, [&]( auto& data ) {
//         data.id = buy_order_table.available_primary_key();
//         data.price = price;
//         data.owner = user;
//         data.sym = sym;
//     });
// }

void cmnt::setmanager( symbol_code sym, uint64_t manager_token_id, vector<uint64_t> manager_token_list, vector<uint64_t> ratio_list, uint64_t others_ratio ) {
    token_index token_table( get_self(), sym.raw() );
    auto token_data = token_table.find( manager_token_id );
    eosio_assert( token_data != token_table.end(), "token with id is not found" );

    name manager = token_data->owner;
    require_auth( manager );

    community_index community_table( get_self(), sym.raw() );
    auto community_data = community_table.find( manager_token_id );
    eosio_assert( community_data != community_table.end(), "you are not manager" );

    uint64_t manager_list_length = manager_token_list.size();
    uint64_t ratio_list_length = ratio_list.size();
    eosio_assert( manager_list_length == ratio_list_length, "list size is invalid" );

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "currency with symbol does not exist" );

    float_t sum_of_ratio = others_ratio;
    uint64_t token_id;
    uint64_t i;
    for ( i = 0; i != manager_list_length; ++i ) {
        token_data = token_table.find( manager_token_list[i] );
        eosio_assert( token_data != token_table.end(), "token with id is not found" );

        eosio_assert( sum_of_ratio + ratio_list[i] > sum_of_ratio, "occur overflow" ); // check overflow
        sum_of_ratio += ratio_list[i];

        auto community_data = community_table.find( manager_token_list[i] );
        if ( community_data != community_table.end() ) {
            community_table.erase( community_data );
        }
    }

    eosio_assert( sum_of_ratio > 0, "sum of ratio must be positive" );

    uint64_t num_of_manager = 0;
    for ( i = 0; i != manager_list_length; ++i ) {
        community_data = community_table.find( manager_token_list[i] );

        if ( community_data == community_table.end() ) {
            community_table.emplace( get_self(), [&]( auto& data ) {
                data.manager = manager_token_list[i];
                data.ratio = static_cast<float_t>(ratio_list[i]) / sum_of_ratio;
            });

            num_of_manager += 1;
        } else {
            community_table.modify( community_data, get_self(), [&]( auto& data ) {
                data.ratio += static_cast<float_t>(ratio_list[i]) / sum_of_ratio;
            });
        }
    }

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.numofmanager = num_of_manager;
        data.othersratio = static_cast<float_t>(others_ratio) / sum_of_ratio;
    });
}

void cmnt::receive() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data<transfer_args>();
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
        add_deposit( from, quantity, get_self() );

        vector<string> sbc = split_by_comma( message );
        string contract_name = get_self().to_string();

        if ( sbc[0] == contract_name && sbc[1] == "buy" && sbc.size() == 4 ) {
            uint64_t token_id = static_cast<uint64_t>( std::stoull(sbc[3]) );
            symbol_code sym = symbol_code( sbc[2] );

            sell_order_index sell_order_table( get_self(), sym.raw() );
            auto sell_order = sell_order_table.find( token_id );
            eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );

            /// unlock までやりたいならば、 string -> capi_public_key の変換関数が必要
            buy( from, sym, token_id, string("buy token in cmnt::receive of ") + contract_name );
        }
    } else if ( from == get_self() && to != get_self() ) {
        auto total_deposit = total_deposit_table.find( symbol_code("EOS").raw() );
        asset total_amount = asset{ 0, symbol("EOS", 4) };

        if ( total_deposit != total_deposit_table.end() ) {
            total_amount = total_deposit->balance;
        }

        account_index account_table( name("eosio.token"), get_self().value );
        auto account_data = account_table.find( symbol_code("EOS").raw() );
        asset current_amount = asset{ 0, symbol("EOS", 4) };

        if ( account_data != account_table.end() ) {
            current_amount = account_data->balance;
        }

        /// このコントラクトの EOS 残高を check し、預かっている額よりも減らないように監視する
        eosio_assert( total_amount <= current_amount, "this contract must leave at least total deposit" );
    }
}

void cmnt::withdraw( name user, asset quantity, string memo ) {
    require_auth( user );
    sub_deposit( user, quantity );
    transfer_eos( user, quantity, memo );
}

void cmnt::setoffer( name provider, symbol_code sym, string uri, asset price ) {
    require_auth( provider );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );

    eosio_assert( price.amount >= 0, "offer fee must be not negative value" );
    eosio_assert( price.symbol == symbol("EOS", 4), "symbol of price must be EOS" );

    /// オファー料金が前もってコントラクトに振り込まれているか確認
    if ( price.amount != 0 ) {
        deposit_index deposit_table( get_self(), provider.value );
        auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
        eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

        asset deposit = deposit_data->balance;
        eosio_assert( deposit.amount >= price.amount, "offer fee exceed your deposit" );
    }

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist." );

    offer_index offer_list( get_self(), sym.raw() );
    offer_list.emplace( provider, [&]( auto& data ) {
        data.id = offer_list.available_primary_key();
        data.price = price;
        data.provider = provider;
        data.uri = uri;
    });
}

void cmnt::acceptoffer( name manager, symbol_code sym, uint64_t offer_id ) {
    require_auth( manager );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist." );

    community_index community_table( get_self(), sym.raw() );
    auto community_data = community_table.find( manager.value );
    eosio_assert( community_data == community_table.end(), "manager must be community manager" );

    offer_index offer_list( get_self(), sym.raw() );
    auto offer_data = offer_list.find( offer_id );
    eosio_assert( offer_data != offer_list.end(), "offer data do not exist" );

    asset price = offer_data->price;
    eosio_assert( price.amount >= 0, "offer fee must be not negative value" );

    name provider = offer_data->provider;

    string uri = offer_data->uri;
    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );

    /// 受け入れられた offer は content に昇格する
    offer_list.erase( offer_data );

    /// TODO: コンテンツが増えてきたら、古いものから消去する
    // if ( contents_table.size() > 5 ) { // size メンバは存在しないので別の方法を探す
    //     auto oldest_content = contents_table.get_index<name("bytimestamp")>().lower_bound(0);
    //     contents_table.erase( oldest_content );
    // }

    uint64_t now = current_time();

    contents_table.emplace( manager, [&]( auto& data ) {
        data.id = contents_table.available_primary_key();
        data.sym = sym;
        data.price = price;
        data.provider = provider;
        data.uri = uri;
        data.pvcount = 0;
        data.accepted = now;
        data.active = 1;
    });

    require_recipient( provider );

    update_pv_rate( sym, now, price );

    if ( price.amount != 0 ) {
        deposit_index deposit_table( get_self(), provider.value );
        auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
        eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

        sub_deposit( provider, price );

        /// 割り切れないときや、DEX に出しているときや、
        /// num_of_owner と num_of_manager が一致するときは、
        /// コントラクトが others_ratio 分をもらう

        asset share;
        uint64_t num_of_owner = currency_data->supply.amount; /// int64 -> uint64
        uint64_t num_of_manager = currency_data->numofmanager;
        uint64_t others_ratio = currency_data->othersratio;

        token_index token_table( get_self(), sym.raw() );
        auto it = token_table.begin();
        for (; it != token_table.end(); ++it ) {
            uint64_t token_id = it->id;
            name owner = it->owner;
            auto community_data = community_table.find( token_id );
            if ( community_data == community_table.end() ) {
                /// トークン管理者でなければ、 price * others ratio をトークン所持者で均等に分配
                share = asset{ static_cast<int64_t>( static_cast<float_t>(price.amount) * ( others_ratio / (num_of_owner - num_of_manager) ) ), price.symbol };
                if ( share.amount > 0 ) {
                    transfer_eos( owner, share, "executed as the inline action in cmnt::acceptoffer of " + get_self().to_string() );
                }
            } else {
                /// トークン管理者でなければ、 price * ratio を分配
                share = asset{ static_cast<int64_t>( static_cast<float_t>(price.amount) * community_data->ratio ), price.symbol };
                if ( share.amount > 0 ) {
                    transfer_eos( manager, share, "executed as the inline action in cmnt::acceptoffer of " + get_self().to_string() );
                }
            }
        }
    }
}

void cmnt::removeoffer( name provider, symbol_code sym, uint64_t offer_id ) {
    require_auth( provider );

    offer_index offer_list( get_self(), sym.raw() );
    auto offer_data = offer_list.find( offer_id );
    eosio_assert( offer_data != offer_list.end(), "offer data do not exist" );
    eosio_assert( offer_data->provider == provider, "you are not provider of this offer" );

    offer_list.erase( offer_data );

    sub_deposit( provider, offer_data->price );
    transfer_eos( provider, offer_data->price, "executed as the inline action in cmnt::acceptoffer of " + get_self().to_string() );
}

void cmnt::addpvcount( uint64_t contents_id, uint64_t pv_count ) {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( get_self() );

    /// すでに指定した uri に関する pv のデータがあることを確認
    auto contents_data = contents_table.find( contents_id );
    eosio_assert( contents_data != contents_table.end(), "this contents is not exist in the contents table" );
    // eosio_assert( contents_data->active != 0, "this contents is not active" );

    /// get block timestamp
    uint64_t now = current_time();

    /// add contents pv count
    pv_count_index pv_count_table( get_self(), contents_id );
    auto pv_count_data = pv_count_table.find( now );

    if ( pv_count_data == pv_count_table.end() ) {
        pv_count_table.emplace( get_self(), [&]( auto& data ) {
            data.timestamp = now;
            data.count = pv_count;
        });
    } else {
        pv_count_table.modify( pv_count_data, get_self(), [&]( auto& data ) {
            data.count += pv_count;
        });
    }

    contents_table.modify( contents_data, get_self(), [&]( auto& data ) {
        data.pvcount += pv_count;
    });

    /// add community pv count
    symbol_code sym = contents_data->sym;
    cmnty_pv_count_index cmnty_pv_count_table( get_self(), sym.raw() );

    auto cmnty_pv_count_data = cmnty_pv_count_table.find( now );
    if ( cmnty_pv_count_data == cmnty_pv_count_table.end() ) {
        cmnty_pv_count_table.emplace( get_self(), [&]( auto& data ) {
            data.timestamp = now;
            data.count = pv_count;
        });
    } else {
        cmnty_pv_count_table.modify( cmnty_pv_count_data, get_self(), [&]( auto& data ) {
            data.count += pv_count;
        });
    }

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "this currency does not exist" );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.pvcount = get_cmnty_pv_count( sym );
    });

    /// add world pv count
    auto world_pv_count_data = world_pv_count_table.find( now );
    if ( world_pv_count_data == world_pv_count_table.end() ) {
        world_pv_count_table.emplace( get_self(), [&]( auto& data ) {
            data.timestamp = now;
            data.count = pv_count;
        });
    } else {
        world_pv_count_table.modify( world_pv_count_data, get_self(), [&]( auto& data ) {
            data.count += pv_count;
        });
    }

    auto world_data = world_table.find( get_self().value );
    world_table.modify( world_data, get_self(), [&]( auto& data ) {
        data.timestamp = now;
        data.pvcount = get_world_pv_count();
    });
}

// void cmnt::resetpvcount( uint64_t contents_id ) {
//     /// コントラクトアカウントのみが呼び出せる
//     require_auth( get_self() );
//
//     /// すでに指定した uri に関する pv のデータがあることを確認
//     auto contents_data = contents_table.find( contents_id );
//     eosio_assert( contents_data != contents_table.end(), "this contents is not exist in the contents table" );
//
//     contents_table.modify( contents_data, get_self(), [&]( auto& data ) {
//         data.pvcount = 0;
//     });
//
//     pv_count_index pv_count_table( get_self(), contents_id );
//     auto data = pv_count_table.begin();
//     while ( data != pv_count_table.end() ) {
//         pv_count_table.erase( data );
//         pv_rate_index pv_rate_table( get_self(), contents_id );
//         data = pv_count_table.begin();
//     }
// }

// void cmnt::stopcontent( name manager, uint64_t contents_id ) {
//     require_auth( manager );
//
//     auto contents_data = contents_table.find( contents_id );
//     contents_table.modify( contents_data, manager, [&]( auto& data ) {
//         data.active = 0;
//     });
// }

// void cmnt::startcontent( name manager, uint64_t contents_id ) {
//     require_auth( manager );
//
//     auto contents_data = contents_table.find( contents_id );
//     contents_table.modify( contents_data, manager, [&]( auto& data ) {
//         data.active = 1;
//     });
// }

// void cmnt::dropcontent( name manager, uint64_t contents_id ) {
//     require_auth( manager );
//
//     auto contents_data = contents_table.find( contents_id );
//     contents_table.erase( contents_data );
// }

uint64_t cmnt::get_cmnty_pv_count( symbol_code sym ) {
    cmnty_pv_count_index cmnty_pv_count_table( get_self(), sym.raw() );

    /// あるいは過去30日分だけを取り出す
    auto it = cmnty_pv_count_table.lower_bound( static_cast<uint64_t>( current_time() - 30*24*60*60 ) );

    uint64_t cmnty_pv_count = 0;
    for (; it != cmnty_pv_count_table.end(); ++it ) {
        cmnty_pv_count += it->count;
    }
    return cmnty_pv_count;
}

uint64_t cmnt::get_world_pv_count() {
    /// あるいは過去30日分だけを取り出す
    auto it = world_pv_count_table.lower_bound( static_cast<uint64_t>( current_time() - 30*24*60*60 ) );

    uint64_t world_pv_count = 0;
    for (; it != world_pv_count_table.end(); ++it ) {
        world_pv_count += it->count;
    }
    return world_pv_count;
}


void cmnt::update_pv_rate( symbol_code sym, uint64_t timestamp, asset new_offer_price ) {
    eosio_assert( new_offer_price.symbol == symbol("EOS", 4), "symbol of offer price must be EOS" );

    eosio_assert( timestamp != 0, "invalid timestamp" );

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "this currency does not exist" );
    uint64_t cmnty_pv_count = currency_data->pvcount;

    uint64_t world_pv_count = get_world_pv_count();

    float_t last_pv_rate = 0;
    auto latest_pv_rate_data = pv_rate_table.rbegin();

    if ( latest_pv_rate_data != pv_rate_table.rend() ) {
        last_pv_rate = latest_pv_rate_data->rate;
    }

    float_t pv_rate = (last_pv_rate * world_pv_count + new_offer_price.amount) / (world_pv_count + cmnty_pv_count);

    auto pv_rate_data = pv_rate_table.find( timestamp );
    if ( pv_rate_data == pv_rate_table.end() ) {
        pv_rate_table.emplace( get_self(), [&]( auto& data ) {
            data.timestamp = timestamp;
            data.rate = pv_rate;
        });
    } else {
        pv_rate_table.modify( pv_rate_data, get_self(), [&]( auto& data ) {
            data.rate = pv_rate;
        });
    }
}

void cmnt::update_minimum_price( symbol_code sym ) {
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "this currency does not exist" );
    uint64_t cmnty_pv_count = currency_data->pvcount;

    float_t last_pv_rate = 0;
    auto latest_pv_rate_data = pv_rate_table.rend();
    if ( latest_pv_rate_data != pv_rate_table.rbegin() ) {
        last_pv_rate = latest_pv_rate_data->rate;
    }

    /// TODO: overflow 対策が必要?
    int64_t minimum_price = cmnty_pv_count * last_pv_rate;

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.minimumprice = asset{ minimum_price, symbol("EOS", 4) };
    });
}

void cmnt::add_balance( name owner, asset quantity, name ram_payer ) {
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

void cmnt::sub_balance( name owner, asset quantity ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

    account_index account_table( get_self(), owner.value );
    auto account_data = account_table.find( quantity.symbol.code().raw() );
    eosio_assert( account_data != account_table.end(), "no balance object found" );

    eosio_assert( account_data->balance >= quantity, "overdrawn balance" );

    if ( account_data->balance == quantity ) {
        account_table.erase( account_data );
    } else {
        account_table.modify( account_data, owner, [&]( auto& data ) {
            data.balance -= quantity;
        });
    }
}

void cmnt::add_supply( asset quantity ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

    symbol_code sym = quantity.symbol.code();

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.supply += quantity;
    });
}

void cmnt::sub_supply( asset quantity ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

    symbol_code sym = quantity.symbol.code();

    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );
    eosio_assert( currency_data->supply >= quantity, "exceed the withdrawable amount" );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.supply -= quantity;
    });
}

void cmnt::add_deposit( name owner, asset quantity, name ram_payer ) {
    eosio_assert( is_account( owner ), "owner account does not exist" );
    eosio_assert( is_account( ram_payer ), "ram_payer account does not exist" );
    eosio_assert( quantity.symbol == symbol("EOS", 4), "must EOS token" );

    deposit_index deposit_table( get_self(), owner.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );

    if( deposit_data == deposit_table.end() ) {
        deposit_table.emplace( ram_payer, [&]( auto& data ) {
            data.balance = quantity;
        });
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance += quantity;
        });
    }

    auto total_deposit = total_deposit_table.find( symbol_code("EOS").raw() );

    if ( total_deposit == total_deposit_table.end() ) {
        total_deposit_table.emplace( get_self(), [&]( auto& data ) {
            data.balance = quantity;
        });
    } else {
        total_deposit_table.modify( total_deposit, get_self(), [&]( auto& data ) {
            data.balance += quantity;
        });
    }
}

void cmnt::sub_deposit( name owner, asset quantity ) {
    eosio_assert( is_account( owner ), "owner account does not exist");
    eosio_assert( quantity.symbol == symbol("EOS", 4), "must be EOS token" );
    eosio_assert( quantity.amount >= 0, "must be nonnegative" );

    deposit_index deposit_table( get_self(), owner.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
    eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

    asset deposit = deposit_data->balance;
    eosio_assert( deposit >= quantity, "exceed the withdrawable amount" );

    if ( deposit == quantity ) {
        deposit_table.erase( deposit_data );
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance -= quantity;
        });
    }

    auto total_deposit = total_deposit_table.find( symbol_code("EOS").raw() );
    eosio_assert( total_deposit != total_deposit_table.end(), "total deposit data do not exist" );
    eosio_assert( total_deposit->balance >= quantity, "can not subtract more than total deposit" );

    if ( total_deposit->balance == quantity ) {
        total_deposit_table.erase( total_deposit );
    } else {
        total_deposit_table.modify( total_deposit, get_self(), [&]( auto& data ) {
            data.balance -= quantity;
        });
    }
}

/// トークンを増やす前に確認する
void cmnt::increment_member( name user, symbol_code sym ) {
    token_index token_table( get_self(), sym.raw() );
    auto it = token_table.begin();
    bool found = false;
    for (; it != token_table.end(); ++it ) {
        if ( it->owner == user ) {
            found = true;
            break;
        }
    }

    if ( !found ) {
        auto currency_data = currency_table.find( sym.raw() );
        eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );
        currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
            data.numofmember += 1;
        });
    }
}

/// トークンを減らした後に確認する
void cmnt::decrement_member( name user, symbol_code sym ) {
    token_index token_table( get_self(), sym.raw() );
    auto it = token_table.begin();
    bool found = false;
    for (; it != token_table.end(); ++it ) {
        if ( it->owner == user ) {
            found = true;
            break;
        }
    }

    if ( !found ) {
        auto currency_data = currency_table.find( sym.raw() );
        eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );
        currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
            data.numofmember -= 1;
        });
    }
}

uint64_t cmnt::find_own_token( name owner, symbol_code sym ) {
    eosio_assert( is_account( owner ), "invalid account" );

    token_index token_table( get_self(), sym.raw() );

    auto it = token_table.begin();
    bool found = false;
    uint64_t token_id = 0;
    for (; it != token_table.end(); ++it ) {
        if ( it->owner == owner ) {
            token_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert( found, "token is not found or is not owned by account" );

    return token_id;
}

uint64_t cmnt::find_own_order( name owner, symbol_code sym ) {
    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto it = sell_order_table.begin();
    bool found = false;
    uint64_t token_id = 0;
    for (; it != sell_order_table.end(); ++it ) {
        if ( it->owner == owner ) {
            token_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert( found, "sell order is not found or is not owned by account" );

    return token_id;
}

uint64_t cmnt::find_pvdata_by_uri( symbol_code sym, string uri ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exist. create token before issue" );

    auto it = contents_table.begin();

    /// uri でテーブルを検索
    bool found = false;
    uint64_t uri_id = 0;
    for (; it != contents_table.end(); ++it ) {
        if ( it->uri == uri ) {
            uri_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert( found, "uri is not found" );

    return uri_id;
}

vector<string> cmnt::split_by_comma( string str ) {
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
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if ( code == name("eosio.token").value ) {
            if ( action == name("transfer").value ) {
                /// When one call eosio.token::transfer action,
                /// receiveeos::transfereos action is also executed.
                execute_action( name(receiver), name(code), &cmnt::receive );
            }
        } else if ( code == receiver ) {
            switch( action ) {
               EOSIO_DISPATCH_HELPER( cmnt,
                   (create)
                   // (destroy)
                   (issue)
                   (issueunlock)
                   (transferbyid)
                   (transfer)
                   (burnbyid)
                   (burn)
                   (refleshkey)
                   (sellbyid)
                   (sell)
                   (issueandsell)
                   (buy)
                   (buyandunlock)
                   (sendandbuy)
                   (cancelbyid)
                   (cancel)
                   (cancelburn)
                   (setmanager)
                   (withdraw)
                   (setoffer)
                   (acceptoffer)
                   (removeoffer)
                   // (resetpvcount)
                   (addpvcount)
                   // (stopcontent)
                   // (startcontent)
                   // (dropcontent)
               );
            }
        }
    }
}
