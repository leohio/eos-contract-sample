const fetch = require("node-fetch");

async function getTable(query) {
    const url = "https://api-kylin.eosasia.one:443/v1/chain/get_table_rows";
    const req = {
        method: "POST",
        mode: "cors",
        body: JSON.stringify({"json": true, ...query})
    };

    try {
        const response = await fetch(url, req);
        const result = await response.json();
        return result;
    } catch(err) {
        console.log(err);
    }
}

// async function pushTransaction(query) {
//     const query = {
//         signatures: , // array of signatures required to authorize transaction
//         compression: false, // compression used, usually false
//         packed_context_free_data: "", // json to hex
//         packed_trx: signedTransaction, // json to hex
//     }
//     const url = "https://api-kylin.eosasia.one:443/v1/chain/push_transaction";
//     const req = {
//         method: "POST",
//         mode: "cors",
//         body: JSON.stringify({"json": true, ...query})
//     };
//
//     let response = await fetch(url, req);
//     let result = await response.json();
//
//     return result;
// }

async function getOwnToken(_contractName, _tokenName, _accountName) {
    const query = {
        "code": _contractName,
        "scope": _tokenName,
        "table": "token",
        "index_position": 2, // secondary index 'byowner'
        "key_type": "i64",
        "lower_bound": _accountName,
        "upper_bound": _accountName
    };

    const result = await getTable(query);

    return result.rows;
}

async function getSellOrder(_contractName, _symbolCode, _tokenId) {
    const query = {
        "code": _contractName,
        "scope": _symbolCode,
        "table": "sellorder",
        "lower_bound": _tokenId,
        "upper_bound": _tokenId
    };

    const result = await getTable(query);

    return result.rows;
}

async function getCheapestToken(_contractName, _symbolCode, _exceptAccountName) {
    const query = {
        "code": _contractName,
        "scope": _symbolCode,
        "table": "sellorder",
        "index_position": 2, // secondary index 'byprice'
        "key_type": "i64",
        "limit": 10
    };

    const result = await getTable(query);

    console.log(result.rows);
    return result.rows.filter((v) => v.user !== _exceptAccountName);
}

module.exports = { getTable, getOwnToken, getSellOrder, getCheapestToken };
