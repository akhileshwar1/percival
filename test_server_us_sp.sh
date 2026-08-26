#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "file=@Sp_Large_Cap/eq_strat_sp_large.csv" \
    $SERVER/create-strategy

# # create bill group
# curl -v -X POST \
#     -F "billSymbol=Biller" \
#     -F "hurdlerate=-50" \
#     -F "frequency=ANNIVERSARY" \
#     -F "perfFee=20" \
#     -F "date=21/03/2024" \
#     $SERVER/bill-group
#
# 1. Exchange Rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_inv_sp_large_Cap/eq_inv_vikas.csv" \
    $SERVER/add-investor

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@Sp_Large_Cap/eq_bank_sp_large.csv" \
    $SERVER/bank-transfer

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@Sp_Large_Cap/Eq_units/eq_units _vikas.csv" \
    $SERVER/allot-units

# 8. upload securities. 
curl -v -X POST \
    -F "file=@Sp_Large_Cap/security_sp_large.csv" \
    $SERVER/upload-securities

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_positions_sp_large.csv" \
    $SERVER/upload-positions

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_fno_positions_sp_large.csv" \
    $SERVER/upload-fno-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_balances_sp_large.csv" \
    $SERVER/upload-balances

# # corp action
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "date=16/06/2026" \
#     -F "file=@corp_action.csv" \
#     $SERVER/corporate-action
#
# 11. eq Bhavcopy


curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_bhav_1_sp_large.CSV" \
    $SERVER/price-update

# curl -v -X POST \
#     -F "strategySymbol=SSFSPUSOP" \
#     -F "date=01/04/2026" \
#     -F "file=@alpha_500/WO010426.CSV" \
#     $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "file=@Sp_Large_Cap/eq_bhav_1_sp_large.CSV" \
    $SERVER/price-update

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    $SERVER/mtm-process

curl -v -X POST \
    -F "strategySymbol=SSFSPUSOP" \
    -F "date=02/04/2026" \
    $SERVER/process-nav
# ===== here=======
# # 9. Fund Expense
# curl -v -X POST \
#     -F "file=@fund_expense.csv" \
#     $SERVER/fund-expense
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
