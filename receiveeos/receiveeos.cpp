/**
 * @name receiveeos
 * @dev detect that one transfer EOS token to account deployed this contract
 *     and record its amount in the multi-index table
 * @version eosio.cdt v1.4.1
 * @note this contract don't use as it is, because some vulnerability exist in it.
**/

#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>

using namespace eosio;

using std::string;

class receiveeos : public eosio::contract {
    private:
        /// the arguments of eosio.token::transfer action
        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
        };

        /// amount which sender account transfer to eosio.token contract
        struct balance {
            name sender;
            asset quantity;
            uint64_t primary_key() const { return sender.value; }
        };

        /// define table named "balance"
        using balance_table = eosio::multi_index< name("balance"), balance >;

        balance_table bt;

    public:
        using contract::contract;

        receiveeos( name receiver, name code, datastream<const char*> ds ):
            contract( receiver, code, ds ),
            bt( receiver, receiver.value ) {}

        void transfereos() {
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
                        data.sender = from_account;
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
            }
        }

        void withdraw( name user, asset quantity ) {
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

            /// execute "transfer" action (third arg)
            /// defined in "eosio.token" account (second arg)
            /// passing its arguments by tuple (fourth arg)
            /// using this account's active permission (first arg)
            action(
                permission_level{ get_self(), name("active") },
                name("eosio.token"),
                name("transfer"),
                std::make_tuple( get_self(), user, quantity, string("withdraw EOS") )
            ).send();
        }
};

/// dispatcher
extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ){
        if ( code == name("eosio.token").value ) {
            if ( action == name("transfer").value ) {
                /// When one call eosio.token::transfer action,
                /// receiveeos::transfereos action is also executed.
                execute_action( name(receiver), name(code), &receiveeos::transfereos );
            }
        } else if ( code == receiver ) {
            switch( action ) {
               EOSIO_DISPATCH_HELPER( receiveeos, (withdraw) );
            }
        }
    }
}
