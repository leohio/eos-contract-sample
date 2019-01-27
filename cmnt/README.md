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

# API

* \_self はコントラクトがデプロイされていいるアカウント名を表します.
* [] で括った文字列は変数の型を表します.

## Action

### create

コミュニティを作成する.
コミュニティの名前は,
そこで用いられるトークンの名前にもなる.
コミュニティの作成者は自動的にその管理者になり,
トークンを新規発行する権限を持つ.

#### Argument

* issuer [name] コミュニティの作成者
* sym [symbol_code] コミュニティの名前

#### Authentication

* issuer と同じアカウント
* \_self 以外

#### Example

```
cleos push action toycashcmnty create '["leohioleohio", "TOY"]' -p leohioleohio@active
```


### issue

コミュニティ内で用いられるトークンを新規発行する.

#### Argument

* user [name] トークンを渡す相手
* quantity [asset] コミュニティの名前と発行する枚数
* memo [string]

#### Authentication

* quantity に指定したコミュニティの作成者

#### Example

```
cleos push action toycashcmnty issue '["leohioleohio", "2 TOY", "make TOY"]' -p leohioleohio@active
```


### transferbyid

トークンの所有者を変更する.

#### Argument

* from [name] トークンの所有者
* to [name] トークンを渡す相手
* sym [symbol_code] トークンの名前
* token_id [uint64_t] 渡すトークンの ID
* memo [string]

#### Authentication

* from に指定したアカウント
* \_self, to 以外
* 名前が sym で ID が token_id であるトークンの所有者

#### Example


### burnbyid

所持しているトークンを削除する.

#### Argument

* owner [name] トークンの所有者
* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 削除するトークンの ID

#### Authentication

* 名前が sym で ID が token_id であるトークンの所有者
* \_self 以外

#### Example


### refleshkey

所持しているトークンに subkey を設定し,
アクティベートする.

#### Argument

* owner [name] トークンの所有者
* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 削除するトークンの ID
* subkey [capi_public_key] 認証に用いる公開鍵

#### Authentication

* 名前が sym で ID が token_id であるトークンの所有者
* \_self 以外

#### Example


### addsellobyid

所持しているトークンを販売する.
この間トークンの所有者は \_self になる
（もちろん \_self は販売中のトークンに対していかなる操作も実行できない）.

#### Argument

* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 売りたいトークンの ID
* price [asset] 販売額（単位は EOS）

#### Authentication

* 名前が sym で ID が token_id であるトークンの所有者
* \_self 以外

#### Example


### buyfromorder

販売されているトークンを購入する.
トークンの所有権は購入者に移動し,
新しい subkey を設定するまでトークンの認証機能がロックされる.
購入に必要な EOS は事前のデポジットから引き落とされ,
販売者に渡される.

#### Argument

* buyer [name] トークンの購入者
* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 買いたいトークンの ID
* memo [string]

#### Authentication

* buyer に指定したアカウント
* \_self, 販売者以外

#### Example


### cancelsobyid

トークンの販売をやめる.
トークンの所有権が復活する.

#### Argument

* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 販売をやめるトークンの ID

#### Authentication

* 名前が sym で ID が token_id であるトークンの販売者
* \_self 以外

#### Example


### addbuyorder

トークンの購入を予約する.
購入に必要な EOS は事前のデポジットから引き落とされる.

#### Argument

* buyer [name] 購入を予約するアカウント
* sym [symbol_code] コミュニティの名前
* price [asset] 購入額（単位は EOS）

#### Authentication

* buyer に指定したアカウント
* \_self 以外

#### Example


### selltoorder

予約中の人に対してトークンを販売する.
トークンの所有権は販売者から購入者に移動し,
新しい subkey を設定するまでトークンの認証機能がロックされる.
購入額分の EOS が販売者に渡される.

#### Argument

* sym [symbol_code] コミュニティの名前
* token_id [uint64_t] 販売するトークンの ID
* order_id [uint64_t] 注文の ID
* price [asset] 購入額（単位は EOS）

#### Authentication

* 名前が sym で ID が token_id であるトークンの所有者
* \_self, 購入者以外

#### Example


### cancelbobyid

トークンの予約をやめる.
事前に引き落とされた EOS が戻ってくる.

#### Argument

* sym [symbol_code] コミュニティの名前
* order_id [uint64_t] 予約をやめる注文の ID

#### Authentication

* 名前が sym で ID が order_id である予約をしたアカウント
* \_self 以外

#### Example


### withdraw

デポジットしている EOS を自分のアカウントに戻す.

#### Argument

* user [name] 引き出しを行うアカウント
* value [asset] 引出額（単位は EOS）
* memo [string]

#### Authentication

* buyer に指定したアカウント
* \_self 以外

#### Example


### setoffer

#### Argument

#### Authentication

#### Example


### acceptoffer

#### Argument

#### Authentication

#### Example


### removeoffer

#### Argument

#### Authentication

#### Example


### addpvcount

#### Argument

#### Authentication

#### Example


## Table

### accounts

トークンの所有枚数が記録される.

#### Scope

アカウント名（[name] の value 値）

#### Primary Key

balance.symbol.code().raw()

#### Field

* balance [asset] トークンの所有枚数

#### Example

```
cleos get table toycashcmnty leohioleohio accounts
> {
>   "rows": [{
>       "balance": "40 PCS"
>      },{
>       "balance": "2 TOY"
>     }
>   ],
>   "more": false
> }
```

---

### deposit

EOS のデポジットが記録される.

#### Scope

アカウント名（[name] の value 値）

#### Primary Key

balance.symbol.code().raw()

#### Field

* balance [asset] トークンの所有枚数

#### Example

```
cleos get table toycashcmnty leohioleohio deposit
```
