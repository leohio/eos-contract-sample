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
cleos push action toycashcmnty create '["leohioleohio", "PCS"]' -p toycashcmnty@active

cleos get table toycashcmnty PCS currency

# 自分に対してトークンを2つ発行する
cleos push action toycashcmnty issue '["leohioleohio", "2 PCS", "make PCS"]' -p leohioleohio@active

cleos get table toycashcmnty PCS currency
cleos get table toycashcmnty toycashcmnty token
cleos get table toycashcmnty leohioleohio accounts

# トークンに subkey を登録する
cleos push action toycashcmnty refleshkey '["leohioleohio", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty token

# 所持トークンをトークンを送信する
cleos push action toycashcmnty transferbyid '["leohioleohio", "mokemokecore", 0, "send token"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty token

# token ID を指定してトークンを送信する
cleos push action toycashcmnty transfer '["mokemokecore", "leohioleohio", "PCS", "return token"]' -p mokemokecore@active

cleos get table toycashcmnty toycashcmnty token

# トークンを破棄する
cleos push action toycashcmnty burn '["leohioleohio", "PCS"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty token
```

### DEX

```
cleos get table toycashcmnty toycashcmnty token
cleos get table toycashcmnty toycashcmnty bid
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action toycashcmnty sellbyid '["leohioleohio", 1, "0.1000 EOS", "serve bid order"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty token
cleos get table toycashcmnty toycashcmnty bid

# トークンを購入
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "toycashcmnty,buy,1"]' -p mokemokecore@active

cleos get table toycashcmnty toycashcmnty token
cleos get table toycashcmnty toycashcmnty bid
cleos get table eosio.token leohioleohio accounts
```

### Offer & Contents

```
cleos get table toycashcmnty toycashcmnty balance

# トークンをデポジット
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "transfer EOS for offer"]' -p mokemokecore@active

cleos get table toycashcmnty PCS offer
cleos get table toycashcmnty PCS contents

# オファーの提案を出す
cleos push action toycashcmnty setoffer '["mokemokecore", "PCS", "https://www.coinershigh.com", "0.1000 EOS"]' -p mokemokecore@active

cleos get table toycashcmnty PCS offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action toycashcmnty acceptoffer '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table toycashcmnty PCS offer
cleos get table toycashcmnty PCS contents

# PV 数を記録する
cleos push action toycashcmnty addpvcount '["PCS", 0, 1]' -p toycashcmnty@active

cleos get table toycashcmnty PCS contents

# コンテンツの配信を停止する
cleos push action toycashcmnty stopcontents '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table toycashcmnty PCS contents

# コンテンツを削除する
cleos push action toycashcmnty dropcontents '["leohioleohio", "PCS", 0]' -p leohioleohio@active

cleos get table toycashcmnty PCS contents
```
