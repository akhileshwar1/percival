#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "isUSD=0" \
    -F "file=@alpha_India_Combined/eq_strat_India_Combined.csv" \
    $SERVER/create-strategy

# # create bill group
curl -v -X POST \
    -F "billSymbol=Biller" \
    -F "hurdlerate=-50" \
    -F "frequency=ANNIVERSARY" \
    -F "perfFee=20" \
    -F "mgmtFee=1" \
    -F "date=21/03/2024" \
    $SERVER/bill-group

# 1. Exchange Rate
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "file=@alpha_India_Combined/exchange_rate_India_Combined.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "file=@alpha_India_Combined/eq_inv_India_Combined/eq_inv_Hemant_N.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "file=@alpha_India_Combined/UPA_Alpha_India_Combined/eq_subs_upa_hemantN.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "file=@alpha_India_Combined/eq_bank_India_Combined.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_Combined/UPA_Rev_Alpha_India_Combined/eq_subs_upa_rev_Hemant_N.csv" \
    $SERVER/reverse-upa

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@alpha_India_Combined/Eq_units/eq_units _Hemant N.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "file=@alpha_India_Combined/eq_fno_positions_India_Combined.csv" \
    $SERVER/upload-fno-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "file=@alpha_India_Combined/eq_balances_India_Combined.csv" \
    $SERVER/upload-balances

# # corp action
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "date=16/06/2026" \
#     -F "file=@corp_action.csv" \
#     $SERVER/corporate-action
#
# 11. eq Bhavcopy
# curl -v -X POST \
#     -F "strategySymbol=31500012C" \
#     -F "date=01/04/2026" \
#     -F "file=@alpha_India_Combined/eq_bhav_1_alpha_India_Combined.CSV" \
#     $SERVER/bhav-eq
#

curl -v -X POST \
    -F "date=01/04/2026" \
    -F "file=@alpha_India_Combined/eq_trades_India_Combined.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "date=01/04/2026" \
    -F "file=@alpha_India_Combined/eq_bhav_1_alpha_India_Combined.CSV" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "strategySymbol=31500012C" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

# curl -v -X POST \
#     -F "strategySymbol=31500012C" \
#     -F "file=@alpha_India_Combined/21_Price Update_TVSMNCRPSS.csv" \
#     $SERVER/price-update
#
curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "date=02/04/2026" \
    -F "file=@alpha_500/WO020426.CSV" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=02/04/2026" \
    -F "strategySymbol=31500012C" \
    $SERVER/mtm-process

curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "date=02/04/2026" \
    $SERVER/process-nav
# ===== here=======
# # 9. Fund Expense
# curl -v -X POST \
#     -F "file=@fund_expense.csv" \
#     $SERVER/fund-expense
#
# #     # -F "strategySymbol=31500012C" \
# # 11. F&O Bhavcopy
# curl -v -X POST \
#     -F "strategySymbol=31500012C" \
#     -F "file=@ab_bhav_21.csv" \
#     $SERVER/bhav-fno
#
# # 12. MTM Process
# curl -v -X POST \
#     -F "strategySymbol=31500012C" \
#     $SERVER/mtm-process
#
# # 13. Process NAV
# curl -v -X POST \
#     -F "strategySymbol=31500012C" \
#     -F "date=21/03/2025" \
#     $SERVER/process-nav
