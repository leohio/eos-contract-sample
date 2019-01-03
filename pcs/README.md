# pcstoycashio

## Commands

### Preparation

```
alias cleos='cleos -u https://api-kylin.eosasia.one:443'
```

### Token

```
# トークンを作成する
cleos push action pcstoycashio create '["leohioleohio", "PCS"]' -p pcstoycashio@active

cleos get table pcstoycashio PCS currency

# トークンを発行する
cleos push action pcstoycashio issue '["leohioleohio", "PCS", ["EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"], "make PCS"]' -p leohioleohio@active

cleos get table pcstoycashio PCS currency
cleos get table pcstoycashio pcstoycashio token
cleos get table pcstoycashio leohioleohio accounts

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

### PV Count Data

```
# URI を登録する
cleos push action pcstoycashio resisteruris '["leohioleohio", "PCS", ["https://www.geomerlin.com"]]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio pvcount

# PV 数を記録する
cleos push action pcstoycashio setpvdata '["pcstoycashio", "PCS", "https://www.geomerlin.com", 50]' -p pcstoycashio@active

cleos get table pcstoycashio pcstoycashio pvcount

# URI の登録を解除する （ PV 数は復活しない）
cleos push action pcstoycashio removepvdata '["pcstoycashio", "PCS", "https://www.geomerlin.com"]' -p pcstoycashio@active

cleos get table pcstoycashio pcstoycashio pvcount
```

### DEX

```
# 売りに出すためのトークンを購入
push action pcstoycashio issue '["leohioleohio", "PCS", ["EOS6KEzAbW8EowSEPc1fd5t1mLMmnVDk5rw3PMjNsnZqRN9PPD23S"], "make PCS"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio token
cleos get account leohioleohio

# トークンを売りに出す
cleos push action pcstoycashio servebid '["leohioleohio", 0, "0.1000 EOS", "serve bid order"]' -p leohioleohio@active

cleos get table pcstoycashio pcstoycashio bid
cleos get account leohioleohio

# トークンを購入するために EOS をデポジット
cleos push action eosio.token transfer '["mokemokecore", "pcstoycashio", "0.1000 EOS", "send EOS to buy token"]' -p mokemokecore@active

# トークンを購入
cleos push action pcstoycashio buy '["mokemokecore", 0, "0.1000 EOS", "buy token"]' -p mokemokecore@active

cleos get table pcstoycashio pcstoycashio bid
cleos get account leohioleohio
cleos get table pcstoycashio pcstoycashio token

# subkey を新しくしてトークンをアクティベートする
cleos push action pcstoycashio refleshkey '["mokemokecore", 0, "EOS6wjz1Z9nN6rWWxQyJGxxXQrWZUj3LxDF5F2MxMnKFiutrTM5Q5"]' -p mokemokecore@active
```
