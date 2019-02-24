# Community Scaling Token

## Abstraction

PCS を拡張し、配信コンテンツの PV 数を元にトークンの価値を算出できるようにした規格

[API Reference](https://github.com/mokemokehardcore/eos-contract-sample/wiki/Community-Scaling-Token)

## Example

### Preparation

```
# デフォルトで Kylin testnet に接続するようにする
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
# トークンを作成する
cleos push action toycashio123 create '["leohioleohio", "TOY"]' -p leohioleohio@active

cleos get table toycashio123 toycashio123 currency
cleos get table toycashio123 TOY token

# 自分に対してトークンを発行する
cleos push action toycashio123 issue '["leohioleohio", "2 TOY", "make TOY"]' -p leohioleohio@active

cleos get table toycashio123 toycashio123 currency
cleos get table toycashio123 TOY token
cleos get table toycashio123 leohioleohio accounts

# トークンに subkey を登録する
cleos push action toycashio123 refleshkey '["TOY", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table toycashio123 TOY token

# 所持トークンを送信する
cleos push action toycashio123 transferbyid '["leohioleohio", "mokemokecore", "TOY", 1, "send token"]' -p leohioleohio@active

cleos get table toycashio123 TOY token

# トークンを破棄する
cleos push action toycashio123 burnbyid '["TOY", 1]' -p leohioleohio@active

cleos get table toycashio123 TOY token
```

### Sell Order

```
cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY sellorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action toycashio123 addsellobyid '["TOY", 2, "0.1000 EOS", "add sell order"]' -p leohioleohio@active

cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY sellorder
cleos get table eosio.token mokemokecore accounts

# EOS をデポジットする
cleos push action eosio.token transfer '["mokemokecore", "toycashio123", "0.1000 EOS", "deposit EOS to buy token from order"]' -p mokemokecore@active

# トークンを購入
cleos push action toycashio123 buyfromorder '["mokemokecore", "TOY", 2, "buy token from leohioleohio"]' -p mokemokecore@active

cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY sellorder
cleos get table eosio.token leohioleohio accounts
```

### Buy Order

```
cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY buyorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# EOS をデポジットする
cleos push action eosio.token transfer '["leohioleohio", "toycashio123", "0.1000 EOS", "deposit EOS to order token"]' -p leohioleohio@active

# トークンを注文する
cleos push action toycashio123 addbuyorder '["leohioleohio", "TOY", "0.1000 EOS"]' -p leohioleohio@active

cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY buyorder
cleos get table eosio.token leohioleohio accounts

# トークンを売る
cleos push action toycashio123 selltoorder '["TOY", 2, 0, "sell token"]' -p mokemokecore@active

cleos get table toycashio123 TOY token
cleos get table toycashio123 TOY buyorder
cleos get table eosio.token mokemokecore accounts
```

### Offer & Contents

```
cleos get table toycashio123 mokemokecore deposit
cleos get table toycashio123 TOY offer
cleos get table toycashio123 toycashio123 contents

# EOS をデポジットする
cleos push action eosio.token transfer '["mokemokecore", "toycashio123", "0.1000 EOS", "deposit EOS to offer"]' -p mokemokecore@active

# オファーの提案を出す
cleos push action toycashio123 setoffer '["mokemokecore", "TOY", "https://www.geomerlin.com", "0.1000 EOS", "set offer"]' -p mokemokecore@active

cleos get table toycashio123 TOY offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action toycashio123 acceptoffer '["leohioleohio", "TOY", 0]' -p leohioleohio@active

cleos get table toycashio123 TOY offer
cleos get table toycashio123 toycashio123 currency
cleos get table toycashio123 TOY contents
cleos get table toycashio123 toycashio123 world

# ID: 0 のコンテンツの PV 数を 1 増加させる
cleos push action toycashio123 addpvcount '[[["PCS", 0, 1}, ["PCS", 1, 1]]]' -p toycashio123@active

cleos get table toycashio123 toycashio123 currency
cleos get table toycashio123 TOY contents
cleos get table toycashio123 toycashio123 world
```

### Others

```
# 配当の分配比率を決める（デフォルトはトークン発行者が全部もらう）
# id: 0 のトークンを持っている人は 1/2
# id: 1 のトークンを持っている人は 1/4
# id: 2 のトークンを持っている人は 1/4
# その他のトークンを持っている人で残りの 0/4 を均等に分配
# トークン 0, 1, 2 以外は発行されていないならば、残りはコントラクトがもらう
cleos push action toycashio123 setmanager '["TOY", 0, [0, 1, 2], [2, 1, 1], 0]' -p leohioleohio@active

# デポジットした EOS の引き出し
cleos push action toycashio123 withdraw '["leohioleohio", "0.0001 EOS", "withdraw"]' -p leohioleohio@active

# offer を拒否する
cleos push action toycashio123 rejectoffer '["leohioleohio", "TOY", 0, "The reason why I rejected this offer is because of hogehoge"]' -p leohioleohio@active
```
