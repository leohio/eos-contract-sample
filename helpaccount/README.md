# helpaccount

## Warning

This contract has not complete yet.

## Usage

When this contract is deployed in <contract> account,
if one would like to buy some ram for <to> account using <permission>,
then one execute the following command:

```
cleos -u <endpoint> push action <contract> buyram '[<to>, <ram_bytes>]' -p <permission>
```

For example,

```
cleos -u https://api-kylin.eosasia.one:443 \
push action helpaccount1 buyram '["mokemokecore", 1024]' -p mokemokecore@active
```
