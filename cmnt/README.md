# Community Scaling Token

## Abstraction

TOY を拡張し、配信コンテンツの PV 数を元にトークンの価値を算出できるようにした規格

## Commands

### Preparation

```
# デフォルトで Kylin testnet に接続するようにする
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
# トークンを作成する
cleos push action toycashcmnty create '["leohioleohio", "TOY"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty currency
cleos get table toycashcmnty TOY token

# 自分に対してトークンを発行する
cleos push action toycashcmnty issue '["leohioleohio", "2 TOY", "make TOY"]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty currency
cleos get table toycashcmnty TOY token
cleos get table toycashcmnty leohioleohio accounts

# トークンに subkey を登録する
cleos push action toycashcmnty refleshkey '["leohioleohio", "TOY", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token

# 所持トークンをトークンを送信する
cleos push action toycashcmnty transferbyid '["leohioleohio", "mokemokecore", "TOY", 1, "send token"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token

# トークンを破棄する
cleos push action toycashcmnty burnbyid '["leohioleohio", "TOY", 1]' -p leohioleohio@active

cleos get table toycashcmnty TOY token
```

### DEX

```
cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action toycashcmnty sellbyid '["leohioleohio", "TOY", 2, "0.1000 EOS", "serve sell order"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder

# トークンを購入
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "toycashcmnty,buy,TOY,2"]' -p mokemokecore@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder
cleos get table eosio.token leohioleohio accounts
```

### Offer & Contents

```
cleos get table toycashcmnty mokemokecore deposit

# トークンをデポジット
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "transfer EOS for offer"]' -p mokemokecore@active

cleos get table toycashcmnty mokemokecore deposit
cleos get table toycashcmnty TOY offer
cleos get table toycashcmnty toycashcmnty contents

# 配当の分配比率を決める
# id: 0 のトークンを持っている人は 10/16
# id: 2 のトークンを持っている人は  4/16
# その他のトークンを持っている人で残りの 2/16 を均等に分配
# 今回は、トークン 0, 2 以外は発行されていない前提なので、残りはコントラクトがもらう
cleos push action toycashcmnty setmanager '["TOY", 0, [0, 2], [10, 4], 2]' -p leohioleohio@active

cleos get table toycashcmnty TOY community

# オファーの提案を出す
cleos push action toycashcmnty setoffer '["mokemokecore", "TOY", "https://www.coinershigh.com", "0.1000 EOS"]' -p mokemokecore@active

cleos get table toycashcmnty TOY offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action toycashcmnty acceptoffer '["leohioleohio", "TOY", 0]' -p leohioleohio@active

cleos get table toycashcmnty TOY offer
cleos get table toycashcmnty toycashcmnty contents

# PV 数を記録する
cleos push action toycashcmnty addpvcount '[0, 1]' -p toycashcmnty@active

cleos get table toycashcmnty toycashcmnty contents

# コンテンツの配信を停止する
cleos push action toycashcmnty stopcontents '["leohioleohio", 0]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty contents

# コンテンツを削除する
cleos push action toycashcmnty dropcontents '["leohioleohio", 0]' -p leohioleohio@active

cleos get table toycashcmnty toycashcmnty contents
```
