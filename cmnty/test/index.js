const Eos = require("eosjs");
const EosApi = require("eosjs-api");
const ecc = require("eosjs-ecc");
const { Contract } = require('./contract.js');
const { getCheapestToken } = require('./table.js');

const fetch = require('node-fetch');                  // node only; not needed in browsers
const { TextEncoder, TextDecoder } = require('util'); // node only; native TextEncoder/Decoder

const keyProvider = require('./cleosWalletPrivateKeys.json');

const httpEndpoint = "https://api-kylin.eosasia.one:443";

const chainId = "5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191";

const api = new EosApi({ httpEndpoint });

const cmntyContractName = "toycashio123";

const eos = Eos({ keyProvider, httpEndpoint, chainId });
console.log(eos.contract);

async function processEvent(event, context, callback) {
    try {

        const contract = new Contract(eos, cmntyContractName);
        const message = Math.floor(Number(new Date()) / (24 * 60 * 60 * 1000)).toString();
        console.log(message);

        const { symbolCode, signature, subKey } = event;

        const success = ecc.recover(signature, message) === subKey;

        if (!success) {
            throw new Error("the signature is invalid");
        }

        const rows = await getCheapestToken(cmntyContractName, symbolCode, accountName);

        if (rows.length === 0) {
            throw new Error("No token is found in the selling order");
        }

        const tokenId = rows[0].id;
        const tokenPrice = rows[0].price;

        const signedTransaction = await contract.buyTokenFromDex(accountName, symbolCode, tokenId, tokenPrice, subKey);

        return callback(null, signedTransaction);
    } catch (err) {
        console.log(err);

        return callback(err);
    }
}

// const symbolCode = "PCS";
// const message = Math.floor(Number(new Date()) / (24 * 60 * 60 * 1000)).toString();
// console.log(message);
// const wif = ;
// const signature = ecc.sign(message, wif);
// const subkey = ecc.privateToPublic(wif);
//
// console.log(subkey);
// console.log(signature);

// processEvent({ symbolCode, signature, subKey });
