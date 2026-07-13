#!/bin/bash

SERVER=http://localhost:8888

# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate.csv" \
    $SERVER/exchange-rate

# 2. Create Strategy
curl -v -X POST \
    -F "file=@ab_strat.csv" \
    $SERVER/create-strategy

# 3. Add Investor
curl -v -X POST \
    -F "file=@ab_inv.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "file=@ab_subs_upa.csv" \
    $SERVER/subs-upa


