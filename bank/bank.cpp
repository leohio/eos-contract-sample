/**
 * @name bank
 * @dev detect that one transfer EOS token to account deployed this contract
 *     and record its amount in the multi-index table
 * @version eosio.cdt v1.4.1
**/

#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

using namespace eosio;
using std::string;

class [[eosio::contract]] bank : public eosio::contract {
    private:
        /**
         * Struct
        **/

        /// the arguments of eosio.token::transfer action
        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        /// amount which sender account transfer to eosio.token contract
        struct [[eosio::table]] account {
            asset balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        /**
         * Multi Index
        **/

        using account_index = eosio::multi_index< name("accounts"), account >;
        using balance_index = eosio::multi_index< name("balance"), account >;
        using deposit_index = eosio::multi_index< name("totaldeposit"), account >;

        deposit_index deposit_table;

        /**
         * Public Function
        **/

        void add_balance( name user, asset quantity );
        void add_deposit( name user, asset quantity );
        void sub_balance( name user, asset quantity );
        void sub_deposit( name user, asset quantity );
        void transfer_to( name user, asset quantity );
        asset get_total_deposit();
        asset get_current_balance();

    public:
        /**
         * Constructor
        **/

        bank( name receiver, name code, datastream<const char*> ds ):
            contract::contract( receiver, code, ds ),
            deposit_table( receiver, receiver.value ) {}

        /**
         * Public Function
        **/

        [[eosio::action]] void  transfereos();
        [[eosio::action]] void     withdraw( name user, asset quantity );
};

void bank::transfereos() {
    /// receive action data of eosio.token::transfer
    auto transfer_data = eosio::unpack_action_data<transfer_args>();
    name from = transfer_data.from;
    name to = transfer_data.to;
    asset quantity = transfer_data.quantity;

    if ( to == get_self() && from != get_self() ) {
        /// 着金を確認したら、テーブルに書き込む
        add_balance( from, quantity );
        add_deposit( from, quantity );
    } else if ( from == get_self() && to != get_self() ) {
        /// このコントラクトの EOS 残高を check し
        /// 預かっている額よりも減らないように監視する
        asset total_amount = get_total_deposit();
        asset current_amount = get_current_balance();
        eosio_assert( total_amount <= current_amount, "this contract must leave at least total deposit" );
    }
}

void bank::withdraw( name user, asset quantity ) {
    require_auth( user );
    sub_balance( user, quantity );
    sub_deposit( user, quantity );
    transfer_to( user, quantity );
}

void bank::add_balance( name user, asset quantity ) {
    symbol core_symbol = quantity.symbol;

    balance_index balance_table( get_self(), user.value );
    auto balance_data = balance_table.find( core_symbol.code().raw() );

    if( balance_data == balance_table.end() ) {
        balance_table.emplace( get_self(), [&]( auto& data ) {
            data.balance = quantity;
        });
    } else {
        asset deposit = balance_data->balance;
        eosio_assert( deposit.symbol == quantity.symbol, "different symbol or precision mismatch from token of total deposit" );

        /// modify the record of balance_data
        /// ram payer is this contract account
        balance_table.modify( balance_data, get_self(), [&]( auto& data ) {
            data.balance += quantity;
        });
    }
}

void bank::add_deposit( name user, asset quantity ) {
    symbol core_symbol = quantity.symbol;

    auto deposit_data = deposit_table.find( core_symbol.code().raw() );

    if( deposit_data == deposit_table.end() ) {
        deposit_table.emplace( get_self(), [&]( auto& data ) {
            data.balance = quantity;
        });
    } else {
        asset deposit = deposit_data->balance;
        eosio_assert( deposit.symbol == quantity.symbol, "different symbol or precision mismatch from your token deposited" );

        /// modify the record of deposit_data
        /// ram payer is this contract account
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance += quantity;
        });
    }
}

void bank::sub_balance( name user, asset quantity ) {
    symbol core_symbol = quantity.symbol;

    balance_index balance_table( get_self(), user.value );
    auto balance_data = balance_table.find( core_symbol.code().raw() );
    eosio_assert( balance_data != balance_table.end(), "do not exist your data" );

    asset deposit = balance_data->balance;
    eosio_assert( deposit.symbol == core_symbol, "invalid your deposit symbol" );
    eosio_assert( deposit.amount >= quantity.amount, "exceed the withdrawable amount" );

    if ( deposit == quantity ) {
        balance_table.erase( balance_data );
    } else {
        /// modify the record of balance_data
        /// ram payer is this contract account
        balance_table.modify( balance_data, get_self(), [&]( auto& data ) {
            data.balance -= quantity;
        });
    }
}

void bank::sub_deposit( name user, asset quantity ) {
    symbol core_symbol = quantity.symbol;

    auto deposit_data = deposit_table.find( core_symbol.code().raw() );
    eosio_assert( deposit_data != deposit_table.end(), "do not exist total deposit data" );

    asset total_deposit = deposit_data->balance;
    eosio_assert( total_deposit.symbol == core_symbol, "invalid total deposit symbol" );
    eosio_assert( total_deposit.amount >= quantity.amount, "total deposit table is broken" );

    if ( total_deposit == quantity ) {
        deposit_table.erase( deposit_data );
    } else {
        /// modify the record of deposit_data
        /// ram payer is this contract account
        deposit_table.modify( deposit_data, get_self(), [&]( auto& data ) {
            data.balance -= quantity;
        });
    }
}

void bank::transfer_to( name user, asset quantity ) {
    /// execute "transfer" action (third arg)
    /// defined in "eosio.token" account (second arg)
    /// passing its arguments by tuple (fourth arg)
    /// using this account's active permission (first arg)
    action(
        permission_level{ get_self(), name("active") },
        name("eosio.token"),
        name("transfer"),
        std::make_tuple( get_self(), user, quantity, string("call as inline action in ") + get_self().to_string() )
    ).send();
}

asset bank::get_total_deposit() {
    symbol core_symbol = symbol( "EOS", 4 );
    auto total_deposit = deposit_table.find( core_symbol.code().raw() );
    asset total_amount = asset{ 0, core_symbol };

    if ( total_deposit != deposit_table.end() ) {
        total_amount = total_deposit->balance;
    }

    return total_amount;
}

asset bank::get_current_balance() {
    symbol core_symbol = symbol( "EOS", 4 );
    account_index account_table( name("eosio.token"), get_self().value );
    auto account_data = account_table.find( core_symbol.code().raw() );
    asset current_amount = asset{ 0, core_symbol };

    if ( account_data != account_table.end() ) {
        current_amount = account_data->balance;
    }

    return current_amount;
}

/// dispatcher
extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        if ( code == name("eosio.token").value ) {
            if ( action == name("transfer").value ) {
                /// When one call eosio.token::transfer action,
                /// bank::transfereos action is also executed.
                execute_action( name(receiver), name(code), &bank::transfereos );
            }
        } else if ( code == receiver ) {
            switch( action ) {
               EOSIO_DISPATCH_HELPER( bank, (withdraw) );
            }
        }
    }
}
