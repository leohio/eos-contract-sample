class Contract {
    constructor(_eosjsInstance, _cmntyContractName, _broadcast=false, _blocksBehind=1, _expireSeconds=30) {
        this.cmntyContractName = _cmntyContractName;
        this.eosioTokenContractName = "eosio.token";
        this.broadcast = _broadcast;
        this.blocksBehind = _blocksBehind;
        this.expireSeconds = _expireSeconds;
        this.eos = _eosjsInstance;
    }

    async signTransaction(...actionData) {
        const pushTransactionArgs = await this.eos.transaction({
            actions: actionData
        }, {
            broadcast: this.broadcast,
            blocksBehind: this.blocksBehind,
            expireSeconds: this.expireSeconds
        });

        console.log("trx ID: " + pushTransactionArgs.transaction_id);

        return pushTransactionArgs;
    }

    async refleshKey(_ownerAccountName, _symbolCode, _tokenId, _newSubKey) {
        const refleshKeyActionData = {
            account: this.cmntyContractName,
            name: "refleshkey",
            authorization: [{
                actor: _ownerAccountName,
                permission: "active"
            }],
            data: {
                sym: _symbolCode,
                token_id: _tokenId,
                subkey: _newSubKey
            }
        };

        return await this.signTransaction(refleshKeyActionData);
    }

    async addPVCount(_managerAccountName, _symbolCode, _contentsId, _pvCount) {
        const addPVCountActionData = {
            account: this.cmntyContractName,
            name: "addpvcount",
            authorization: [{
                actor: _managerAccountName,
                permission: "active"
            }],
            data: {
                sym: _symbolCode,
                contents_id: _contentsId,
                pv_count: _pvCount
            }
        };

        return await this.signTransaction(addPVCountActionData);
    }

    async buyTokenFromDex(_buyerAccountName, _symbolCode, _tokenId, _tokenPrice, _newSubKey) {
        const transferEosActionData = {
            account: this.eosioTokenContractName,
            name: "transfer",
            authorization: [{
                actor: _buyerAccountName,
                permission: "active"
            }],
            data: {
                from: _buyerAccountName,
                to: this.cmntyContractName,
                quantity: _tokenPrice,
                memo: "deposit token to buy token from selling order"
            }
        };

        const buyFromOrderActionData = {
            account: this.cmntyContractName,
            name: "buyfromorder",
            authorization: [{
                actor: _buyerAccountName,
                permission: "active"
            }],
            data: {
                buyer: _buyerAccountName,
                sym: _symbolCode,
                token_id: _tokenId,
                memo: "buy token to sell order"
            }
        };

        const refleshKeyActionData = {
            account: this.cmntyContractName,
            name: "refleshkey",
            authorization: [{
                actor: _buyerAccountName,
                permission: "active"
            }],
            data: {
                sym: _symbolCode,
                token_id: _tokenId,
                subkey: _newSubKey
            }
        };

        return await this.signTransaction(transferEosActionData, buyFromOrderActionData, refleshKeyActionData);
    }

    async setOffer(_providerAccountName, _symbolCode, _contentsUri, _offerPrice) {
        const transferEosActionData = {
            account: this.eosioTokenContractName,
            name: "transfer",
            authorization: [{
                actor: _providerAccountName,
                permission: "active"
            }],
            data: {
                from: _providerAccountName,
                to: this.cmntyContractName,
                quantity: _offerPrice,
                memo: "deposit token to set offer"
            }
        };

        const setOfferActionData = {
            account: this.cmntyContractName,
            name: "setoffer",
            authorization: [{
                actor: _providerAccountName,
                permission: "active"
            }],
            data: {
                provider: _providerAccountName,
                sym: _symbolCode,
                uri: _contentsUri,
                price: _offerPrice
            }
        };

        return await this.signTransaction(transferEosActionData, setOfferActionData);
    }

    async reserveToken(_buyerAccountName, _symbolCode, _tokenPrice) {
        const transferEosActionData = {
            account: this.eosioTokenContractName,
            name: "transfer",
            authorization: [{
                actor: _buyerAccountName,
                permission: "active"
            }],
            data: {
                from: _buyerAccountName,
                to: this.cmntyContractName,
                quantity: _tokenPrice,
                memo: "deposit token to reserve token"
            }
        };

        const addBuyOrderActionData = {
            account: this.cmntyContractName,
            name: "addbuyorder",
            authorization: [{
                actor: _buyerAccountName,
                permission: "active"
            }],
            data: {
                buyer: _buyerAccountName,
                sym: _symbolCode,
                price: _tokenPrice
            }
        };

        return await this.signTransaction(transferEosActionData, addBuyOrderActionData);
    }
}

module.exports = { Contract };
