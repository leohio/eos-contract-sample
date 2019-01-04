#include "pcs.hpp"

using namespace eosio;
using std::string;
using std::vector;

void pcs::create( name issuer, symbol_code sym ) {
	require_auth( get_self() );
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

void pcs::destroy( name claimer, symbol_code sym ) {
    require_auth( get_self() );
    eosio_assert( claimer == get_self(), "only destroy token by contract account" );

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Check if currency with symbol already exists
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exists" );

    /// Create new currency
    currency_table.erase( existing_currency );
}

void pcs::mint_token( name user, symbol_code sym, capi_public_key subkey, name ram_payer ) {
    add_balance( user, sym, ram_payer );

    /// set uri with creator paying for RAM
    tokens.emplace( ram_payer, [&]( auto& data ) {
        data.id = tokens.available_primary_key();
        data.subkey = subkey;
        data.owner = user;
        data.sym = sym;
        data.active = 1;
    });
}

void pcs::issue( name user, symbol_code sym, vector<capi_public_key> subkeys, string memo ) {
	eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token by contract account" );
    eosio_assert( sym.is_valid(), "invalid symbol code" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    /// Ensure have issuer authorization
    require_auth( existing_currency->issuer );

    /// valid quantity
    eosio_assert( subkeys.size() > 0, "must issue positive quantity of NFT" );
    eosio_assert( sym == existing_currency->supply.symbol.code(), "symbol precision mismatch" );

    /// Increase supply
    add_supply( sym, subkeys.size() );

    for (auto const& subkey: subkeys) {
        mint_token( user, sym, subkey, existing_currency->issuer );
    }
}

void pcs::transferid( name from, name to, symbol_code sym, uint64_t id, string memo ) {
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
    sub_balance( from, send_token->sym );
    add_balance( to, send_token->sym, from );
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
    sub_balance( owner, sym );

    /// Lower supply from currency
    sub_supply( sym, 1 );
}

/// ram payer を token 発行者から所有者に変更する
void pcs::setrampayer( name payer, uint64_t token_id ) {
	require_auth( payer );

	/// Ensure token ID exists
	auto payer_token = tokens.find( token_id );
	eosio_assert( payer_token != tokens.end(), "token with specified ID does not exist" );

	/// Ensure payer owns token
	eosio_assert( payer_token->owner == payer, "payer does not own token with specified ID");

	/// Notify payer
	require_recipient( payer );

	/// Set owner as a RAM payer
    const auto& token = *payer_token;
	tokens.modify( payer_token, payer, [&](auto& data) {
		data.id = token.id;
		data.subkey = token.subkey;
		data.owner = token.owner;
		data.sym = token.sym;
        data.active = token.active;
	});

	sub_balance( payer, payer_token->sym );
	add_balance( payer, payer_token->sym, payer );
}

void pcs::refleshkey( name owner, uint64_t token_id, capi_public_key subkey ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "do not reflesh key by contract account" );

    /// Find token to burn
    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( target_token->owner == owner, "token not owned by account" );

    tokens.modify( target_token, owner, [&](auto& data) {
        data.subkey = subkey;
        data.active = 1; /// lock 解除をついでに行う
    });

    require_recipient( owner );
}

void pcs::lock( name accuser, uint64_t token_id, string data, capi_signature sig ) {
    require_auth( accuser );
    eosio_assert( accuser != get_self(), "do not burn token by contract account" );

    auto target_token = tokens.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );

    /// 署名検証
    /// target_token に記載された public key に対応する
    /// private key を知っている人のみがロックの権限をもつ
    capi_public_key pk = target_token->subkey;
    capi_checksum256 digest;
    sha256(&data[0], data.size(), &digest);

    assert_recover_key(&digest, (const char *)&sig, sizeof(sig), (const char *)&pk, sizeof(pk));

    tokens.modify( target_token, accuser, [&](auto& data) {
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

    tokens.modify( target_token, owner, [&]( auto& data ) {
        data.owner = get_self(); /// 所有者をコントラクトアカウントに
    });

    /// Lower balance from owner
    sub_balance( owner, target_token->sym );
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
    auto bid_order = bids.find( token_id );
    eosio_assert( target_token != tokens.end(), "token with id does not exist" );
    eosio_assert( bid_order != bids.end(), "token with id does not exist" );

    /// Check memo size and print
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    bids.erase( bid_order );

    tokens.modify( target_token, buyer, [&]( auto& data ) {
        data.owner = buyer;
        data.active = 0; /// lock until reflesh key for safety
    });

    add_balance( buyer, bid_order->sym, buyer );

    /// 支払い
    sub_deposit( buyer, bid_order->price );

    /// 売り手に送金
    string message = "executed as the inline action in pcs::buy of " + get_self().to_string();
    transfer_eos( bid_order->owner, bid_order->price, message );
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

    add_balance( owner, bid_order->sym, owner );
}

void pcs::receive() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data< transfer_args >();
    name from = transfer_data.from;
    name to = transfer_data.to;
    asset quantity = transfer_data.quantity;
    string message = transfer_data.memo;

    /// Because "transfereos" action is called when this contract account
    /// is the sender or receiver of "eosio.token::transfer",
    /// so `to == get_self()` is needed.
    /// This contract account do not have balance record,
    /// so `from != get_self()` is needed.
    if ( to == get_self() && from != get_self() ) {
        eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "receive only EOS token" );

        auto balance_data = eosbt.find( from.value );

        if( balance_data == eosbt.end() ) {
            eosbt.emplace( get_self(), [&]( auto& data ) {
                data.username = from;
                data.quantity = quantity;
            });
        } else {
            asset deposit = balance_data->quantity;
            eosio_assert( deposit.amount + quantity.amount > deposit.amount, "since occurred overflow, revert the state" );

            /// modify the record of balance_data
            /// ram payer is this contract account
            eosbt.modify( balance_data, get_self(), [&]( auto& data ) {
                data.quantity = deposit + quantity;
            });
        }

        // For example, "buy token#0 in pcstoycashio" match this pattern.
        // std::regex pattern( "buy\\s([A-Z{1,7}])\\stoken#(\\d+)\\sin\\s" + get_self().to_string() );
        // std::smatch sm;
        // if ( std::regex_match( message, sm, pattern ) ) {
        //     // 得られた token symbol を symbol_code に変換
        //     symbol_code sym = symbol_code( sm[1].str() );
        //     // 得られた token ID を uint64 型にキャスト
        //     uint64_t token_id = static_cast<uint64_t>( std::stoull(sm[2]) );
        //
        //     auto bid_order = bids.find( token_id );
        //     eosio_assert( bid_order != bids.end(), "token does not exist" );
        //     eosio_assert( bid_order->sym == sym, "symbol code is not match" );
        //
        //     // 残高が足りているか事前に確認する
        //     // auto buyer_balance = eosbt.find( from.value );
        //     // eosio_assert( buyer_balance != eosbt.end(), "your balance data do not exist" );
        //     //
        //     // asset buyer_deposit = buyer_balance->quantity;
        //     // asset price = bid_order->price;
        //     // eosio_assert( buyer_deposit.symbol == price.symbol, "different symbol or precision mismatch" );
        //     // eosio_assert( buyer_deposit.amount >= price.amount, "token price exceed your balance" );
        //
        //     action(
        //         permission_level{ from, name("active") }, // このアカウントの権限を用いて
        //         get_self(), // このコントラクト内にある
        //         name("buy"), // このメソッドに
        //         std::make_tuple( from, token_id, "buy token as inline action in pcs::receive of " + get_self().to_string() ) // 引数をタプルで渡して
        //     ).send(); // アクションを実行する
        // }
    }
    // else if ( from == get_self() && to != get_self() ) {
    //     asset quantity = transfer_data.quantity;
    //     asset owned_token = ???;
    //     asset deposit_sum = ???;
    //
    //     eosio_assert(
    //         deposit_sum > owned_token - quantity,
    //         "please leave deposit"
    //     );
    // }
}

void pcs::set_uri( name user, symbol_code sym, string uri ) {
    /// Check uri size
    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );

    pv_count_index pv_count_table( get_self(), sym.raw() );

    pv_count_table.emplace( user, [&]( auto& data ) {
        data.id = pv_count_table.available_primary_key();
        data.uri = uri;
        data.count = 0;
    });
}

void pcs::resisteruris( name user, symbol_code sym, vector<string> uris ) {
    require_auth( user );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    for ( auto const& uri: uris ) {
        pcs::set_uri( user, sym, uri );
    }
}

void pcs::setpvid( name claimer, symbol_code sym, uint64_t uri_id, uint64_t count ) {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( claimer );
    eosio_assert( claimer == get_self(), "setpvid action must execute by the contract account" );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// すでに指定した uri に関する pv のデータがあることを確認
    uint64_t symbol_name = sym.raw();
    pv_count_index pv_count_table( get_self(), symbol_name );
    auto pv_data = pv_count_table.find( uri_id );
    eosio_assert( pv_data != pv_count_table.end() ,"this uri is not exist in the pv count table" );

    /// overflow 対策
    eosio_assert( pv_data->count + count > pv_data->count, "pv count overflow! so revert state." );

    pv_count_table.modify( pv_data, claimer, [&]( auto& data ) {
        data.count = data.count + count;
    });
}

void pcs::setpvdata( name claimer, symbol_code sym, string uri, uint64_t count ) {
    uint64_t uri_id = pcs::find_pvdata_by_uri( sym, uri );
    pcs::setpvid( claimer, sym, uri_id, count );
}

void pcs::removepvid( name claimer, symbol_code sym, uint64_t uri_id ) {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( claimer );
    eosio_assert( claimer == get_self(), "removepvid action must execute by the contract account" );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// すでに指定した uri に関する pv のデータがあることを確認
    uint64_t symbol_name = sym.raw();
    pv_count_index pv_count_table( get_self(), symbol_name );
    auto pv_data = pv_count_table.find( uri_id );

    eosio_assert( pv_data != pv_count_table.end() ,"this uri is not exist in the pv count table" );

    pv_count_table.erase( pv_data );
}

void pcs::removepvdata( name claimer, symbol_code sym, string uri ) {
    uint64_t uri_id = pcs::find_pvdata_by_uri( sym, uri );
    pcs::removepvid( claimer, sym, uri_id );
}

void pcs::setoffer( name payer, symbol_code sym, string uri, asset price ) {
    require_auth( payer );

    /// get timestamp
    uint64_t now = current_time();

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// オファー料金が前もってコントラクトに振り込まれているか確認
    auto balance_data = eosbt.find( payer.value );
    eosio_assert( balance_data != eosbt.end(), "your balance data do not exist" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    offer_index offer_list( get_self(), sym.raw() );
    offer_list.emplace( payer, [&]( auto& data ) {
        data.id = offer_list.available_primary_key();
        data.price = price;
        data.payer = payer;
        data.uri = uri;
    });
}

void pcs::acceptoffer( name manager, symbol_code sym, uint64_t offer_id ) {
    require_auth( manager );

    /// get timestamp
    uint64_t now = current_time();

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    // eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    offer_index offer_list( get_self(), sym.raw() );
    auto offer_data = offer_list.find( offer_id );
    eosio_assert( offer_data != offer_list.end(), "offer data do not exist" );

    asset price = offer_data->price;
    name payer = offer_data->payer;
    string uri = offer_data->uri;


    /// オファー料金が前もってコントラクトに振り込まれているか確認
    auto balance_data = eosbt.find( payer.value );
    eosio_assert( balance_data != eosbt.end(), "your balance data do not exist" );

    /// 受け入れられた offer は content に昇華する
    offer_list.erase( offer_data );

    content_index content_list( get_self(), sym.raw() );

    /// コンテンツが増えてきたら、古いものから消去する
    // if ( content_list.size() > 5 ) { // size メンバは存在しないので別の方法を探す
    //     auto oldest_content = content_list.get_index<name("bytimestamp")>().lower_bound(0);
    //     content_list.erase( oldest_content );
    // }

    content_list.emplace( payer, [&]( auto& data ) {
        data.id = content_list.available_primary_key();
        data.price = price;
        data.payer = payer;
        data.uri = uri;
        data.timestamp = now;
        data.active = 1;
    });

    pcs::sub_eos_balance( payer, price );

    name to = existing_currency->issuer;
    string message = "executed as the inline action in pcs::acceptoffer of " + get_self().to_string();
    pcs::transfer_eos( to, price, message );
}

void pcs::removeoffer( name payer, symbol_code sym, uint64_t offer_id ) {
    require_auth( payer );

    offer_index offer_list( get_self(), sym.raw() );
    auto offer_data = offer_list.find( offer_id );
    eosio_assert( offer_data != offer_list.end(), "offer data do not exist" );
    eosio_assert( offer_data->payer == payer, "payer is not of this offer" );

    offer_list.erase( offer_data );

    pcs::sub_eos_balance( payer, offer_data->price );

    string message = "executed as the inline action in pcs::acceptoffer of " + get_self().to_string();
    pcs::transfer_eos( payer, offer_data->price, message );
}

void pcs::stopcontent( name manager, symbol_code sym, uint64_t content_id ) {
    require_auth( manager );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    // eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    content_index content_list( get_self(), sym.raw() );
    auto content_data = content_list.find( content_id );
    content_list.modify( content_data, manager, [&]( auto& data ) {
        data.active = 0;
    });
}

void pcs::dropcontent( name manager, symbol_code sym, uint64_t content_id ) {
    require_auth( manager );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    currency_index currency_table( get_self(), sym.raw() );
    auto existing_currency = currency_table.find( sym.raw() );
    // eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    content_index content_list( get_self(), sym.raw() );
    auto content_data = content_list.find( content_id );
    content_list.erase( content_data );
}

void pcs::sub_eos_balance( name owner, asset quantity ) {
    // eosio_assert( is_account( owner ), "owner account does not exist");

    auto balance_data = eosbt.find( owner.value );
    eosio_assert( balance_data != eosbt.end(), "your balance data do not exist" );

    asset balance = balance_data->quantity;

    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "offer fee must pay EOS token");
    eosio_assert( balance.amount >= quantity.amount, "exceed your balance");

    if ( balance.amount == quantity.amount ) {
        eosbt.erase( balance_data );
    } else {
        // modify the record of balance_data
        // ram payer is this contract account
        eosbt.modify( balance_data, get_self(), [&]( auto& data ) {
            data.quantity = balance - quantity;
        });
    }
}

void pcs::add_eos_balance( name owner, asset quantity, name ram_payer ) {
    // eosio_assert( is_account( owner ), "owner account does not exist");
    // eosio_assert( is_account( ram_payer ), "ram_payer account does not exist");
    eosio_assert( quantity.symbol == symbol( "EOS", 4 ), "offer fee must pay EOS token");

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

void pcs::sub_balance( name owner, symbol_code sym ) {
	account_index account_table( get_self(), owner.value );
    const auto& from = account_table.get( sym.raw(), "no balance object found" );
    eosio_assert( from.balance.amount >= 1, "overdrawn balance" );

    if ( from.balance.amount == 1 ) {
        account_table.erase( from );
    } else {
        asset burnt_supply = asset{ 1, symbol( sym, 0 ) };

        account_table.modify( from, owner, [&]( auto& data ) {
            data.balance -= burnt_supply;
        });
    }
}

void pcs::add_balance( name owner, symbol_code sym, name ram_payer ) {
	account_index account_table( get_self(), owner.value );
    auto to = account_table.find( sym.raw() );
    asset issued_supply = asset{ 1, symbol( sym, 0 ) };

    if ( to == account_table.end() ) {
        account_table.emplace( ram_payer, [&]( auto& data ) {
            data.balance = issued_supply;
        });
    } else {
        account_table.modify( to, get_self(), [&]( auto& data ) {
            data.balance += issued_supply;
        });
    }
}

void pcs::sub_supply( symbol_code sym, uint64_t value ) {
    currency_index currency_table( get_self(), sym.raw() );
    auto current_currency = currency_table.find( sym.raw() );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply -= asset{ static_cast<int64_t>(value), symbol( sym, 0 ) };
    });
}

void pcs::add_supply( symbol_code sym, uint64_t value ) {
    currency_index currency_table( get_self(), sym.raw() );
    auto current_currency = currency_table.find( sym.raw() );

    currency_table.modify( current_currency, get_self(), [&]( auto& data ) {
        data.supply += asset{ static_cast<int64_t>(value), symbol( sym, 0 ) };
    });
}

void pcs::sub_deposit( name user, asset quantity ) {
    auto balance_data = eosbt.find( user.value );
    eosio_assert(balance_data != eosbt.end(), "your balance data do not exist");

    asset deposit = balance_data->quantity;
    eosio_assert(deposit.symbol == quantity.symbol, "different symbol or precision mismatch from your token deposited");
    eosio_assert(deposit.amount >= quantity.amount, "exceed the withdrawable amount");

    if ( deposit.amount == quantity.amount ) {
        eosbt.erase( balance_data );
    } else if ( deposit.amount > quantity.amount ) {
        /// modify the record of balance_data
        /// ram payer is this contract account
        eosbt.modify( balance_data, get_self(), [&]( auto& data ) {
            data.quantity = deposit - quantity;
        });
    }
}

uint64_t pcs::find_own_token( name owner, symbol_code sym ) {
    auto token_table = tokens.get_index<name("bysymbol")>();

    auto it = token_table.lower_bound( sym.raw() );

    bool found = false;
    uint64_t token_id = 0;
    for (; it != token_table.end(); ++it) {
        if( it->sym == sym && it->owner == owner) { // symbol_code
            token_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert(found, "token is not found or is not owned by account");

    return token_id;
}

uint64_t pcs::find_pvdata_by_uri( symbol_code sym, string uri ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    /// Ensure currency has been created
    uint64_t symbol_name = sym.raw();
    currency_index currency_table( get_self(), symbol_name );
    auto existing_currency = currency_table.find( symbol_name );
    eosio_assert( existing_currency != currency_table.end(), "token with symbol does not exist. create token before issue" );

    pv_count_index pv_count_table( get_self(), symbol_name );
    auto pv_table = pv_count_table.get_index<name("byuriid")>();

    /// 0 でいいのかは要検討
    auto it = pv_table.lower_bound(0);

    /// uri でテーブルを検索
    bool found = false;
    uint64_t uri_id = 0;
    for (; it != pv_table.end(); ++it) {
        if( it->uri == uri ) {
            uri_id = it->id;
            found = true;
            break;
        }
    }

    eosio_assert(found, "uri is not found");

    return uri_id;
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
                   (setrampayer)(refleshkey)(lock) /// token
                   (servebid)(buy)(cancelbid) /// bid
                   (resisteruris)(setpvid)(setpvdata)(removepvid)(removepvdata) /// pvcount
                   (setoffer)(acceptoffer)(removeoffer)(stopcontent) /// offer, content
               );
            }
        }
    }
}
