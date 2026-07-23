#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.186.189:8888

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
    -F "file=@error.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@ab_subs_upa.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@ab_bank.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@ab_subs_upa_rev.csv" \
    $SERVER/reverse-upa

# 7. Fund Cashflow
curl -v -X POST \
    -F "file=@ab_cashflow.csv" \
    $SERVER/fund-cashflow

# 8. Unit Allotment
curl -v -X POST \
    -F "file=@ab_units.csv" \
    $SERVER/allot-units

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@ab_off_cashflow.csv" \
#     $SERVER/offboard-cashflow
#
# 9. Fund Expense
curl -v -X POST \
    -F "file=@fund_expense.csv" \
    $SERVER/fund-expense

# 10. F&O Trades
curl -v -X POST \
    -F "file=@ab_trades_21.csv" \
    $SERVER/trades-fno

#     # -F "strategySymbol=31500012A" \
# 11. F&O Bhavcopy
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@ab_bhav_21.csv" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    $SERVER/mtm-process

# 13. Process NAV
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=21/03/2025" \
    $SERVER/process-nav


# 2ND DAY -24th -----------------------------------------------------
# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate_24.csv" \
    $SERVER/exchange-rate

curl -v -X POST \
    -F "file=@ab_trades_24.csv" \
    $SERVER/trades-fno

# 11. F&O Bhavcopy
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@ab_bhav_24.csv" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    $SERVER/mtm-process

# 13. Process NAV
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=24/03/2025" \
    $SERVER/process-nav

# 4TH DAY - 25th-----------------------------------------------------
# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate_25.csv" \
    $SERVER/exchange-rate

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@WO250325.CSV" \
    $SERVER/bhav-fno

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=25/03/2025" \
    $SERVER/process-nav

# 5TH DAY - 26th-----------------------------------------------------
# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate_26.csv" \
    $SERVER/exchange-rate

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@WO260325.CSV" \
    $SERVER/bhav-fno

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=26/03/2025" \
    $SERVER/process-nav

# 5TH DAY - 27th-----------------------------------------------------
# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate_27.csv" \
    $SERVER/exchange-rate

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "file=@WO270325.CSV" \
    $SERVER/bhav-fno

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=27/03/2025" \
    $SERVER/process-nav



# # 6TH DAY -28th -----------------------------------------------------
# 1. Exchange Rate
curl -v -X POST \
    -F "file=@exchange_rate_28.csv" \
    $SERVER/exchange-rate

curl -v -X POST \
    -F "file=@ab_trades_28.csv" \
    $SERVER/trades-fno

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    $SERVER/mtm-process

# 13. Process NAV
curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=28/03/2025" \
    $SERVER/process-nav
