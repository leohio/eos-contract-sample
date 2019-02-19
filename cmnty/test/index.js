const Eos = require("eosjs");
const { Contract } = require('./contract.js');
const { getCheapestToken } = require('./table.js');

/// ./cleosWalletPrivateKeys.json の中身は
/// cleos wallet private_keys コマンドで出力される JSON
/// [[PUBLIC_KEY1, PRIVATE_KEY1], [PUBLIC_KEY2, PRIVATE_KEY2], ...]
const keyPairs = require('./cleosWalletPrivateKeys.json');

const keyProvider = keyPairs.map( v => v[1] );

const httpEndpoint = "https://api-kylin.eosasia.one:443";

const chainId = "5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191";

const eos = Eos({ keyProvider, httpEndpoint, chainId });

const cmntyContractName = "toycashio123";

const contract = new Contract(eos, cmntyContractName);

async function test(_myAccountName, _symbolCode, _subKey) {
    const rows = await getCheapestToken(cmntyContractName, _symbolCode);

    if (rows.length === 0) {
        throw new Error("No token is found in the selling order");
    }

    const tokenId = rows[0].id;
    const tokenPrice = rows[0].price;

    await contract.buyTokenFromDex(_myAccountName, _symbolCode, tokenId, tokenPrice);

    await contract.refleshKey(_myAccountName, _symbolCode, tokenId, _subKey);

    console.log(`success! ${_myAccountName} account obtain ${_symbolCode} token with ID: ${tokenId} and set subkey: ${_subKey}.`);
}

test(
    "mokemokecore",
    "PCS",
    "EOS5BNS8DwZKb2KE4G5WqHVXrWMLq36f1HmeafRk586X6Uq9FFin4"
).catch((e) => {
    console.log(e);
});
