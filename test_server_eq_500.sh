#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

# 2. Create Strategy
curl -v -X POST \
    -F "file=@alpha_500/eq_strat_alpha_500.csv" \
    $SERVER/create-strategy

# create bill group
curl -v -X POST \
    -F "billSymbol=Biller" \
    -F "hurdlerate=-50" \
    -F "frequency=ANNIVERSARY" \
    -F "perfFee=20" \
    -F "date=21/03/2024" \
    $SERVER/bill-group

# 1. Exchange Rate
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/exchange_rate_alpha_500.csv" \
    $SERVER/exchange-rate

# 3. Add Investor
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/eq_inv_alpha_500/eq_inv_Lalit.csv" \
    $SERVER/add-investor

# 3. Add Investor lalit
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/eq_inv_alpha_500/eq_inv_SB_ADV.csv" \
    $SERVER/add-investor

# 3. Add Investor vikas 
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/eq_inv_alpha_500/eq_inv_Vikas_J.csv" \
    $SERVER/add-investor

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/UPA_Alpha_500/eq_subs_upa_lalit.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/UPA_Alpha_500/eq_subs_upa_Sb_adv.csv" \
    $SERVER/subs-upa

# 4. Subscription UPA
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/UPA_Alpha_500/eq_subs_upa_vikas_J.csv" \
    $SERVER/subs-upa

# 5. Bank Transfer
curl -v -X POST \
    -F "file=@alpha_500/eq_bank_alpha_500.csv" \
    $SERVER/bank-transfer

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_500/UPA_Rev_Alpha_500/eq_subs_upa_rev_Lalit.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_500/UPA_Rev_Alpha_500/eq_subs_upa_rev_Sb_adv.csv" \
    $SERVER/reverse-upa

# 6. Reverse UPA
curl -v -X POST \
    -F "file=@alpha_500/UPA_Rev_Alpha_500/eq_subs_upa_rev_Vikas_J.csv" \
    $SERVER/reverse-upa

# # 7. Fund Cashflow
# curl -v -X POST \
#     -F "file=@eq_cashflow.csv" \
#     $SERVER/fund-cashflow
#
# 8. Unit Allotment
curl -v -X POST \
    -F "file=@alpha_500/Eq_units/eq_units _lalit.csv" \
    $SERVER/allot-units

# 8. Unit Allotment lalit
curl -v -X POST \
    -F "file=@alpha_500/Eq_units/eq_units_SB_adv.csv" \
    $SERVER/allot-units

# 8. Unit Allotment vikas 
curl -v -X POST \
    -F "file=@alpha_500/Eq_units/eq_units_Vikas_J.csv" \
    $SERVER/allot-units

# 8. upload securities. 
curl -v -X POST \
    -F "file=@alpha_500/security_alpha_500.csv" \
    $SERVER/upload-securities


# 8. upload old positons. 
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/eq_positions_alpha_500.csv" \
    $SERVER/upload-positions

# 8. upload balances. 
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "file=@alpha_500/eq_balances_alpha_500.csv" \
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
#     -F "strategySymbol=B105S29D" \
#     -F "file=@alpha_500/eq_bhav_1_alpha_500.csv" \
#     $SERVER/bhav-eq
#
#  # to use 2nd april's ex rate for fee accrual. Nav is inconsequential
# curl -v -X POST \
#     -F "strategySymbol=B105S29D" \
#     -F "date=02/04/2026" \
#     $SERVER/process-nav
#
# 10. Trades
curl -v -X POST \
    -F "file=@alpha_500/eq_trades_alpha500.csv" \
    $SERVER/trades-eq

# 11. eq Bhavcopy
curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "date=02/04/2026" \
    -F "file=@alpha_500/Bhavcopies/WE020426.CSV" \
    $SERVER/bhav-eq

curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "date=02/04/2026" \
    $SERVER/process-nav

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
