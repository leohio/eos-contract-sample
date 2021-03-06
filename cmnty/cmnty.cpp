/**
 * @environment
 *     eosio.cdt v1.3.2
 * @note NFT standard is basd on https://github.com/unicoeos/eosio.nft .
**/

#include "cmnty.hpp"

using namespace eosio;
using std::string;
using std::vector;

void cmnty::create( name issuer, symbol_code sym ) {
	require_auth( get_self() ); // only create token by contract account
    eosio_assert( issuer != get_self(), "do not create token by contract account" );

	/// Check if issuer account exists
	eosio_assert( is_account( issuer ), "issuer account does not exist");

    eosio_assert( sym != symbol_code("EOS"), "EOS is invalid token symbol");

    /// Valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    _create_token( issuer, sym );
}

// void cmnty::destroy( symbol_code sym ) {
//     require_auth( get_self() ); // only destroy token by contract account
//
//     /// Valid symbol
//     eosio_assert( sym.is_valid(), "invalid symbol name" );
//
//     /// Check if currency with symbol already exists
//     auto& currency_data = currency_table.get( get_self().value, "token with symbol does not exists" );
//     uint64_t num_of_owner = static_cast<uint64_t>(currency_data.supply.amount);
//     eosio_assert( num_of_owner < 2, "who has this token exists except for one manager" );
//
//     /// Delete currency
//     currency_table.erase( currency_data );
//
//     /// Delete manager
//     community_manager_index community_manager_table( get_self(), sym.raw() );
//     auto& community_manager_data = community_manager_table.get( 0, "token#0 does not exist" );
//     community_manager_table.erase( community_manager_data );
// }

void cmnty::issue( name user, asset quantity, string memo ) {
    eosio_assert( is_account( user ), "user account does not exist");
    eosio_assert( user != get_self(), "do not issue token for contract account" );

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure valid quantity
    assert_positive_quantity_of_nft( quantity );
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision mismatch" );

    symbol_code sym = quantity.symbol.code();

    /// Ensure currency has been created
    auto& currency_data = currency_table.get(sym.raw(), "currency info with the symbol does not exist. create currency before issue" );

    /// Ensure have issuer authorization
    name issuer = currency_data.issuer;
    require_auth( issuer );
    eosio_assert( issuer != get_self(), "contract account should not issue token" );

    uint64_t token_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        token_id = _issue_token( user, sym );
    }
}

void cmnty::transferbyid( name from, name to, symbol_code sym, uint64_t token_id, string memo ) {
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( from != get_self(), "do not transfer token from contract account" );
    eosio_assert( to != get_self(), "do not transfer token to contract account" );
    eosio_assert( is_account( to ), "to account does not exist");

	/// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Notify both recipients
    require_recipient( from );
    require_recipient( to );

    _transfer_token( from, to, sym, token_id );
    _lock_token( sym, token_id );
}

void cmnty::transfer( name from, name to, symbol_code sym, string memo ) {
    uint64_t token_id = find_own_token( from, sym ); /// 所持トークンを1つ探す
    string message = "execute in transfer of " + get_self().to_string();
    action(
        permission_level{ from, name("active") },
        get_self(),
        name("transferbyid"),
        std::make_tuple( from, to, sym, token_id, message )
    ).send();

    // eosio_assert( from != to, "cannot transfer to self" );
    // require_auth( from );
    // eosio_assert( from != get_self(), "do not transfer token from contract account" );
    // eosio_assert( to != get_self(), "do not transfer token to contract account" );
    // eosio_assert( is_account( to ), "to account does not exist");
    //
	// /// Check memo size
    // eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
    //
    // /// Notify both recipients
    // require_recipient( from );
    // require_recipient( to );
    //
    // uint64_t token_id = find_own_token( from, sym ); /// 所持トークンを1つ探す
    // _transfer_token( from, to, sym, token_id );
    // _lock_token( sym, token_id );
}

void cmnty::burnbyid( symbol_code sym, uint64_t token_id ) {
    /// Find token to burn
    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );
    name owner = target_token.owner;
    // require_auth( owner );
    // eosio_assert( owner != get_self(), "does not burn token by contract account" );

	_burn_token( owner, sym, token_id );
}

void cmnty::burn( name owner, asset quantity ) {
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    assert_positive_quantity_of_nft( quantity );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist" );

    uint64_t token_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        token_id = find_own_token( owner, sym );
        burnbyid( sym, token_id );
    }
}

/// subkey を変更し lock 解除を行う
void cmnty::refreshkey( symbol_code sym, uint64_t token_id, capi_public_key subkey ) {
    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );
    name owner = target_token.owner;
    require_auth( owner );
    eosio_assert( owner != get_self(), "does not burn token by contract account" );

    _set_subkey( owner, sym, token_id, subkey );
}

// スペルミス
void cmnty::refleshkey( symbol_code sym, uint64_t token_id, capi_public_key subkey ) {
    refreshkey( sym, token_id, subkey );
}

// void cmnty::lock( name claimer, symbol_code sym, uint64_t token_id, string data, capi_signature sig ) {
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
//
//    _lock_token( sym, token_id );
// }

/// add sell order by ID
void cmnty::addsellobyid( symbol_code sym, uint64_t token_id, asset price, string memo ) {
    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );

    name seller = target_token.owner;
    require_auth( seller );
    eosio_assert( seller != get_self(), "contract accmount should not sell token" );

    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );
    // eosio_assert( price >= currency_data.borderprice, "price is lower than border price" );

    assert_non_negative_eos( price );

    _add_sell_order( seller, sym, token_id, price );
}

void cmnty::addsellorder( name seller, asset quantity, asset price, string memo ) {
    require_auth( seller );
    eosio_assert( seller != get_self(), "does not serve bid order by contract account" );

    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    assert_non_negative_eos( price );

    assert_positive_quantity_of_nft( quantity );
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision mismatch" );

    uint64_t token_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        token_id = find_own_token( seller, sym );
        _add_sell_order( seller, sym, token_id, price );
    }
}

void cmnty::issueandsell( asset quantity, asset price, string memo ) {
    symbol_code sym = quantity.symbol.code();
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist. create token before issue" );

    // Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    /// Ensure have issuer authorization
    name issuer = currency_data.issuer;
    require_auth( issuer );

    /// Ensure valid quantity
    assert_positive_quantity_of_nft( quantity );
    eosio_assert( quantity.symbol.precision() == 0, "symbol precision mismatch" );

    uint64_t token_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        token_id = _issue_token( issuer, sym );
        _add_sell_order( issuer, sym, token_id, price );
    }
}

void cmnty::_transfer_eos( name to, asset value, string memo ) {
    eosio_assert( is_account( to ), "to account does not exist" );
    assert_non_negative_eos( value );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    action(
        permission_level{ get_self(), name("active") },
        name("eosio.token"),
        name("transfer"),
        std::make_tuple( get_self(), to, value, memo )
    ).send();
}

void cmnty::buyfromorder( name buyer, symbol_code sym, uint64_t token_id, string memo ) {
    require_auth( buyer );
    eosio_assert( buyer != get_self(), "does not buy token by contract account" );

    /// Check memo size
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );

    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto& sell_order = sell_order_table.get( token_id, "order does not exist" );

    asset price = sell_order.price;
    name seller = sell_order.user;
    eosio_assert( seller != buyer, "buyer should differ from seller" );

    _sub_sell_order( buyer, sym, token_id );
    _lock_token( sym, token_id );

    if ( price.amount > 0 ) {
        string message = "executed as the inline action in buyfromorder of " + get_self().to_string();
        _transfer_eos( seller, price, message );
    }
}

/// cancel sell order by ID
void cmnty::cancelsobyid( symbol_code sym, uint64_t token_id ) {
    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto& sell_order = sell_order_table.get( token_id, "order does not exist" );
    name seller = sell_order.user;
    require_auth( seller );
    eosio_assert( seller != get_self(), "does not cancel bid order by contract account" );

    _sub_sell_order( seller, sym, token_id );
}

/// cancel sell order
void cmnty::cancelsello( name seller, asset quantity ) {
    require_auth( seller );
    eosio_assert( seller != get_self(), "does not cancel sell order by contract account" );

    assert_positive_quantity_of_nft( quantity );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist" );

    uint64_t token_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        token_id = find_own_sell_order( seller, sym );
        cancelsobyid( sym, token_id );
    }
}

/// cancel sell order and burn its token
// void cmnty::cancelsoburn( name seller, asset quantity ) {
//     require_auth( seller );
//     eosio_assert( seller != get_self(), "does not burn token by contract account" );
//
//     assert_positive_quantity_of_nft( quantity );
//
//     /// Ensure currency has been created
//     symbol_code sym = quantity.symbol.code();
//     auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist" );
//
//     uint64_t token_id;
//     for ( uint64_t i = 0; i != quantity.amount; ++i ) {
//         token_id = find_own_sell_order( seller, sym );
//         cancelsobyid( sym, token_id );
//         burnbyid( sym, token_id );
//     }
// }

void cmnty::addbuyorder( name buyer, symbol_code sym, asset price, string memo ) {
    require_auth( buyer );
    eosio_assert( buyer != get_self(), "contract itself should not buy token" );

    /// deposit: buyer -> _self
    uint64_t order_id = _add_buy_order( buyer, sym, price );
}

void cmnty::selltoorder( symbol_code sym, uint64_t token_id, uint64_t order_id, string memo ) {
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );
    name seller = target_token.owner;
    require_auth( seller );
    eosio_assert( seller != get_self(), "contract account should " );

    buy_order_index buy_order_table( get_self(), sym.raw() );
    auto& buy_order_data = buy_order_table.get( order_id, "the buy order does not exist" );

    name buyer = buy_order_data.user;
    eosio_assert( seller != buyer, "seller should differ from buyer" );

    /// deposit: _self -> seller
    _sub_buy_order( seller, sym, order_id );

    /// token: seller -> buyer
    _transfer_token( seller, buyer, sym, token_id );
    _lock_token( sym, token_id );

    // string message = "execute in selltoorder of " + get_self().to_string();
    // action(
    //     permission_level{ seller, name("active") },
    //     get_self(),
    //     name("transferbyid"),
    //     std::make_tuple( seller, buyer, sym, token_id, message )
    // ).send();
}

/// cancel buy order by ID
void cmnty::cancelbobyid( symbol_code sym, uint64_t order_id ) {
    buy_order_index buy_order_table( get_self(), sym.raw() );
    auto& buy_order_data = buy_order_table.get( order_id, "the buy order does not exist" );
    name buyer = buy_order_data.user;

    require_auth( buyer );

    /// deposit: _self -> buyer
    _sub_buy_order( buyer, sym, order_id );
}

/// cancel buy order
void cmnty::cancelbuyo( name buyer, asset quantity ) {
    require_auth( buyer );
    eosio_assert( buyer != get_self(), "does not cancel buy order by contract account" );

    assert_positive_quantity_of_nft( quantity );

    /// Ensure currency has been created
    symbol_code sym = quantity.symbol.code();
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist" );

    uint64_t order_id;
    for ( uint64_t i = 0; i != quantity.amount; ++i ) {
        order_id = find_own_buy_order( buyer, sym );
        cancelbobyid( sym, order_id );
    }
}

/// manager 権限を剥奪したいときは、 ratio を 0 にする
void cmnty::setmanager( symbol_code sym, uint64_t manager_token_id, vector<uint64_t> token_id_list, vector<uint16_t> weight_list, uint16_t others_weight ) {
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist. create token before issue" );

    name issuer = currency_data.issuer;
    require_auth( issuer );

    community_manager_index community_manager_table( get_self(), sym.raw() );
    auto& community_data_of_manager = community_manager_table.get( manager_token_id, "you are not community manager" );

    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( manager_token_id, "token with symbol does not exist" );
    name manager = token_data.owner;
    require_auth( manager );

    uint64_t token_id_list_length = static_cast<uint64_t>( token_id_list.size() );
    uint64_t weight_list_length = static_cast<uint64_t>( weight_list.size() );
    eosio_assert( token_id_list_length == weight_list_length, "list size is invalid" );

    uint64_t sum_of_weight = others_weight;
    for ( uint64_t i = 0; i != token_id_list_length; ++i ) {
        auto& token_data = token_table.get( token_id_list[i], "token with id is not found" );

        eosio_assert( sum_of_weight + weight_list[i] >= sum_of_weight, "occur overflow" ); // check overflow
        sum_of_weight += weight_list[i];
    }

    eosio_assert( sum_of_weight > 0, "sum of weight must be positive" );

    uint64_t num_of_manager = currency_data.numofmanager;
    int64_t weight_diff = 0;
    for ( uint64_t j = 0; j != token_id_list_length; ++j ) {
        auto community_data = community_manager_table.find( token_id_list[j] );
        if ( community_data == community_manager_table.end() ) {
            if (weight_list[j] > 0) {
                num_of_manager += 1;

                eosio_assert( weight_diff + weight_list[j] > weight_diff, "occur overflow" );
                weight_diff += weight_list[j];

                community_manager_table.emplace( get_self(), [&]( auto& data ) {
                    data.token_id = token_id_list[j];
                    data.weight = weight_list[j];
                });
            }
        } else {
            uint16_t old_weight = community_data->weight;
            if (weight_list[j] > 0) {
                eosio_assert( weight_diff + weight_list[j] > weight_diff, "occur overflow" );
                weight_diff += weight_list[j];
                eosio_assert( weight_diff - old_weight <= weight_diff, "occur underflow" );
                weight_diff -= old_weight;

                community_manager_table.modify( community_data, get_self(), [&]( auto& data ) {
                    data.weight = weight_list[j];
                });
            } else {
                num_of_manager -= 1;

                eosio_assert( weight_diff - old_weight <= weight_diff, "occur underflow" );
                weight_diff -= old_weight;

                community_manager_table.erase( community_data );
            }
        }
    }

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.numofmanager = num_of_manager;
        data.others_reward_weight = others_weight;
        data.sum_of_reward_weight = sum_of_weight;
    });
}

void cmnty::moveeos( name from, name to, asset quantity, string memo ) {
    if ( to == get_self() && from != get_self() ) {
        _add_deposit( from, quantity );

        /// TODO: transfer しながら buy するなどは、
        /// トランザクションに 2つのアクションを入れればいいだけなので
        /// この部分はおそらく不要になる。
        /// scatterJS のせいでなぜかできない
        /// もしかしてできる？
        vector<string> sbc = split_by_space( memo );
        string contract_name = get_self().to_string();

        if ( sbc[0] == contract_name && sbc[1] == "buyfromorder" && sbc.size() == 4 ) {
            symbol_code sym = symbol_code( sbc[2] );
            uint64_t token_id = static_cast<uint64_t>( std::stoull(sbc[3]) );
            string message = "buy token in moveeos of " + contract_name;
            buyfromorder( from, sym, token_id, message );
        } else if ( sbc[0] == contract_name && sbc[1] == "addbuyorder" && sbc.size() == 3 ) {
            symbol_code sym = symbol_code( sbc[2] );
            string message = "add sell order in moveeos of " + contract_name;
            addbuyorder( from, sym, quantity, message );
        } else if ( sbc[0] == contract_name && sbc[1] == "setoffer" && sbc.size() == 5 ) {
            symbol_code sym = symbol_code( sbc[2] );
            string uri = sbc[3];
            string short_title = sbc[4];
            string message = "set offer in moveeos of " + contract_name;
            setoffer( from, sym, uri, short_title, quantity, message );
        }
    } else if ( from == get_self() && to != get_self() ) {
        /// 預金総額を取得
        asset total_amount( 0, quantity.symbol ); /// quantity.symbol is symbol("EOS", 4)
        auto total_deposit = total_deposit_table.find( quantity.symbol.code().raw() );
        if ( total_deposit != total_deposit_table.end() ) {
            total_amount = total_deposit->balance;
        }

        /// _self の EOS 残高を取得
        asset current_amount( 0, quantity.symbol );
        account_index account_table( get_code(), get_self().value ); /// get_code() is name("eosio.token")
        auto account_data = account_table.find( quantity.symbol.code().raw() );
        if ( account_data != account_table.end() ) {
            current_amount = account_data->balance;
        }

        /// 残高が預金総額よりも減らないように監視する
        eosio_assert( total_amount <= current_amount, "this contract must leave at least total deposit" );
    }
}

void cmnty::withdraw( name user, asset value, string memo ) {
    require_auth( user );
    _sub_deposit( user, value );

    string message = "execute as inline action in withdraw of " + get_self().to_string();
    _transfer_eos( user, value, message );
}

void cmnty::setoffer( name provider, symbol_code sym, string uri, string short_title, asset price, string memo ) {
    require_auth( provider );

    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );
    eosio_assert( short_title.size() <= 64, "short title more than 64 bytes");

    assert_non_negative_eos( price );

    eosio_assert( !find_offer_by_uri( sym, uri ), "already proposed this offer" );
    eosio_assert( !find_offer_by_title( sym, short_title ), "already proposed this title" );
    eosio_assert( !find_contents_by_uri( sym, uri ), "already existed the uri" );
    eosio_assert( !find_contents_by_title( sym, short_title ), "already existed the title" );

    /// オファー料金が前もってコントラクトに振り込まれているか確認
    if ( price.amount > 0 ) {
        deposit_index deposit_table( get_self(), provider.value );
        auto& deposit_data = deposit_table.get( price.symbol.code().raw(), "your balance data do not exist" );

        _sub_deposit( provider, price );
    }

    /// Ensure currency has been created

    offer_index offer_list( get_self(), sym.raw() );
    uint64_t offer_id = offer_list.available_primary_key();
    offer_list.emplace( get_self(), [&]( auto& data ) {
        data.id = offer_id;
        data.price = price;
        data.provider = provider;
        data.uri = uri;
        data.short_title = short_title;
    });
}

void cmnty::acceptoffer( symbol_code sym, uint64_t manager_token_id, uint64_t offer_id ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto& currency_data = currency_table.get( sym.raw(), "currency info with the symbol does not exist." );

    community_manager_index community_manager_table( get_self(), sym.raw() );
    auto& community_data_of_manager = community_manager_table.get( manager_token_id, "you are not community manager" );

    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( manager_token_id, "token with symbol does not exist" );
    name manager = token_data.owner;
    require_auth( manager );

    offer_index offer_list( get_self(), sym.raw() );
    auto& offer_data = offer_list.get( offer_id, "offer data do not exist" );

    asset price = offer_data.price;
    name provider = offer_data.provider;
    string uri = offer_data.uri;
    string short_title = offer_data.short_title;

    assert_non_negative_eos( price );
    eosio_assert( uri.size() <= 256, "uri has more than 256 bytes" );

    /// 受け入れられた offer は content に昇格する
    offer_list.erase( offer_data );

    uint64_t now = current_time();

    _add_contents( manager, sym, price, provider, uri, short_title, now );
    _update_pv_rate( sym, now, price );

    require_recipient( provider );

    if ( price.amount > 0 ) {
        /// 割り切れないときや、DEX に出しているときや、
        /// num_of_owner と num_of_manager が一致するときは、
        /// コントラクトが others_ratio 分をもらう

        int64_t num_of_owner = currency_data.supply.amount;
        int64_t num_of_manager = static_cast<int64_t>(currency_data.numofmanager);
        int64_t sum_of_weight = static_cast<int64_t>(currency_data.sum_of_reward_weight);
        int64_t others_weight = static_cast<int64_t>(currency_data.others_reward_weight);
        int64_t num_of_others = static_cast<int64_t>(num_of_owner - num_of_manager);

        token_index token_table( get_self(), sym.raw() );
        for ( auto it = token_table.begin(); it != token_table.end(); ++it ) {
            name owner = it->owner;
            // string message = "executed as the inline action in acceptoffer of " + get_self().to_string();

            auto community_data = community_manager_table.find( it->id );
            if ( community_data == community_manager_table.end() ) {
                /// トークン管理者でなければ、 price * others_reward_ratio をトークン所持者で均等に分配
                asset reward = price * others_weight / num_of_others / sum_of_weight;
                if ( reward.amount > 0 ) {
                    _add_deposit( owner, reward );
                }
            } else {
                /// トークン管理者でなければ、 price * reward_ratio を分配
                int64_t weight = static_cast<int64_t>(community_data->weight);
                asset reward = price * weight / sum_of_weight;
                if ( reward.amount > 0 ) {
                    _add_deposit( owner, reward );
                }
            }
        }
    }
}

void cmnty::contentsname( symbol_code sym, uint64_t contents_id, string short_title ) {
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto& currency_data = currency_table.get( sym.raw(), "currency info with the symbol does not exist." );

    contents_index contents_table( get_self(), sym.raw() );
    auto& contents_data = contents_table.get( contents_id, "the contents does not exist" );
    eosio_assert( short_title.size() <= 64, "short title more than 64 bytes");

    require_auth( contents_data.provider );

    if ( contents_data.short_title != short_title ) {
        contents_table.erase( contents_data );
        contents_table.emplace( contents_data.provider, [&]( auto& data ) {
            data = contents_data;
            data.short_title = short_title;
        });
    }
}

void cmnty::rejectoffer( symbol_code sym, uint64_t manager_token_id, uint64_t offer_id, string memo ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    community_manager_index community_manager_table( get_self(), sym.raw() );
    auto& community_data_of_manager = community_manager_table.get( manager_token_id, "you are not community manager" );

    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( manager_token_id, "token with symbol does not exist" );
    name manager = token_data.owner;
    require_auth( manager );

    offer_index offer_list( get_self(), sym.raw() );
    auto& offer_data = offer_list.get( offer_id, "offer data do not exist" );

    name provider = offer_data.provider;

    /// 拒否された offer はテーブルから消される
    offer_list.erase( offer_data );

    require_recipient( provider );
}

void cmnty::removeoffer( name provider, symbol_code sym, uint64_t offer_id ) {
    require_auth( provider );

    offer_index offer_list( get_self(), sym.raw() );
    auto& offer_data = offer_list.get( offer_id, "offer data do not exist" );
    eosio_assert( offer_data.provider == provider, "you are not provider of this offer" );

    offer_list.erase( offer_data );

    string message = "executed as the inline action in removeoffer of " + get_self().to_string();
    _transfer_eos( provider, offer_data.price, message );
}

void cmnty::addpvcount( vector<pv_data_str> pv_data_list ) {
    /// コントラクトアカウントのみが呼び出せる
    require_auth( get_self() );

    for (auto pv_data: pv_data_list) {
        symbol_code sym = pv_data.sym;
        uint64_t contents_id = pv_data.contents_id;
        uint64_t pv_count = pv_data.pv_count;
        _add_pv_count( sym, contents_id, pv_count );
    }
}

void cmnty::_add_pv_count( symbol_code sym, uint64_t contents_id, uint64_t pv_count ) {
    /// すでに指定した contents に関する pv のデータがあることを確認
    contents_index contents_table( get_self(), sym.raw() );
    auto& contents_data = contents_table.get( contents_id, "this contents is not exist in the contents table" );
    eosio_assert( contents_data.active != 0, "this contents is not active" );

    contents_table.modify( contents_data, get_self(), [&]( auto& data ) {
        data.pvcount += pv_count;
    });

    /// get block timestamp
    uint64_t now = current_time();

    auto& global_data = global_table.get( 0, "global data does not exist" );
    global_table.modify( global_data, get_self(), [&]( auto& data ) {
        data.last_modified = now;
        data.pvcount += pv_count;
    });

    auto& currency_data = currency_table.get( sym.raw(), "this currency does not exist" );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.pvcount += pv_count;
    });

    get_border_price( sym );
}

// void cmnty::resetpvcount( uint64_t contents_id ) {
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

// void cmnty::stopcontent( name manager, uint64_t contents_id ) {
//     require_auth( manager );
//
//     auto contents_data = contents_table.find( contents_id );
//     contents_table.modify( contents_data, manager, [&]( auto& data ) {
//         data.active = 0;
//     });
// }

// void cmnty::startcontent( name manager, uint64_t contents_id ) {
//     require_auth( manager );
//
//     auto contents_data = contents_table.find( contents_id );
//     contents_table.modify( contents_data, manager, [&]( auto& data ) {
//         data.active = 1;
//     });
// }

void cmnty::dropcontent( symbol_code sym, uint64_t manager_token_id, uint64_t contents_id ) {
    community_manager_index community_manager_table( get_self(), sym.raw() );
    auto& community_data_of_manager = community_manager_table.get( manager_token_id, "you are not community manager" );

    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( manager_token_id, "token with symbol does not exist" );
    name manager = token_data.owner;
    require_auth( manager );

    contents_index contents_table( get_self(), sym.raw() );
    auto& contents_data = contents_table.get( contents_id, "this offer does not exist" );
    contents_table.erase( contents_data );
}

void cmnty::_create_token( name issuer, symbol_code sym ) {
    /// Check if currency with symbol already exists
    auto currency_data = currency_table.find( sym.raw() );
    eosio_assert( currency_data == currency_table.end(), "currency info with the symbol already exists" );

    uint64_t now = current_time();
    /// Create new currency
    currency_table.emplace( get_self(), [&]( auto& data ) {
        data.supply = asset{ 0, symbol(sym, 0) };
        data.issuer = issuer;
        data.established = now;
        data.borderprice = asset{ 1, symbol("EOS", 4) };
        data.pvcount = 0;
        data.numofmanager = 1;
        data.others_reward_weight = 0;
        data.sum_of_reward_weight = 1;
        data.active = 1;
    });

    /// Issue token to issuer
    uint64_t token_id = _issue_token( issuer, sym );

    /// Resister community manager
    community_manager_index community_manager_table( get_self(), sym.raw() );
    community_manager_table.emplace( get_self(), [&]( auto& data ) {
        data.token_id = token_id;
        data.weight = 1;
    });
}

/// subkey を登録しないでトークン発行
uint64_t cmnty::_issue_token( name to, symbol_code sym ) {
    /// Ensure currency has been created
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist. create token before issue" );

    name issuer = currency_data.issuer;
    asset quantity( 1, symbol(sym, 0) );

    _add_supply( quantity );
    _add_balance( to, quantity, get_self() );
    // _increment_member( to, sym );

    token_index token_table( get_self(), sym.raw() );
    uint64_t token_id = token_table.available_primary_key();
    token_table.emplace( get_self(), [&]( auto& data ) {
        data.id = token_id;
        // subkey は初期値のまま
        data.owner = to;
        data.active = 0;
    });

    return token_id;
}

void cmnty::_transfer_token( name from, name to, symbol_code sym, uint64_t id ) {
    /// Ensure token ID exists
    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( id, "token with specified ID does not exist" );

    /// Ensure owner owns token
    eosio_assert( token_data.owner == from, "sender does not own token with specified ID");

    asset quantity( 1, symbol(sym, 0) );

    if ( to != get_self() ) {
        // _increment_member( to, sym );
        _add_balance( to, quantity , from );
    }

    /// Transfer NFT from sender to receiver
    token_table.modify( token_data, from, [&]( auto& data ) {
        data.owner = to;
    });

    if ( from != get_self() ) {
        // _decrement_member( from, sym );
        _sub_balance( from, quantity );
    }
}

void cmnty::_burn_token( name owner, symbol_code sym, uint64_t token_id ) {
    token_index token_table( get_self(), sym.raw() );
    auto& token_data = token_table.get( token_id, "token with id does not exist" );

    // community_manager_index community_manager_table( get_self(), sym.raw() );
    // auto community_data = community_manager_table.find( token_id );
    // eosio_assert( community_data == community_manager_table.end(), "manager should not delete my token" );

    /// Remove token from tokens table
    token_table.erase( token_data );

    // _decrement_member( owner, sym );

    asset quantity( 1, symbol(sym, 0) );
    _sub_balance( owner, quantity );
    _sub_supply( quantity );
}

void cmnty::_set_subkey( name owner, symbol_code sym, uint64_t token_id, capi_public_key subkey ) {
    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );
    eosio_assert( target_token.owner == owner, "token not owned by account" );

    token_table.modify( target_token, owner, [&](auto& data) {
        data.id = token_id;
        data.subkey = subkey;
        data.owner = owner;
        data.active = 1;
    });

    /// RAM を owner に負担させる
    asset quantity( 1, symbol(sym, 0) );
    _sub_balance( owner, quantity );
	_add_balance( owner, quantity, owner );
}

void cmnty::_lock_token( symbol_code sym, uint64_t token_id ) {
    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );

    token_table.modify( target_token, get_self(), [&]( auto& data ) {
        data.active = 0;
    });
}

void cmnty::_add_sell_order( name from, symbol_code sym, uint64_t token_id, asset price ) {
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol do not exists" );

    token_index token_table( get_self(), sym.raw() );
    auto& target_token = token_table.get( token_id, "token with id does not exist" );

    assert_non_negative_eos( price );

    asset border_price = get_border_price( sym );
    eosio_assert( price >= border_price, "price is lower than border price" );

    _transfer_token( from, get_self(), sym, token_id );
    // string message = "execute in add_sell_order of " + get_self().to_string();
    // action(
    //     permission_level{ from, name("active") },
    //     get_self(),
    //     name("transferbyid"),
    //     std::make_tuple( from, get_self(), sym, token_id, message )
    // ).send();

    sell_order_index sell_order_table( get_self(), sym.raw() );
    sell_order_table.emplace( from, [&]( auto& data ) {
        data.id = token_id;
        data.price = price;
        data.user = from;
    });
}

void cmnty::_sub_sell_order( name to, symbol_code sym, uint64_t token_id ) {
    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto& sell_order = sell_order_table.get( token_id, "order does not exist" );

    _transfer_token( get_self(), to, sym, token_id );
    // string message = "execute in _sub_sell_order of " + get_self().to_string();
    // action(
    //     permission_level{ from, name("active") },
    //     get_self(),
    //     name("transferbyid"),
    //     std::make_tuple( get_self(), to, sym, token_id, message )
    // ).send();

    sell_order_table.erase( sell_order );
}

uint64_t cmnty::_add_buy_order( name from, symbol_code sym, asset price ) {
    assert_non_negative_eos( price );

    auto& currency_data = currency_table.get( sym.raw(), "token with symbol do not exists" );

    assert_non_negative_eos( price );

    asset border_price = get_border_price( sym );
    eosio_assert( price >= border_price, "price is lower than border price" );

    _sub_deposit( from, price );

    buy_order_index buy_order_table( get_self(), sym.raw() );
    uint64_t order_id = buy_order_table.available_primary_key();
    buy_order_table.emplace( get_self(), [&]( auto& data ) {
        data.id = order_id;
        data.price = price;
        data.user = from;
    });

    return order_id;
}

void cmnty::_sub_buy_order( name to, symbol_code sym, uint64_t order_id ) {
    buy_order_index buy_order_table( get_self(), sym.raw() );
    auto& buy_order_data = buy_order_table.get( order_id, "the buy order does not exist" );

    _add_deposit( to, buy_order_data.price );

    buy_order_table.erase( buy_order_data );
}

void cmnty::_update_pv_rate( symbol_code sym, uint64_t timestamp, asset new_offer_price ) {
    assert_non_negative_eos( new_offer_price );

    eosio_assert( timestamp != 0, "invalid timestamp" );

    auto& currency_data = currency_table.get( sym.raw(), "this currency does not exist" );
    uint64_t cmnty_pv_count = currency_data.pvcount;

    auto& global_data = global_table.get( 0, "global data does not exist" );
    uint64_t global_pv_count = global_data.pvcount;
    float_t last_pv_rate = global_data.pvrate;
    float_t pv_rate = last_pv_rate;

    // pvrate を維持するためには new_offer_price.amount が last_pv_rate * cmnty_pv_count である必要がある。
    // これより低い金額での offer を出されるこは、そのコミュニティが期待されていないことを示し、
    // 逆に、これより高い金額での offer を出されることは、そのコミュニティが期待されていることを示す。
    if ( global_pv_count + cmnty_pv_count != 0 ) {
        pv_rate = (last_pv_rate * global_pv_count + new_offer_price.amount) / (global_pv_count + cmnty_pv_count);
    }

    // auto pv_rate_data = pv_rate_table.find( timestamp );
    // if ( pv_rate_data == pv_rate_table.end() ) {
    //     pv_rate_table.emplace( get_self(), [&]( auto& data ) {
    //         data.timestamp = timestamp;
    //         data.rate = pv_rate;
    //     });
    // } else {
    //     pv_rate_table.modify( pv_rate_data, get_self(), [&]( auto& data ) {
    //         data.rate = pv_rate;
    //     });
    // }

    global_table.modify( global_data, get_self(), [&]( auto& data ) {
        data.last_modified = timestamp;
        data.pvrate = pv_rate;
    });
}

asset cmnty::get_cmnty_offer_reward( symbol_code sym, uint64_t ago ) {
    contents_index contents_table( get_self(), sym.raw() );
    auto table = contents_table.get_index<name("bytime")>();

    // table は accepted の小さい順に並んでいるので、 ago 以降であるものを見れば良い
    auto it = table.lower_bound( ago );

    asset total_offer_reward = asset{ 0, symbol("EOS", 4) };

    for (; it != table.end(); ++it ) {
        if ( it->price.symbol == symbol("EOS", 4) ) {
            total_offer_reward += it->price; /// asset 型では overflow しているかどうかを演算のたびに判断している
        }
    }

    return total_offer_reward;
}

asset cmnty::get_border_price( symbol_code sym ) {
    auto& currency_data = currency_table.get( sym.raw(), "this currency does not exist" );
    uint64_t cmnty_pv_count = currency_data.pvcount;


    // float_t last_pv_rate = 0;
    // auto latest_pv_rate_data = pv_rate_table.rbegin(); // 最後の要素を取得する
    // if ( latest_pv_rate_data != pv_rate_table.rend() ) {
    //     last_pv_rate = latest_pv_rate_data->rate;
    // }


    auto& global_data = global_table.get( 0, "global data deos not exist" );
    float_t last_pv_rate = global_data.pvrate;

    /// now はマイクロミリ秒単位
    uint64_t now = current_time();

    /// 1か月前までの offer の報酬合計
    uint64_t a_month = 30ll*24*60*60*1000000;
    asset total_offer_reward_in_a_month = get_cmnty_offer_reward( sym, now - a_month );

    /// 今までの PV の合計
    uint64_t total_pv_count = currency_data.pvcount;

    float_t others_ratio = static_cast<float_t>(currency_data.others_reward_weight) / currency_data.sum_of_reward_weight;
    uint64_t num_of_owner = static_cast<uint64_t>(currency_data.supply.amount);
    uint64_t number_of_others = num_of_owner - currency_data.numofmanager;
    // an_year = 365ll*24*60*60*1000000;

    int64_t expected_offer_in_a_year = total_offer_reward_in_a_month.amount * 12 * others_ratio / number_of_others;
    uint64_t expected_pv_in_a_year = total_pv_count * last_pv_rate * a_month / (now - currency_data.established);

    int64_t border_price_amount = expected_offer_in_a_year + expected_pv_in_a_year;
    eosio_assert( border_price_amount >= expected_offer_in_a_year, "occur overflow" );

    asset border_price = asset{ border_price_amount, total_offer_reward_in_a_month.symbol }; /// total_offer_reward_in_a_month.symbol is symbol("EOS", 4)
    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.borderprice = border_price;
    });

    return border_price;
}

void cmnty::_add_contents( name ram_payer, symbol_code sym, asset price, name provider, string uri, string short_title, uint64_t timestamp ) {
    assert_non_negative_eos( price );

    contents_index contents_table( get_self(), sym.raw() );
    uint64_t new_contents_id = contents_table.available_primary_key();
    contents_table.emplace( ram_payer, [&]( auto& data ) {
        data.id = new_contents_id;
        // data.sym = sym;
        data.price = price;
        data.provider = provider;
        data.uri = uri;
        data.pvcount = 0;
        data.accepted = timestamp;
        data.short_title = short_title;
        data.active = 1;
    });

    auto table = contents_table.get_index<name("bytime")>();

    // reverse iterator の最初の row を取得
    auto rit = table.rbegin();

    // rit を20つ進める
    uint8_t counter = 0;
    bool found = false;
    for (; rit != table.rend(); ++rit ) {
        counter++;
        if ( counter == 20 ) {
            found = true;
            break;
        }
    }

    // contents ID が20つ前のものを消去する
    if ( found ) {
        auto& contents_data = *rit;
        contents_table.erase( contents_data );
    }
}

void cmnty::_add_balance( name owner, asset quantity, name ram_payer ) {
    assert_positive_quantity_of_nft( quantity );

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

void cmnty::_sub_balance( name owner, asset quantity ) {
    assert_positive_quantity_of_nft( quantity );

    account_index account_table( get_self(), owner.value );
    auto& account_data = account_table.get( quantity.symbol.code().raw(), "no balance object found" );

    eosio_assert( account_data.balance >= quantity, "overdrawn balance" );

    if ( account_data.balance == quantity ) {
        account_table.erase( account_data );
    } else {
        account_table.modify( account_data, owner, [&]( auto& data ) {
            data.balance -= quantity;
        });
    }
}

void cmnty::_add_supply( asset quantity ) {
    assert_positive_quantity_of_nft( quantity );

    symbol_code sym = quantity.symbol.code();
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.supply += quantity;
    });
}

void cmnty::_sub_supply( asset quantity ) {
    assert_positive_quantity_of_nft( quantity );

    symbol_code sym = quantity.symbol.code();
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );
    eosio_assert( currency_data.supply >= quantity, "exceed total supply" );

    currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
        data.supply -= quantity;
    });
}

void cmnty::_add_deposit( name owner, asset value ) {
    eosio_assert( is_account( owner ), "owner account does not exist" );
    assert_non_negative_eos( value );

    deposit_index deposit_table( get_self(), owner.value );
    auto deposit_data = deposit_table.find( value.symbol.code().raw() ); /// value.symbol is symbol("EOS", 4)
    if( deposit_data == deposit_table.end() ) {
        deposit_table.emplace( get_self(), [&]( auto& data ) {
            data.balance = value;
        });
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance += value;
        });
    }

    auto total_deposit = total_deposit_table.find( value.symbol.code().raw() );
    if ( total_deposit == total_deposit_table.end() ) {
        total_deposit_table.emplace( get_self(), [&]( auto& data ) {
            data.balance = value;
        });
    } else {
        total_deposit_table.modify( total_deposit, get_self(), [&]( auto& data ) {
            data.balance += value;
        });
    }
}

void cmnty::_sub_deposit( name owner, asset value ) {
    eosio_assert( is_account( owner ), "owner account does not exist" );
    assert_non_negative_eos( value );

    deposit_index deposit_table( get_self(), owner.value );
    auto& deposit_data = deposit_table.get( value.symbol.code().raw(), "your balance data do not exist" );

    eosio_assert( deposit_data.balance >= value, "exceed the withdrawable amount" );

    if ( deposit_data.balance == value ) {
        deposit_table.erase( deposit_data );
    } else {
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance -= value;
        });
    }

    auto& total_deposit = total_deposit_table.get( value.symbol.code().raw(), "total deposit data do not exist" );
    eosio_assert( total_deposit.balance >= value, "can not subtract more than total deposit" );

    if ( total_deposit.balance == value ) {
        total_deposit_table.erase( total_deposit );
    } else {
        total_deposit_table.modify( total_deposit, get_self(), [&]( auto& data ) {
            data.balance -= value;
        });
    }
}

/// トークンを増やす前に確認する
// void cmnty::_increment_member( name user, symbol_code sym ) {
//     token_index token_table( get_self(), sym.raw() );
//     bool found = false;
//     for ( auto it = token_table.begin(); it != token_table.end(); ++it ) {
//         if ( it->owner == user ) {
//             found = true;
//             break;
//         }
//     }
//
//     if ( !found ) {
//         auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );
//         currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
//             data.numofmember += 1;
//         });
//     }
// }

/// トークンを減らした後に確認する
// void cmnty::_decrement_member( name user, symbol_code sym ) {
//     bool found = false;
//
//     token_index token_table( get_self(), sym.raw() );
//     for ( auto it = token_table.begin(); it != token_table.end(); ++it ) {
//         if ( it->owner == user ) {
//             found = true;
//             break;
//         }
//     }
//
//     if ( !found ) {
//         auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );
//         currency_table.modify( currency_data, get_self(), [&]( auto& data ) {
//             data.numofmember -= 1;
//         });
//     }
// }

void cmnty::assert_positive_quantity_of_nft( asset quantity ) {
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.symbol.precision() == 0, "quantity must be a whole number" );
    eosio_assert( quantity.amount > 0, "must be positive quantity of NFT" );
}

void cmnty::assert_non_negative_eos( asset price ) {
    eosio_assert( price.symbol == symbol("EOS", 4), "symbol of price must be EOS" );
    eosio_assert( price.amount >= 0, "offer fee must be not negative value" );
}

uint64_t cmnty::find_own_token( name owner, symbol_code sym ) {
    eosio_assert( is_account( owner ), "invalid account" );

    token_index token_table( get_self(), sym.raw() );
    auto table = token_table.get_index<name("byowner")>();
    auto it = table.lower_bound( owner.value );
    eosio_assert( it->owner == owner, "token is not found or is not owned by account" );

    uint64_t token_id = it->id;

    return token_id;
}

uint64_t cmnty::find_own_sell_order( name owner, symbol_code sym ) {
    sell_order_index sell_order_table( get_self(), sym.raw() );
    auto table = sell_order_table.get_index<name("byowner")>();
    auto it = table.lower_bound( owner.value );
    eosio_assert( it->user == owner, "sell order is not found or is not owned by account" );

    uint64_t token_id = it->id;
    return token_id;
}

uint64_t cmnty::find_own_buy_order( name owner, symbol_code sym ) {
    buy_order_index buy_order_table( get_self(), sym.raw() );
    auto table = buy_order_table.get_index<name("byowner")>();
    auto it = table.lower_bound( owner.value );
    eosio_assert( it->user == owner, "buy order is not found or is not owned by account" );

    uint64_t order_id = it->id;
    return order_id;
}

bool cmnty::find_offer_by_uri( symbol_code sym, string uri ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    bool found = false;
    uint64_t offer_id = 0;

    /// uri でテーブルを検索
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    offer_index offer_table( get_self(), sym.raw() );
    auto it = offer_table.begin();
    for (; it != offer_table.end(); ++it ) {
        if ( it->uri == uri ) {
            offer_id = it->id;
            found = true;
            break;
        }
    }

    return found;
}

bool cmnty::find_offer_by_title( symbol_code sym, string short_title ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    bool found = false;
    uint64_t offer_id = 0;

    /// uri でテーブルを検索
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    offer_index offer_table( get_self(), sym.raw() );
    auto it = offer_table.begin();
    for (; it != offer_table.end(); ++it ) {
        if ( it->short_title == short_title ) {
            offer_id = it->id;
            found = true;
            break;
        }
    }

    return found;
}

bool cmnty::find_contents_by_uri( symbol_code sym, string uri ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    bool found = false;
    uint64_t contents_id = 0;

    /// uri でテーブルを検索
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    contents_index contents_table( get_self(), sym.raw() );
    auto it = contents_table.begin();
    for (; it != contents_table.end(); ++it ) {
        if ( it->uri == uri ) {
            contents_id = it->id;
            found = true;
            break;
        }
    }

    return found;
}

bool cmnty::find_contents_by_title( symbol_code sym, string short_title ) {
    /// Ensure valid symbol
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    bool found = false;
    uint64_t contents_id = 0;

    /// uri でテーブルを検索
    auto& currency_data = currency_table.get( sym.raw(), "token with symbol does not exist." );

    contents_index contents_table( get_self(), sym.raw() );
    auto it = contents_table.begin();
    for (; it != contents_table.end(); ++it ) {
        if ( it->short_title == short_title ) {
            contents_id = it->id;
            found = true;
            break;
        }
    }

    return found;
}

// uint64_t cmnty::get_cmnty_pv_count( symbol_code sym, uint64_t ago ) {
//     cmnty_pv_count_index cmnty_pv_count_table( get_self(), sym.raw() );
//
//     // table は timestamp の小さい順に並んでいるので、 ago 以降であるものを見れば良い
//     auto it = cmnty_pv_count_table.lower_bound( ago );
//
//     uint64_t total_pv_count = 0;
//
//     for (; it != cmnty_pv_count_table.end(); ++it ) {
//         eosio_assert( total_pv_count + it->count >= total_pv_count, "overflow occurred" );
//         total_pv_count += it->count;
//     }
//
//     return total_pv_count;
// }

// uint64_t cmnty::get_global_pv_count() {
//     uint64_t global_pv_count = 0;
//
//     /// あるいは過去30日分だけを取り出す
//     // auto it = global_pv_count_table.lower_bound( static_cast<uint64_t>( current_time() - 30*24*60*60 ) );
//     auto it = global_pv_count_table.lower_bound( 0 );
//     for (; it != global_pv_count_table.end(); ++it ) {
//         global_pv_count += it->count;
//     }
//     return global_pv_count;
// }

vector<string> cmnty::split_by_space( string str ) {
    eosio_assert( str.size() <= 256, "too long str" );
    const char* c = str.c_str();
    vector<char> char_list;
    string segment;
    vector<string> split_list;

    for ( uint8_t i = 0; c[i]; ++i ) {
        eosio_assert( ' ' <= c[i] && c[i] <= '~', "str only contains between 0x20 and 0x7e" );

        if ( c[i] == ' ' ) {
            split_list.push_back( string( char_list.begin(), char_list.end() ) );
            char_list.clear();
        } else {
            char_list.push_back( c[i] );
        }
    }

    split_list.push_back( string( char_list.begin(), char_list.end() ) );

    return split_list;
}

asset cmnty::ram_to_eos( uint32_t bytes ) {
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

    return amount;
}

void cmnty::updatebprice( symbol_code sym ) {
    get_border_price( sym );
}

// void cmnty::resetpvrate() {
//     require_auth( get_self() );
//
//     auto pv_rate_data = pv_rate_table.begin();
//     if ( pv_rate_data != pv_rate_table.end() ) {
//         pv_rate_table.erase( pv_rate_data );
//     }
// }

// void cmnty::deletepvrate( uint64_t timestamp ) {
//     require_auth( get_self() );
//
//     auto& data = pv_rate_table.get( timestamp );
//     uint64_t rate = data.rate;
//     pv_rate_table.erase( data );
// }

/// dispatcher
extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if ( code == name("eosio.token").value ) {
            if ( action == name("transfer").value ) {
                eosio::execute_action( name(receiver), name(code), &cmnty::moveeos );
            }
        } else if ( code == receiver ) {
            switch( action ) {
               EOSIO_DISPATCH_HELPER( cmnty,
                   (create)
                   // (destroy)
                   (issue)
                   (transferbyid)
                   (transfer)
                   (burnbyid)
                   // (burn)
                   (refreshkey)
                   (refleshkey) // スペルミス
                   (addsellobyid)
                   // (addsellorder)
                   // (issueandsell)
                   (buyfromorder)
                   (cancelsobyid)
                   (cancelsello)
                   // (cancelsoburn)
                   (addbuyorder)
                   (selltoorder)
                   (cancelbobyid)
                   (cancelbuyo)
                   (setmanager)
                   (withdraw)
                   (setoffer)
                   (acceptoffer)
                   (contentsname)
                   (rejectoffer)
                   (removeoffer)
                   (addpvcount)
                   (dropcontent)
                   (updatebprice)
               );
            }
        }
    }
}
