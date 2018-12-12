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
            uint64_t amount;
            uint64_t primary_key() const { return sender.value; }
            uint64_t get_amount() const { return amount; }
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
                /// receive only EOS token
                // eosio_assert(
                //     transfer_data.quantity.symbol == "EOS",
                //     "be able to receive only EOS"
                // );

                uint64_t amount = static_cast<uint64_t>( transfer_data.quantity.amount );

                auto balance_data = bt.find( from_account.value );

                if( balance_data == bt.end() ) {
                    bt.emplace( get_self(), [&]( auto& data ) {
                        data.sender = from_account;
                        data.amount = amount;
                    });
                } else {
                    eosio_assert(
                        balance_data->amount + amount > balance_data->amount,
                        "since occurred overflow, revert the state"
                    );
                    /// modify the record of balance_data
                    /// ram payer is this contract account
                    bt.modify( balance_data, get_self(), [&]( auto& data ) {
                        data.amount = balance_data->amount + amount;
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

            // eosio_assert(
            //     quantity.symbol == "EOS",
            //     "be able to receive only EOS"
            // );

            uint64_t current_balance = balance_data->amount;
            uint64_t amount = quantity.amount;

            eosio_assert(
                current_balance >= amount,
                "exceed the withdrawable amount"
            );

            if ( current_balance == amount ) {
                bt.erase( balance_data );
            } else if ( current_balance > amount ) {
                /// modify the record of balance_data
                /// ram payer is this contract account
                bt.modify( balance_data, get_self(), [&]( auto& data ) {
                    data.amount = current_balance - amount;
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
               EOSIO_DISPATCH_HELPER( receiveeos, (withdraw) )
            }
        }
    }
}
