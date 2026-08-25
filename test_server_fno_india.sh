#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "file=@alpha_India_FNO/eq_strat_India_FNO.csv" \
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
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/exchange_rate_India_FNO.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_inv_India_FNO/eq_inv_Lalit.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_inv_India_FNO/eq_inv_AdityaS.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_inv_India_FNO/eq_inv_TusharG.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_inv_India_FNO/eq_inv_AnandJ.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_inv_India_FNO/eq_inv_PawanN.csv" \
    $SERVER/add-investor


# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/UPA_Alpha_India_FNO/eq_subs_upa_lalit.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/UPA_Alpha_India_FNO/eq_subs_upa_AnandJ.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/UPA_Alpha_India_FNO/eq_subs_upa_TusharG.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/UPA_Alpha_India_FNO/eq_subs_upa_AdityaS.csv" 
    $SERVER/subs-upa


# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/UPA_Alpha_India_FNO/eq_subs_upa_PawanN.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@alpha_India_FNO/eq_bank_India_FNO.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_FNO/UPA_Rev_Alpha_India_FNO/eq_subs_upa_rev_Lalit.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_FNO/UPA_Rev_Alpha_India_FNO/eq_subs_upa_rev_Anand.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_FNO/UPA_Rev_Alpha_India_FNO/eq_subs_upa_rev_Pawan_N.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_FNO/UPA_Rev_Alpha_India_FNO/eq_subs_upa_rev_TusharG.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_India_FNO/UPA_Rev_Alpha_India_FNO/eq_subs_upa_rev_AdityaS.csv" \
    $SERVER/reverse-upa

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@alpha_India_FNO/Eq_units/eq_units _lalit.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_India_FNO/Eq_units/eq_units _AnandJ.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_India_FNO/Eq_units/eq_units _PawanN.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_India_FNO/Eq_units/eq_units _AdityaS.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_India_FNO/Eq_units/eq_units _TusharG.csv" \
    $SERVER/allot-units

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_fno_positions_India_FNO.csv" \
    $SERVER/upload-fno-positions

curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_bond_positions_alpha_FNO.csv" \
    $SERVER/upload-bond-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "file=@alpha_India_FNO/eq_balances_INDIA_FNO.csv" \
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
#     -F "strategySymbol=B105S29E" \
#     -F "date=01/04/2026" \
#     -F "file=@alpha_India_FNO/eq_bhav_1_alpha_India_FNO.CSV" \
#     $SERVER/bhav-eq
#
curl -v -X POST \
    -F "file=@alpha_India_FNO/eq_trades_India_FNO_.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "date=01/04/2026" \
    -F "file=@alpha_India_FNO/eq_bhav_1_alpha_India_FNO.CSV" \
    $SERVER/bhav-fno

# sensex bhav
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "date=01/04/2026" \
    -F "file=@alpha_India_FNO/bhav_sensex_2_apr.CSV" \
    $SERVER/bse-bhav

# 12. MTM Process
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

# curl -v -X POST \
#     -F "strategySymbol=B105S29E" \
#     -F "file=@alpha_India_FNO/21_Price Update_TVSMNCRPSS.csv" \
#     $SERVER/price-update
#
# curl -v -X POST \
#     -F "strategySymbol=B105S29E" \
#     -F "date=02/04/2026" \
#     -F "file=@alpha_500/WO020426.CSV" \
#     $SERVER/bhav-fno
#
# # 12. MTM Process
# curl -v -X POST \
#     -F "strategySymbol=B105S29E" \
#     $SERVER/mtm-process
#
# curl -v -X POST \
#     -F "strategySymbol=B105S29E" \
#     -F "date=02/04/2026" \
#     $SERVER/process-nav
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
