# Comunity Scaling Token

## Abstraction

PCS を拡張し、配信コンテンツの PV 数を元にトークンの価値を算出できるようにした規格

## Commands

### Preparation

```
# デフォルトで Kylin testnet に接続するようにする
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
# トークンを作成する
cleos push action pcstoycashio create '["leohioleohio", "PCS"]' -p pcstoycashio@active

cleos get table pcstoycashio PCS currency

# 自分に対してトークンを2つ発行する
cleos push action pcstoycashio issue '["leohioleohio", "2 PCS", "make PCS"]' -p leohioleohio@active

cleos get table pcstoycashio PCS currency
cleos get table pcstoycashio pcstoycashio token
cleos get table pcstoycashio leohioleohio accounts

# トークンに subkey を登録する
cleos push action pcstoycashio refleshkey '["leohioleohio", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio token

# 所持トークンをトークンを送信する
cleos push action pcstoycashio transfer '["leohioleohio", "mokemokecore", "PCS", "send token"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio token

# token ID を指定してトークンを送信する
cleos push action pcstoycashio transferid '["mokemokecore", "leohioleohio", 0, "return token"]' -p mokemokecore@active

cleos get table pcstoycashio pcstoycashio token

# トークンを破棄する
cleos push action pcstoycashio burn '["leohioleohio", 0]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio token
```

### DEX

```
cleos get table pcstoycashio pcstoycashio token
cleos get table pcstoycashio pcstoycashio bid
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action pcstoycashio servebid '["leohioleohio", 1, "0.1000 EOS", "serve bid order"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio token
cleos get table pcstoycashio pcstoycashio bid

# トークンを購入
cleos push action eosio.token transfer '["mokemokecore", "pcstoycashio", "0.1000 EOS", "buy token#1 in pcstoycashio"]' -p mokemokecore@active

cleos get table pcstoycashio pcstoycashio token
cleos get table pcstoycashio pcstoycashio bid
cleos get table eosio.token leohioleohio accounts
```

### Offer & Contents

```
cleos get table pcstoycashio pcstoycashio balance

# トークンをデポジット
cleos push action eosio.token transfer '["mokemokecore", "pcstoycashio", "0.1000 EOS", "transfer EOS for offer"]' -p mokemokecore@active

cleos get table pcstoycashio PCS offer
cleos get table pcstoycashio PCS contents

# オファーの提案を出す
cleos push action pcstoycashio setoffer '["mokemokecore", "PCS", "https://www.coinershigh.com", "0.1000 EOS"]' -p mokemokecore@active

cleos get table pcstoycashio PCS offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action pcstoycashio acceptoffer '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table pcstoycashio PCS offer
cleos get table pcstoycashio PCS contents

# PV 数を記録する
cleos push action pcstoycashio addpvcount '["PCS", 0, 1]' -p pcstoycashio@active

cleos get table pcstoycashio PCS contents

# コンテンツの配信を停止する
cleos push action pcstoycashio stopcontents '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table pcstoycashio PCS contents

# コンテンツを削除する
cleos push action pcstoycashio dropcontents '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table pcstoycashio PCS contents
```
