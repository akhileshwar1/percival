#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "file=@eq_strat.csv" \
    $SERVER/create-strategy

# 1. Exchange Rate
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@exchange_rate_eq.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@eq_inv.csv" \
    $SERVER/add-investor

# 3. Add Investor lalit
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@eq_inv_lalit.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@eq_inv_vikas.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@eq_subs_upa.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@eq_bank.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@eq_subs_upa_rev.csv" \
    $SERVER/reverse-upa

# 7. Fund Cashflow
curl -v -X POST \
    -F "file=@eq_cashflow.csv" \
    $SERVER/fund-cashflow

# 8. Unit Allotment
curl -v -X POST \
    -F "file=@eq_units.csv" \
    $SERVER/allot-units

# 8. Unit Allotment lalit
curl -v -X POST \
    -F "file=@eq_units_lalit.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@eq_units_vikas.csv" \
    $SERVER/allot-units

# 8. upload securities. 
curl -v -X POST \
    -F "file=@securities_alpha.csv" \
    $SERVER/upload-securities

# 8. upload old positons. 
curl -v -X POST \
    -F "strategySymbol=alpha" \
    -F "file=@eq_positions.csv" \
    $SERVER/upload-positions

# # corp action
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "date=16/06/2026" \
#     -F "file=@corp_action.csv" \
#     $SERVER/corporate-action
#
# # 11. eq Bhavcopy
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "file=@eq_bhav_16.csv" \
#     $SERVER/bhav-eq
#
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "date=16/06/2026" \
#     $SERVER/process-nav
#
# # 9. Fund Expense
# curl -v -X POST \
#     -F "file=@fund_expense.csv" \
#     $SERVER/fund-expense
#
# 10. F&O Trades
# curl -v -X POST \
#     -F "file=@ab_trades_21.csv" \
#     $SERVER/trades-fno
#
# #     # -F "strategySymbol=31500012A" \
# # 11. F&O Bhavcopy
# curl -v -X POST \
#     -F "strategySymbol=31500012A" \
#     -F "file=@ab_bhav_21.csv" \
#     $SERVER/bhav-fno
#
# # 12. MTM Process
# curl -v -X POST \
#     -F "strategySymbol=31500012A" \
#     $SERVER/mtm-process
#
# # 13. Process NAV
# curl -v -X POST \
#     -F "strategySymbol=31500012A" \
#     -F "date=21/03/2025" \
#     $SERVER/process-nav
