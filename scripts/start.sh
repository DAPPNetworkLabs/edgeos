#!/bin/bash
echo "Restart nodeos"

kill -9 $(lsof -t -i:8888)

nodeos -e -p eosio \
	--plugin eosio::producer_plugin \
	--plugin eosio::producer_api_plugin \
	--plugin eosio::chain_api_plugin \
	--plugin eosio::http_plugin \
	--plugin eosio::history_plugin \
	--plugin eosio::history_api_plugin \
	--delete-all-blocks \
	--filter-on="*" \
	--access-control-allow-origin='*' \
	--contracts-console \
	--http-validate-host=false \
	--max-transaction-time=150000 \
    --http-max-response-time-ms=9999999 \
	--http-server-address=0.0.0.0:8888 \
	--eos-vm-oc-enable\
	--eos-vm-oc-compile-threads=2\
	--verbose-http-errors >> nodeos.log 2>&1 &

echo "Restart ganache1"
kill -9 $(lsof -t -i:8545)
ganache-cli -p 8545 -b 1 -m 'either ostrich protect jump kingdom flat neck cabin sock they vast merit' >> ganache1.log 2>&1 &

echo "Restart ganache2"
kill -9 $(lsof -t -i:9545)
ganache-cli -p 9545 -b 1 -m 'either ostrich protect jump kingdom flat neck cabin sock they vast merit' >> ganache2.log 2>&1 &
