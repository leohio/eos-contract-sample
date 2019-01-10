# pcs

## Abstraction

コンテンツの認証に用いる subkey を NFT に結びつけた

## Commands

### Preparation

```
# デフォルトで Kylin testnet に接続するようにする
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
# トークンを作成する
cleos push action toycashiopcs create '["leohioleohio", "PCS"]' -p toycashiopcs@active

cleos get table toycashiopcs PCS currency

# 自分に対してトークンを2つ発行する
cleos push action toycashiopcs issue '["leohioleohio", "2 PCS", "make PCS"]' -p leohioleohio@active

cleos get table toycashiopcs PCS currency
cleos get table toycashiopcs toycashiopcs token
cleos get table toycashiopcs leohioleohio accounts

# トークンに subkey を登録する
cleos push action toycashiopcs refleshkey '["leohioleohio", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token

# 所持トークンをトークンを送信する
cleos push action toycashiopcs transfer '["leohioleohio", "mokemokecore", "PCS", "send token"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token

# token ID を指定してトークンを送信する
cleos push action toycashiopcs transferid '["mokemokecore", "leohioleohio", 0, "return token"]' -p mokemokecore@active

cleos get table toycashiopcs toycashiopcs token

# トークンを破棄する
cleos push action toycashiopcs burn '["leohioleohio", 0]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token
```

### DEX

```
cleos get table toycashiopcs toycashiopcs token
cleos get table toycashiopcs toycashiopcs sellorder
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action toycashiopcs servebid '["leohioleohio", 1, "0.1000 EOS", "serve bid order"]' -p leohioleohio@active

cleos get table toycashiopcs toycashiopcs token
cleos get table toycashiopcs toycashiopcs sellorder

# トークンを購入
cleos push action eosio.token transfer '["mokemokecore", "toycashiopcs", "0.1000 EOS", "toycashiopcs,buy,1,EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p mokemokecore@active

cleos get table toycashiopcs toycashiopcs token
cleos get table toycashiopcs toycashiopcs sellorder
cleos get table eosio.token leohioleohio accounts
```
