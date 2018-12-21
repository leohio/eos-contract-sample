# toycashiopcs

## Commands

### Preparation

```
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
cleos push action toycashiopcs create '["leohioleohio", "PCS"]' -p toycashiopcs@active

cleos get table toycashiopcs PCS stat

cleos push action toycashiopcs issue '["leohioleohio", "1 PCS", ["EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"],"PCS","make PCS"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token

cleos get table toycashiopcs leohioleohio accounts

cleos push action toycashiopcs transfer '["leohioleohio", "mokemokecore", "1 PCS", "send token"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token

cleos push action toycashiopcs transferid '["mokemokecore", "leohioleohio", 0, "return token"]' -p mokemokecore@active

cleos get table toycashiopcs toycashiopcs token

cleos push action toycashiopcs burn '["leohioleohio", 0]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token
```

### PV Count Data

```
cleos push action toycashiopcs seturi '["leohioleohio", "PCS", "www.geomerlin.com"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs pvcount

cleos push action toycashiopcs setpvdata '["toycashiopcs", "PCS", 0, 50]' -p toycashiopcs@active

cleos get table toycashiopcs toycashiopcs pvcount

cleos push action toycashiopcs removepvdata '[0]' -p toycashiopcs@active

cleos get table toycashiopcs toycashiopcs pvcount
```

### DEX

```
push action toycashiopcs issue '["leohioleohio", "1 PCS", ["EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"],"PCS","make PCS"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token

cleos get account leohioleohio

cleos push action toycashiopcs servebid '["leohioleohio", 0, "0.1000 EOS", "serve bid order"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs bid

cleos get account leohioleohio

cleos push action toycashiopcs buy '["mokemokecore", 0, "0.1000 EOS", "buy token"]' -p mokemokecore@active

cleos get table toycashiopcs toycashiopcs bid

cleos get account leohioleohio

cleos get table toycashiopcs toycashiopcs token
```
