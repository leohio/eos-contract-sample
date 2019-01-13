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
        data.supply = asset{ 0, symbol( sym, 0 ) };
        data.issuer = issuer;
    });
}

void cmnt::destroy( symbol_code sym ) {
    require_auth( get_self() ); // only destroy token by contract account

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data != currency_table.end(), "token with symbol does not exists" );
    eosio_assert( currency_data->supply == asset{ 0, symbol(sym, 0) }, "who has this token exists" );

    /// Create new currency
    currency_table.erase( currency_data );
}

/// subkey を登録しないでトークン発行
uint64_t cmnt::mint_token( name user, symbol_code sym, name ram_payer ) {
    uint64_t token_id = token_table.available_primary_key();
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_id;
        // subkey は初期値のまま
        data.owner = user;
        data.sym = sym;
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

    /// Increase supply and add balance
    add_supply( quantity );
    add_balance( user, quantity, issuer );

    uint64_t i = 0;
    uint64_t token_id;
    for (; i != quantity.amount; ++i ) {
        token_id = mint_token( user, sym, issuer );
    }
}

uint64_t cmnt::mint_unlock_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer ) {
    uint64_t token_id = token_table.available_primary_key();
    token_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = user;
        data.sym = sym;
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

void cmnt::transferbyid( name from, name to, uint64_t id, string memo ) {
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
    uint64_t id = cmnt::find_own_token( from, sym );

	/// Notify both recipients
    require_recipient( from );
	require_recipient( to );

    action(
        permission_level{ from, name("active") },
        get_self(),
        name("transferbyid"),
        std::make_tuple( from, to, id, memo )
    ).send();
}

void cmnt::burnbyid( name owner, uint64_t token_id ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    /// Find token to burn
    auto burn_token = token_table.find( token_id );
    eosio_assert( burn_token != token_table.end(), "token with id does not exist" );
    eosio_assert( burn_token->owner == owner, "token not owned by account" );

	symbol_code sym = burn_token->sym;

	/// Remove token from tokens table
    token_table.erase( burn_token );

    /// Lower balance from owner
    decrease_balance( owner, sym );

    /// Lower supply from currency
    decrease_supply( sym );
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
        burnbyid( owner, token_id );
    }
}

/// subkey を変更し lock 解除を行う
void cmnt::refleshkey( name owner, uint64_t token_id, capi_public_key subkey ) {
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

    /// RAM を所有者に負担させる
    decrease_balance( owner, sym );
	add_balance( owner, asset{ 1, symbol( sym, 0 ) } , owner );
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

void cmnt::add_sell_order( name owner, uint64_t token_id, asset price ) {
    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );

    sell_order_table.emplace( owner, [&]( auto& data ) {
        data.id = token_id;
        data.price = price;
        data.owner = owner;
        data.sym = target_token->sym;
    });
}

void cmnt::sellbyid( name owner, uint64_t token_id, asset price ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not serve bid order by contract account" );

    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    /// price に指定した asset が EOS であることの確認
    eosio_assert( price.symbol == symbol( "EOS", 4 ), "allow only EOS as price" );

    add_sell_order( owner, token_id, price );

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.owner = get_self(); /// 所有者をコントラクトアカウントに
    });

    /// Lower balance from owner
    decrease_balance( owner, target_token->sym );
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
        token_id = mint_token( get_self(), sym, get_self() );
        add_sell_order( issuer, token_id, price );
    }
}

void cmnt::transfer_eos( name to, asset value, string memo ) {
    eosio_assert( is_account( to ), "to account does not exist" );
    eosio_assert( value.symbol == symbol( "EOS", 4 ), "symbol of value must be EOS" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    action(
        permission_level{ get_self(), name("active") },
        name("eosio.token"),
        name("transfer"),
        std::make_tuple( get_self(), to, value, memo )
    ).send();
}

void cmnt::buy( name user, uint64_t token_id, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    auto target_token = token_table.find( token_id );
    eosio_assert( target_token != token_table.end(), "token with id does not exist" );

    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );

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
    transfer_eos( sell_order->owner, sell_order->price, "executed as the inline action in cmnt::buyfromdex of " + get_self().to_string() );
}

void cmnt::buyandunlock( name user, uint64_t token_id, capi_public_key subkey, string memo ) {
    buy( user, token_id, memo );
    refleshkey( user, token_id, subkey );
}

// あらかじめ user にこのコントラクトの eosio.code permission をつけておかないと実行できない。
void cmnt::sendandbuy( name user, uint64_t token_id, capi_public_key subkey, string memo ) {
    require_auth( user );
    eosio_assert( user != get_self(), "does not buy token by contract account" );

    auto sell_order = sell_order_table.find( token_id );
    eosio_assert( sell_order != sell_order_table.end(), "token with id does not exist" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    deposit_index deposit_table( get_self(), user.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
    asset deposit = asset{ 0, symbol("EOS", 4) };

    if ( deposit_data != deposit_table.end() ) {
        deposit = deposit_data->quantity;
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
        std::make_tuple( user, token_id, subkey, buy_memo )
    ).send();
}

void cmnt::cancelbyid( name owner, uint64_t token_id ) {
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
        cancelbyid( owner, token_id );
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
        cancelbyid( owner, token_id );
        burnbyid( owner, token_id );
    }
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

        if ( sbc[0] == contract_name && sbc[1] == "buy" && sbc.size() == 3 ) {
            uint64_t token_id = static_cast<uint64_t>( std::stoull(sbc[2]) );

            auto sell_order = sell_order_table.find( token_id );
            eosio_assert( sell_order != sell_order_table.end(), "order does not exist" );

            /// unlock までやりたいならば、 string -> capi_public_key の変換関数が必要
            buy( from, token_id, string("buy token in cmnt::receive of ") + contract_name );
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
    eosio_assert( price.symbol == symbol( "EOS", 4 ), "symbol of price must be EOS" );

    /// オファー料金が前もってコントラクトに振り込まれているか確認
    if ( price.amount != 0 ) {
        deposit_index deposit_table( get_self(), provider.value );
        auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
        eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

        asset deposit = deposit_data->quantity;
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
    eosio_assert( currency_data->issuer == manager, "manager must be token issuer" );

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

    /// コンテンツが増えてきたら、古いものから消去する
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
        data.count = 0;
        data.accepted = now;
        data.active = 1;
    });

    /// オファー料金が前もってコントラクトに振り込まれているか確認
    if ( price.amount != 0 ) {
        deposit_index deposit_table( get_self(), provider.value );
        auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
        eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );

        sub_deposit( provider, price );
        transfer_eos( manager, price, "executed as the inline action in cmnt::acceptoffer of " + get_self().to_string() );
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
    eosio_assert( contents_data->active != 0, "this contents is not active" );

    /// overflow 対策
    eosio_assert( contents_data->count + pv_count > contents_data->count, "pv count overflow, so revert state." );

    contents_table.modify( contents_data, get_self(), [&]( auto& data ) {
        data.count += pv_count;
    });

    /// get block timestamp
    uint64_t now = current_time();

    pv_count_index pv_count_table( get_self(), contents_id );
    eosio_assert( pv_count_table.find( now ) == pv_count_table.end(), "already have entered data" );

    pv_count_table.emplace( get_self(), [&]( auto& data ) {
        data.timestamp = now;
        data.count = pv_count;
    });
}

uint64_t cmnt::get_sum_of_pv_count( uint64_t contents_id ) {
    uint64_t sum_of_pv = 0;
    pv_count_index pv_count_table( get_self(), contents_id );
    auto it = pv_count_table.begin();
    for (; it != pv_count_table.end(); ++it ) {
        eosio_assert( sum_of_pv + it->count >= sum_of_pv, "overflow occured, so revert state" );
        sum_of_pv += it->count;
        pv_count_table.erase( it );
    }
    return sum_of_pv;
}

void cmnt::updatepvrate() {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( get_self() );

    /// get block timestamp
    uint64_t now = current_time();

    eosio_assert( pv_rate_table.find( now ) == pv_rate_table.end(), "already have entered data" );

    float_t last_pv_rate = 0;
    auto last_pv_rate_data = pv_rate_table.rend();
    if ( last_pv_rate_data != pv_rate_table.rbegin() ) {
        last_pv_rate = last_pv_rate_data->rate;
    }

    float_t pv_rate = last_pv_rate + 0;

    // for () {
    //     auto contents_data = contents_table.find( contents_id );
    //     uint64_t add_pv_count = get_sum_of_pv_count( contents_id );
    //     uint64_t last_pv_count = contents_data->count;
    // }

    pv_rate_table.emplace( get_self(), [&]( auto& data ) {
        data.timestamp = now;
        data.rate = pv_rate;
    });
}

void cmnt::resetpvcount( uint64_t contents_id ) {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( get_self() );

    /// すでに指定した uri に関する pv のデータがあることを確認
    auto contents_data = contents_table.find( contents_id );
    eosio_assert( contents_data != contents_table.end(), "this contents is not exist in the contents table" );

    contents_table.modify( contents_data, get_self(), [&]( auto& data ) {
        data.count = 0;
    });

    // pv_count_index pv_count_table( get_self(), contents_id );
    // auto it = pv_count_table.begin();
    // for (; it != pv_count_table.end(); ++it ) {
    //     pv_count_table.erase( it );
    // }
}

void cmnt::stopcontents( name manager, uint64_t contents_id ) {
    require_auth( manager );

    auto contents_data = contents_table.find( contents_id );
    contents_table.modify( contents_data, manager, [&]( auto& data ) {
        data.active = 0;
    });
}

void cmnt::dropcontents( name manager, uint64_t contents_id ) {
    require_auth( manager );

    auto contents_data = contents_table.find( contents_id );
    contents_table.erase( contents_data );
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

void cmnt::decrease_balance( name owner, symbol_code sym ) {
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

void cmnt::add_supply( asset quantity ) {
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision must be zero" );
    eosio_assert( quantity.amount > 0, "invalid quantity" );

    symbol_code sym = quantity.symbol.code();

    auto current_currency = currency_table.find( sym.raw() );
    eosio_assert( current_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply += quantity;
    });
}

void cmnt::decrease_supply( symbol_code sym ) {
    auto current_currency = currency_table.find( sym.raw() );
    eosio_assert( current_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply -= asset{ 1, symbol( sym, 0 ) };
    });
}

void cmnt::add_deposit( name owner, asset quantity, name ram_payer ) {
    eosio_assert( is_account( owner ), "owner account does not exist" );
    eosio_assert( is_account( ram_payer ), "ram_payer account does not exist" );
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "must EOS token" );

    deposit_index deposit_table( get_self(), owner.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );

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
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "must be EOS token" );
    eosio_assert( quantity.amount >= 0, "must be nonnegative" );

    deposit_index deposit_table( get_self(), owner.value );
    auto deposit_data = deposit_table.find( symbol_code("EOS").raw() );
    eosio_assert( deposit_data != deposit_table.end(), "your balance data do not exist" );
    eosio_assert( deposit_data->quantity >= quantity, "exceed the withdrawable amount" );

    if ( deposit_data->quantity == quantity ) {
        deposit_table.erase( deposit_data );
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.quantity -= quantity;
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

uint64_t cmnt::find_own_token( name owner, symbol_code sym ) {
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

uint64_t cmnt::find_own_order( name owner, symbol_code sym ) {
    auto sell_order_table2 = sell_order_table.get_index<name("bysymbol")>();

    auto it = sell_order_table2.lower_bound( sym.raw() );

    bool found = false;
    uint64_t token_id = 0;
    for (; it != sell_order_table2.end(); ++it ) {
        if ( it->sym == sym && it->owner == owner ) {
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
                   (destroy)
                   (issue)
                   (issueunlock)
                   (transferbyid)
                   (transfer)
                   (burnbyid)
                   (burn)
                   (refleshkey)
                   (sellbyid)
                   (issueandsell)
                   (buy)
                   (buyandunlock)
                   (sendandbuy)
                   (cancelbyid)
                   (cancel)
                   (cancelburn)
                   // (withdraw)
                   // (setoffer)
                   // (acceptoffer)
                   // (removeoffer)
                   // (addpvcount)
                   // // (updatepvrate)
                   // (stopcontents)
                   // (dropcontents)
               );
            }
        }
    }
}
