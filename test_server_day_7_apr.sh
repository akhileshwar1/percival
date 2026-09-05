#!/bin/bash

SERVER=http://localhost:8888
# SERVER=http://13.53.138.234:8888

curl -v -X POST \
    -F "date=07/04/2026" \
    -F "file=@07_april/336 Derivative Dhan Transactions Levitas Apr 07.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "date=07/04/2026" \
    -F "file=@07_april/336 Derivatives 7 April 2026.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "date=07/04/2026" \
    -F "file=@07_april/336 Ramani India 7 April 26.csv" \
    $SERVER/trades-fno

curl -v -X POST \
    -F "strategySymbol=B105S29B" \
    -F "date=07/04/2026" \
    -F "file=@07_april/WE070426.CSV" \
    $SERVER/bhav-eq

# sensex bhav
curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "date=07/04/2026" \
    -F "file=@07_apr/bse_bhav.CSV" \
    $SERVER/bse-bhav

# curl -v -X POST \
#     -F "strategySymbol=B105S29B" \
#     -F "file=@07_april/21_Price Update_TVSMNCRPS.csv" \
#     $SERVER/price-update

curl -v -X POST \
    -F "strategySymbol=B105S29B" \
    -F "date=07/04/2026" \
    -F "file=@07_april/WO070426.CSV" \
    $SERVER/bhav-fno

# 12. MTM Process
curl -v -X POST \
    -F "date=07/04/2026" \
    -F "strategySymbol=All" \
    $SERVER/mtm-process

 # to use 31st april's ex rate for fee accrual. Nav is also done on holidays. 
curl -v -X POST \
    -F "strategySymbol=B105S29B" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=B105S29C" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=B105S29D" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=B105S29E" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=31500012B" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=31500012A" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=31500012C" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

curl -v -X POST \
    -F "strategySymbol=S915F" \
    -F "date=07/04/2026" \
    $SERVER/process-nav

# 10. Trades
# curl -v -X POST \
#     -F "date=02/04/2026" \
#     -F "file=@07_april/eq_trades_alpha100.csv" \
#     $SERVER/trades-equity

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
