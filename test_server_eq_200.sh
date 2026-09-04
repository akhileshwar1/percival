#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "isUSD=0" \
    -F "file=@alpha_200/eq_strat_alpha_200.csv" \
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
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/exchange_rate_alpha_200.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_Lalit.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_Aditya_S.csv" \
    $SERVER/add-investor

curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_TusharG.csv" \
    $SERVER/add-investor

# 3. Add Investor lalit
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_SB_ADV.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_VikasJ.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_inv_alpha_200/eq_inv_PawanN.csv" \
    $SERVER/add-investor


# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_lalit.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_SB_Adv.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_VikasJ.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_TusharG.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_AdityaS.csv" 
    $SERVER/subs-upa


# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/UPA_Alpha_200/eq_subs_upa_PawanN.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "file=@alpha_200/eq_bank_alpha_200.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_Lalit.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_SB_ADV.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_VikasJ.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_PawanN.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_TusharG.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_200/UPA_Rev_Alpha_200/eq_subs_upa_rev_AdityaS.csv" \
    $SERVER/reverse-upa

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _lalit.csv" \
    $SERVER/allot-units

# 8. Unit Allotment lalit
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _SB_ADV.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _VikasJ.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _PawanN.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _AdityaS.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_200/Eq_units/eq_units _TusharG.csv" \
    $SERVER/allot-units

# 8. upload securities. 
curl -v -X POST \
    -F "file=@alpha_200/security_alpha_200.csv" \
    $SERVER/upload-securities

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_positions_alpha_200.csv" \
    $SERVER/upload-positions

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_fno_positions_alpha_200.csv" \
    $SERVER/upload-fno-positions

# 8. upload old positions. 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_bond_positions_alpha_200.csv" \
    $SERVER/upload-bond-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "file=@alpha_200/eq_balances_alpha_200.csv" \
    $SERVER/upload-balances

# # corp action
# curl -v -X POST \
#     -F "strategySymbol=alpha" \
#     -F "date=16/06/2026" \
#     -F "file=@corp_action.csv" \
#     $SERVER/corporate-action
#
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "file=@alpha_200/eq_trades_alpha200_1apr.csv" \
    $SERVER/trades-equity

# 11. eq Bhavcopy
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=01/04/2026" \
    -F "file=@alpha_200/eq_bhav_1_alpha_200.CSV" \
    $SERVER/bhav-eq

curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=01/04/2026" \
    -F "file=@alpha_500/WO010426.CSV" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=01/04/2026" \
    -F "strategySymbol=B105S29C" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=01/04/2026" \
    $SERVER/process-nav

# 10. Trades
curl -v -X POST \
    -F "date=02/04/2026" \
    -F "file=@alpha_200/eq_trades_alpha200_2apr.csv" \
    $SERVER/trades-equity

curl -v -X POST \
    -F "date=02/04/2026" \
    -F "file=@alpha_200/eq_trades_alpha_200_FNO.csv" \
    $SERVER/trades-fno

# 11. eq Bhavcopy
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=02/04/2026" \
    -F "file=@alpha_500/Bhavcopies/WE020426.CSV" \
    $SERVER/bhav-eq

# curl -v -X POST \
#     -F "strategySymbol=B105S29C" \
#     -F "file=@alpha_200/21_Price Update_TVSMNCRPSS.csv" \
#     $SERVER/price-update
#
curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=02/04/2026" \
    -F "file=@alpha_500/WO020426.CSV" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=02/04/2026" \
    -F "strategySymbol=B105S29C" \
    $SERVER/mtm-process

curl -v -X POST \
    -F "strategySymbol=B105S29C" \
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
