#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "isUSD=1" \
    -F "file=@US_ramani/eq_strat_us_active.csv" \
    $SERVER/create-strategy

# create bill group
curl -v -X POST \
    -F "billSymbol=Biller" \
    -F "hurdlerate=-50" \
    -F "frequency=ANNIVERSARY" \
    -F "perfFee=20" \
    -F "mgmtFee=1.5" \
    -F "date=21/03/2024" \
    $SERVER/bill-group

# 1. Exchange Rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_inv_us_active/eq_inv_beena.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_inv_us_active/eq_inv_muffazal.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_inv_us_active/eq_inv_murtaza.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_inv_us_active/eq_inv_ramani.csv" \
    $SERVER/add-investor


# 5. Bank Transfer
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "file=@US_ramani/eq_bank_us_active.csv" \
    $SERVER/bank-transfer

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@US_ramani/Eq_units/eq_units _beena.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@US_ramani/Eq_units/eq_units _muffazal.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@US_ramani/Eq_units/eq_units _murtaza.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@US_ramani/Eq_units/eq_units _ramani.csv" \
    $SERVER/allot-units



# 8. upload securities. 
curl -v -X POST \
    -F "file=@US_ramani/security_us_active.csv" \
    $SERVER/upload-securities

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_positions_us_active.csv" \
    $SERVER/upload-positions

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_fno_positions_us_active.csv" \
    $SERVER/upload-fno-positions


# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_balances_us_active.csv" \
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
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_bhav_1_us_active.CSV" \
    $SERVER/price-update

# curl -v -X POST \
#     -F "strategySymbol=SSF000068" \
#     -F "date=01/04/2026" \
#     -F "file=@alpha_500/WO010426.CSV" \
#     $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "strategySymbol=SSF000068" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
    -F "file=@US_ramani/eq_bhav_2_us_active.CSV" \
    $SERVER/price-update

# 12. MTM Process
curl -v -X POST \
    -F "date=02/04/2026" \
    -F "strategySymbol=SSF000068" \
    $SERVER/mtm-process

curl -v -X POST \
    -F "strategySymbol=SSF000068" \
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
