async function getTable(query) {
    const url = "https://api-kylin.eosasia.one:443/v1/chain/get_table_rows";
    const req = {
        method: "POST",
        mode: "cors",
        body: JSON.stringify({"json": true, ...query})
    };

    let response = await fetch(url, req);
    let result = await response.json();

    return result;
}

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

async function getCheapestToken(_contractName, _symbolCode) {
    const query = {
        "code": _contractName,
        "scope": _symbolCode,
        "table": "sellorder",
        "index_position": 2, // secondary index 'byprice'
        "key_type": "i64",
        "limit": 1
    };

    const result = await getTable(query);

    return result.rows;
}

module.exports = { getTable, getOwnToken, getSellOrder, getCheapestToken };
