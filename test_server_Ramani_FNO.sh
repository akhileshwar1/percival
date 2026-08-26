#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/eq_strat_Active_FNO.csv" \
    $SERVER/create-strategy

# create bill group
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
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/exchange_rate_India_FNO.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Badar.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Beena.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Charanjeet.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Mufazal.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Murtaza.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_inv_Ramani_FNO/eq_inv_Ramani.csv" \
    $SERVER/add-investor

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/eq_bank_Ramani_FNO.csv" \
    $SERVER/bank-transfer

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _Badar.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _Beena.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _charanjeet.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _Muffzal.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _Murtaza.csv" \
    $SERVER/allot-units

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/Eq_units/eq_units _Ramani.csv" \
    $SERVER/allot-units


curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_fno_positions_Ramani_FNO.csv" \
    $SERVER/upload-fno-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "file=@alpha_Ramani_FNO/eq_balances_Ramani_FNO.csv" \
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
#     -F "strategySymbol=S915F" \
#     -F "date=01/04/2026" \
#     -F "file=@alpha_Ramani_FNO/eq_bhav_1_alpha_Ramani_FNO.CSV" \
#     $SERVER/bhav-eq
#

curl -v -X POST \
    -F "file=@alpha_Ramani_FNO/eq_trades_Ramani_FNO.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=01/04/2026" \
    -F "file=@alpha_Ramani_FNO/eq_bhav_1_Ramani_FNO.CSV" \
    $SERVER/bhav-fno

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=01/04/2026" \
    -F "file=@alpha_Ramani_FNO/SENSEX_20260401.CSV" \
    $SERVER/bse-bhav

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=S915F" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

# curl -v -X POST \
#     -F "strategySymbol=S915F" \
#     -F "file=@alpha_Ramani_FNO/21_Price Update_TVSMNCRPSS.csv" \
#     $SERVER/price-update
#
curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=02/04/2026" \
    -F "file=@alpha_500/WO020426.CSV" \
    $SERVER/bhav-fno

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=01/04/2026" \
    -F "file=@alpha_Ramani_FNO/SENSEX_20260402.CSV" \
    $SERVER/bse-bhav

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=S915F" \
    $SERVER/mtm-process

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=02/04/2026" \
    $SERVER/process-nav
# ===== here=======
# # 9. Fund Expense
# curl -v -X POST \
#     -F "file=@fund_expense.csv" \
#     $SERVER/fund-expense
#
# #     # -F "strategySymbol=S915F" \
# # 11. F&O Bhavcopy
# curl -v -X POST \
#     -F "strategySymbol=S915F" \
#     -F "file=@ab_bhav_21.csv" \
#     $SERVER/bhav-fno
#
# # 12. MTM Process
# curl -v -X POST \
#     -F "strategySymbol=S915F" \
#     $SERVER/mtm-process
#
# # 13. Process NAV
# curl -v -X POST \
#     -F "strategySymbol=S915F" \
#     -F "date=21/03/2025" \
#     $SERVER/process-nav
