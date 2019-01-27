# Community Scaling Token

## Abstraction

PCS を拡張し、配信コンテンツの PV 数を元にトークンの価値を算出できるようにした規格

## Example

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
cleos push action toycashcmnty refleshkey
'["TOY", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]'
-p leohioleohio@active

cleos get table toycashcmnty TOY token

# 所持トークンを送信する
cleos push action toycashcmnty transferbyid '["leohioleohio", "mokemokecore", "TOY", 1, "send token"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token

# トークンを破棄する
cleos push action toycashcmnty burnbyid '["TOY", 1]' -p leohioleohio@active

cleos get table toycashcmnty TOY token
```

### Sell Order

```
cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action toycashcmnty addsellobyid '["TOY", 2, "0.1000 EOS", "serve sell order"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder
cleos get table eosio.token mokemokecore accounts

# トークンを購入（デポジットと購入のアクションを同時に行う）
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "toycashcmnty,buy,TOY,2"]' -p mokemokecore@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY sellorder
cleos get table eosio.token leohioleohio accounts
```

### Buy Order

```
cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY buyorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# トークンを注文する（デポジットと注文のアクションを同時に行う）
cleos push action eosio.token transfer '["leohioleohio", "toycashcmnty", "0.1000 EOS", "toycashcmnty,addbuyorder,TOY"]' -p leohioleohio@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY buyorder
cleos get table eosio.token leohioleohio accounts

# トークンを売る
cleos push action toycashcmnty selltoorder '["TOY", 2, 0, "serve sell order"]' -p mokemokecore@active

cleos get table toycashcmnty TOY token
cleos get table toycashcmnty TOY buyorder
cleos get table eosio.token mokemokecore accounts
```

### Offer & Contents

```
cleos get table toycashcmnty mokemokecore deposit
cleos get table toycashcmnty TOY offer
cleos get table toycashcmnty toycashcmnty contents

# オファーの提案を出す（デポジットと提案のアクションを同時に行う）
cleos push action eosio.token transfer '["mokemokecore", "toycashcmnty", "0.1000 EOS", "toycashcmnty,setoffer,TOY,https://www.geomerlin.com"]' -p mokemokecore@active

cleos get table toycashcmnty TOY offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action toycashcmnty acceptoffer '["leohioleohio", "TOY", 0]' -p leohioleohio@active

cleos get table toycashcmnty TOY offer
cleos get table toycashcmnty 0 contentspv # コンテンツ ID: 0 の PV 数追加履歴
cleos get table toycashcmnty toycashcmnty currency
cleos get table toycashcmnty toycashcmnty contents
cleos get table toycashcmnty toycashcmnty world

# ID: 0 のコンテンツの PV 数を 1 増加させる
cleos push action toycashcmnty addpvcount '[0, 1]' -p toycashcmnty@active

cleos get table toycashcmnty 0 contentspv
cleos get table toycashcmnty toycashcmnty currency
cleos get table toycashcmnty toycashcmnty contents
cleos get table toycashcmnty toycashcmnty world
```

### Others

```
# 配当の分配比率を決める（デフォルトはトークン発行者が全部もらう）
# id: 0 のトークンを持っている人は 10/16
# id: 2 のトークンを持っている人は  4/16
# その他のトークンを持っている人で残りの 2/16 を均等に分配
# トークン 0, 2 以外は発行されていないならば、残りはコントラクトがもらう
cleos push action toycashcmnty setmanager '["TOY", 0, [0, 2], [10, 4], 2]' -p leohioleohio@active

# デポジットした EOS の引き出し
cleos push action toycashcmnty withdraw '["leohioleohio", "0.0001 EOS", "withdraw"]' -p leohioleohio@active
```
