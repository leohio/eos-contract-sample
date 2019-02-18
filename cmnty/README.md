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
cleos push action pcstoycashio create '["leohioleohio", "TOY"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio currency
cleos get table pcstoycashio TOY token

# 自分に対してトークンを発行する
cleos push action pcstoycashio issue '["leohioleohio", "2 TOY", "make TOY"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio currency
cleos get table pcstoycashio TOY token
cleos get table pcstoycashio leohioleohio accounts

# トークンに subkey を登録する
cleos push action pcstoycashio refleshkey '["TOY", 0, "EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"]' -p leohioleohio@active

cleos get table pcstoycashio TOY token

# 所持トークンを送信する
cleos push action pcstoycashio transferbyid '["leohioleohio", "mokemokecore", "TOY", 1, "send token"]' -p leohioleohio@active

cleos get table pcstoycashio TOY token

# トークンを破棄する
cleos push action pcstoycashio burnbyid '["TOY", 1]' -p leohioleohio@active

cleos get table pcstoycashio TOY token
```

### Sell Order

```
cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY sellorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# トークンを売りに出す
cleos push action pcstoycashio addsellobyid '["TOY", 2, "0.1000 EOS"]' -p leohioleohio@active

cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY sellorder
cleos get table eosio.token mokemokecore accounts

# EOS をデポジットする
cleos push action eosio.token transfer '["mokemokecore", "pcstoycashio", "0.1000 EOS", "deposit EOS to buy token from order"]' -p mokemokecore@active

# トークンを購入
cleos push action pcstoycashio buyfromorder '["mokemokecore", "TOY", 2, "buy token from leohioleohio"]' -p mokemokecore@active

cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY sellorder
cleos get table eosio.token leohioleohio accounts
```

### Buy Order

```
cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY buyorder
cleos get table eosio.token mokemokecore accounts
cleos get table eosio.token leohioleohio accounts

# EOS をデポジットする
cleos push action eosio.token transfer '["leohioleohio", "pcstoycashio", "0.1000 EOS", "deposit EOS to order token"]' -p leohioleohio@active

# トークンを注文する（デポジットと注文のアクションを同時に行う）
cleos push action pcstoycashio addbuyorder '["leohioleohio", "TOY", "0.1000 EOS"]' -p leohioleohio@active


cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY buyorder
cleos get table eosio.token leohioleohio accounts

# トークンを売る
cleos push action pcstoycashio selltoorder '["TOY", 2, 0, "sell token"]' -p mokemokecore@active

cleos get table pcstoycashio TOY token
cleos get table pcstoycashio TOY buyorder
cleos get table eosio.token mokemokecore accounts
```

### Offer & Contents

```
cleos get table pcstoycashio mokemokecore deposit
cleos get table pcstoycashio TOY offer
cleos get table pcstoycashio pcstoycashio contents

# EOS をデポジットする
cleos push action eosio.token transfer '["mokemokecore", "pcstoycashio", "0.1000 EOS", "deposit EOS to offer"]' -p mokemokecore@active

# オファーの提案を出す
cleos push action eosio.token transfer '["mokemokecore", "TOY", "https://www.geomerlin.com", "0.1000 EOS"]' -p mokemokecore@active

cleos get table pcstoycashio TOY offer

# オファーを受け入れ、コンテンツに昇格させる
cleos push action pcstoycashio acceptoffer '["leohioleohio", "TOY", 0]' -p leohioleohio@active

cleos get table pcstoycashio TOY offer
cleos get table pcstoycashio 0 contentspv # コンテンツ ID: 0 の PV 数追加履歴
cleos get table pcstoycashio pcstoycashio currency
cleos get table pcstoycashio pcstoycashio contents
cleos get table pcstoycashio pcstoycashio world

# ID: 0 のコンテンツの PV 数を 1 増加させる
cleos push action pcstoycashio addpvcount '[0, 1]' -p pcstoycashio@active

cleos get table pcstoycashio 0 contentspv
cleos get table pcstoycashio pcstoycashio currency
cleos get table pcstoycashio pcstoycashio contents
cleos get table pcstoycashio pcstoycashio world
```

### Others

```
# 配当の分配比率を決める（デフォルトはトークン発行者が全部もらう）
# id: 0 のトークンを持っている人は 10/16
# id: 2 のトークンを持っている人は  4/16
# その他のトークンを持っている人で残りの 2/16 を均等に分配
# トークン 0, 2 以外は発行されていないならば、残りはコントラクトがもらう
cleos push action pcstoycashio setmanager '["TOY", 0, [0, 2], [10, 4], 2]' -p leohioleohio@active

# デポジットした EOS の引き出し
cleos push action pcstoycashio withdraw '["leohioleohio", "0.0001 EOS", "withdraw"]' -p leohioleohio@active

# offer を拒否する
cleos push action pcstoycashio rejectoffer '["leohioleohio", "TOY", 0, "The reason why I rejected this offer is because of hogehoge"]' -p leohioleohio@active
```
