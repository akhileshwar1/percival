#!/bin/bash

SERVER=http://localhost:8888

# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate.csv" \
    $SERVER/exchange-rate

# 2. Create Strategy
curl -v -X POST \
    -F "file=@create_strategy.csv" \
    $SERVER/create-strategy

# 3. Add Investor
curl -v -X POST \
    -F "file=@add_investor.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "file=@subs_upa.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@bank_transfer.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@reverse_upa.csv" \
    $SERVER/reverse-upa

# 7. Fund Cashflow
curl -v -X POST \
    -F "file=@fund_cashflow.csv" \
    $SERVER/fund-cashflow

# 8. Unit Allotment
curl -v -X POST \
    -F "file=@unit_allotment.csv" \
    $SERVER/allot-units

# 9. Fund Expense
curl -v -X POST \
    -F "file=@fund_expense.csv" \
    $SERVER/fund-expense

# 10. F&O Trades
curl -v -X POST \
    -F "strategySymbol=CLSA" \
    -F "file=@trades_fno.csv" \
    $SERVER/trades-fno

# 11. F&O Bhavcopy
curl -v -X POST \
    -F "strategySymbol=CLSA" \
    -F "file=@bhav_fno.csv" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=CLSA" \
    $SERVER/mtm-process

# 13. Process NAV
curl -v -X POST \
    -F "strategySymbol=CLSA" \
    -F "date=12/06/2026" \
    $SERVER/process-nav
