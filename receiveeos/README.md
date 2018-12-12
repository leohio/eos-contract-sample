# ReceiveEOS

This contract is able to deposit and withdraw EOS token.

## attention

This contract is a simple contract
for detection and inline call of an action in an external account. 
Please don't use as it is,
because **some vulnerability exist** in it.

For example, the contract account deployed this contract is
able to transfer its EOS token to another account freely.
So `withdraw` action may fail despite that your balance is not zero.

## usage

### check cleos available

```
cleos --help
> Command Line Interface to EOSIO Client
> Usage: cleos [OPTIONS] SUBCOMMAND
> ...
```

### set kylin testnet endpoint

```
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### check connection

```
cleos get info
> {
>   "server_version": "0f6695cb",
>   "chain_id": "5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191",
> ...
```

### check your account

```
cleos get account accountname1
> ...
> EOS balances:
>      liquid:          755.6145 EOS
>      staked:            0.0000 EOS
>      unstaking:         0.0000 EOS
>      total:           755.6145 EOS
```

### transfer EOS token to contract

`accountname1` deposit 0.1000 EOS.

NOTE

`receiveeos` contract is deployed in `toycashio123` account.
Please replace `accountname1` your account.

```
cleos push action eosio.token transfer \
'["accountname1", "toycashio123", "0.1000 EOS", "send"]' \
-p accountname1@active
> executed transaction: bea7bf80...3b  136 bytes  1681 us
> #   eosio.token <= eosio.token::transfer
>  {"from":"accountname1","to":"toycashio123","quantity":"0.1000 EOS","memo":"send"}
> #  accountname1 <= eosio.token::transfer
>  {"from":"accountname1","to":"toycashio123","quantity":"0.1000 EOS","memo":"send"}
> #  toycashio123 <= eosio.token::transfer
>  {"from":"accountname1","to":"toycashio123","quantity":"0.1000 EOS","memo":"send"}
> ...

cleos get table toycashio123 toycashio123 balance
> {
>   "rows": [{
>       "sender": "accountname1",
>       "amount": 1000
>     }
>   ],
>   "more": false
> }

cleos get account accountname1
> ...
> EOS balances:
>      liquid:          755.5145 EOS
>      staked:            0.0000 EOS
>      unstaking:         0.0000 EOS
>      total:           755.5145 EOS
```

### withdraw EOS token from contract

`accountname1` withdraw 0.1000 EOS.

```
cleos push action toycashio123 withdraw \
'["accountname1", "0.1000 EOS"]' \
-p accountname1@active
> executed transaction: 3f06a6ae...8a  120 bytes  2183 us
> #  toycashio123 <= toycashio123::withdraw
>  {"user":"accountname1","quantity":"0.1000 EOS"}
> #   eosio.token <= eosio.token::transfer
>  {"from":"toycashio123","to":"accountname1","quantity":"0.1000 EOS","memo":"withdraw"}
> #  toycashio123 <= eosio.token::transfer
>  {"from":"toycashio123","to":"accountname1","quantity":"0.1000 EOS","memo":"withdraw"}
> #  accountname1 <= eosio.token::transfer
>  {"from":"toycashio123","to":"accountname1","quantity":"0.1000 EOS","memo":"withdraw"}
> ...

cleos get table toycashio123 toycashio123 balance
> {
>   "rows": [],
>   "more": false
> }

cleos get account accountname1
> ...
> EOS balances:
>      liquid:          755.6145 EOS
>      staked:            0.0000 EOS
>      unstaking:         0.0000 EOS
>      total:           755.6145 EOS
```
