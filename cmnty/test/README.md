# cmnty contract test

## First

```
$ cleos wallet open
$ cleos wallet unlock
$ cleos wallet private_keys
[[
    "PUBLIC_KEY1",
    "PRIVATE_KEY1"
  ],[
    "PUBLIC_KEY2",
    "PRIVATE_KEY2"
  ],
  ...
]
```

The output of the above command write into `./cleosWalletPrivateKeys.json` file on this `test` directory.
This JSON will become the `keyProvider` parameter used in the eosjs library.

## Run

```
$ node index.js
trx ID: 34a2b77dbf48db40ef8e2a1f1d488cf5032efe0aacb881ee47ef86e289222836
trx ID: 896ccc8185753075abd764330007b40e9e640764f98761e3ee2ad78d14954c91
success! mokemokecore account obtain PCS token with ID: 4 and set subkey: EOS5BNS8DwZKb2KE4G5WqHVXrWMLq36f1HmeafRk586X6Uq9FFin4.
```
