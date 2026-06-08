#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;
typedef uint16_t uint16;

#define MAX_STRATEGIES 100
#define MAX_INVESTORS 100
#define MAX_SECURITIES 100
#define MAX_POSITIONS 100
#define MAX_EX_RATES 10

typedef enum
{
    USD,
    INR
} Currency_code;

typedef struct
{
    Currency_code curr;
    char date[100];
    real64 rate;
    Currency_code base;
} Exchange_rate;

typedef struct
{
    char symbol[100];
    real64 ltp;
} Bhav;

typedef struct
{
    char symbol[100];
    char date[100];
    char name[100];
} Security;

typedef struct
{
    int id;
    char name[100];
    char inceptionDate[100];
    uint64 units;
} Investor;

typedef enum 
{
    MB, // market buy
    MS, // market sell
    LB, // limit buy
    LS  // limit sell
} Trans_type;

typedef struct
{
    char symbol[100];
    char brokerCode[100];
    char date[100];
    int qty;
    real64 price;
    real64 brokerage;
    real64 serviceTax;
    Trans_type transType;
} Trade;

typedef struct
{
    char symbol[100];
    int qty;
    real64 price;
    real64 ltp;
} Position;

typedef struct
{
    char symbol[100];
    real64 cash;
    real64 nav;
    Investor investors[MAX_INVESTORS];
    Position positions[MAX_POSITIONS];
    int currPosIndex;
    int currInvestorIndex;
} Strategy;

typedef enum
{
    ASSET,
    EXPENSE,
    LIABILITY,
    EQUITY,
    REVENUE
} LedgerEntryType;

typedef struct
{
    int id;
    LedgerEntryType type;
    char accountName[100];
    real64 debit;
    real64 credit;
    char memo[100];
} LedgerEntry;

typedef struct
{
    Exchange_rate exRates[MAX_EX_RATES];
    Security secs[MAX_SECURITIES];
    Strategy strategies[MAX_STRATEGIES];
    LedgerEntry ledger[1000];
    int currStratIndex;
    int currEntryId;
    int currJournalId;
} State;

void
LoadBhav(Bhav *bhav, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(bhav->symbol, token);
        }
        else if (i ==  5)
        {
            bhav->ltp = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadExchangeRate(Exchange_rate *exRate, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            exRate->curr = (strcmp(token, "USD") == 0) ? USD : INR;
        }
        else if (i ==  1)
        {
            strcpy(exRate->date, token);
        }
        else if (i ==  2)
        {
            exRate->rate = (real64)atof(token);
        }
        else if (i ==  3)
        {
            exRate->base = (strcmp(token, "USD") == 0) ? USD : INR;
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadSecurity(Security *sec, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(sec->symbol, token);
        }
        else if (i ==  1)
        {
            strcpy(sec->date, token);
        }
        else if (i ==  2)
        {
            strcpy(sec->name, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadStrategy(Strategy *strat, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(strat->symbol, token);
        }
        else if (i ==  4)
        {
            strat->cash = (real64)atof(token);
        }
        else if (i ==  6)
        {
            strat->nav = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
    strat->currInvestorIndex = -1;
}

void
LoadStrategyFromFile(Strategy *strat, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 3)
        {
            strcpy(strat->symbol, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
    strat->currInvestorIndex = -1;
    strat->nav = 100;
    strat->currPosIndex = -1;
    strat->currInvestorIndex = -1;
}

real64
getDollarValue(char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 5)
        {
            return (real64)atof(token); 
        }
        token = strtok(NULL, ",");
        i++;
    }
    return -1;
}

void
AccountFromReverse(LedgerEntry *liabEntry, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcat(accountName, token); 
        }
        else if (i ==  2)
        {
            strcat(accountName, token); 
            strcpy(liabEntry->accountName, accountName);
            liabEntry->type = LIABILITY;
        }
        else if (i == 7)
        {
            liabEntry->credit = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromExpense(LedgerEntry *assetEntry, LedgerEntry *liabEntry,
                char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcat(accountName, token); 
        }
        else if (i ==  2)
        {
            strcat(accountName, token); 
            strcpy(liabEntry->accountName, accountName);
            liabEntry->type = LIABILITY;
        }
        else if (i == 7)
        {
            strcpy(assetEntry->accountName, "Fund_expenses");
            assetEntry->type = EXPENSE;
            assetEntry->debit = (real64)atof(token);
            liabEntry->credit = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromBank(LedgerEntry *assetEntry, LedgerEntry *liabEntry,
                real64 dollarValue, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    while (token != NULL)
    {
        if (i ==  3)
        {
            strcat(accountName, token); 
        }
        else if (i ==  4)
        {
            strcat(accountName, token); 
            assetEntry->type = ASSET;
            assetEntry->debit = dollarValue;
            strcpy(assetEntry->accountName, accountName);
            strcpy(accountName, "");
        }
        else if (i == 7)
        {
            strcpy(accountName, token); 
        }
        else if (i ==  8)
        {
            strcat(accountName, token); 
            strcpy(liabEntry->accountName, accountName);
            liabEntry->type = LIABILITY;
            liabEntry->credit = dollarValue;
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromSubs(LedgerEntry *entry, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcpy(entry->accountName, token); 
        }
        else if (i == 7)
        {
            switch (entry->type)
            {
                case ASSET:
                case EXPENSE:
                    {
                        entry->debit = abs((real64)atof(token));
                        strcat(entry->accountName, "_cash_advisory"); 
                        break;
                    }
                case LIABILITY:
                case EQUITY:
                case REVENUE:
                    {
                        entry->credit = abs((real64)atof(token));
                        strcat(entry->accountName, "_inv_capital"); 
                        break;
                    }
            }
        }
        else if (i ==  18)
        {
            strcpy(entry->memo, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadInvestorFromClient(Investor *inv, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  5)
        {
            strcpy(inv->name, token);
        }
        else if (i ==  23)
        {
            strcpy(inv->inceptionDate, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadInvestor(Investor *inv, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 1)
        {
            inv->id = (uint64)atoi(token);
        }
        else if (i ==  2)
        {
            strcpy(inv->name, token);
        }
        else if (i ==  7)
        {
            inv->units = (uint64)atoi(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadTrade(Trade *trade, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(trade->symbol, token);
        }
        else if (i ==  1)
        {
            strcpy(trade->brokerCode, token);
        }
        else if (i ==  2)
        {
            if (strcmp(token, "MB") == 0) {
                trade->transType = MB;
            }
            else if (strcmp(token, "MS") == 0) {
                trade->transType = MS;
            }
            else if (strcmp(token, "LB") == 0) {
                trade->transType = LB;
            }
            else if (strcmp(token, "LS") == 0) {
                trade->transType = LS;
            }
        }
        else if (i == 3)
        {
            trade->qty = (uint64)atoi(token);
        }
        else if (i == 4)
        {
            trade->price = (real64)atof(token);
        }
        else if (i == 5)
        {
            trade->brokerage = (real64)atof(token);
        }
        else if (i == 6)
        {
            trade->serviceTax = (real64)atof(token);
        }
        else if (i == 7)
        {
            strcpy(trade->date, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
printFundLedger(State *state)
{
    // print the ledger.
    for (int i = 0; i < state->currEntryId + 1; i++)
    {
        LedgerEntry entry = state->ledger[i];
        printf("name: %s, type: %d, debit: %f, credit: %f\n",
               entry.accountName,
               entry.type,
               entry.debit,
               entry.credit);
    }
}

int
main()
{
    printf("hey there\n");
    FILE *exchangeRateFile = fopen("exchange_rate.csv", "r");
    if (exchangeRateFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    State state = {};
    state.currStratIndex = -1;
    state.currEntryId = -1;
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), exchangeRateFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Exchange_rate exRate = {};
        LoadExchangeRate(&exRate, line);
        state.exRates[i - 1] = exRate; // it's a copy here.
        printf("ex rate is %f\n", state.exRates[i - 1].rate);
    }

    FILE *securityFile = fopen("securities.csv", "r");
    if (securityFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    // upload the securities.
    i = 0;
    while (fgets(line, sizeof(line), securityFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Security sec = {};
        LoadSecurity(&sec, line);
        state.secs[i - 1] = sec;
        printf("security is %s\n", state.secs[i - 1].name);
    }

    // onboard investor and strategy if new.

    // step 0: create new strategy.
    FILE *stratFile = fopen("strategy_master.csv", "r");
    if (stratFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    Strategy strategy = {};
    while (fgets(line, sizeof(line), stratFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LoadStrategyFromFile(&strategy, line);
        state.strategies[++state.currStratIndex] = strategy;
    }

    // step 1: the client_master.csv file.
    Investor inv = {};
    FILE *clientFile = fopen("client_master.csv", "r");
    if (clientFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    while (fgets(line, sizeof(line), clientFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LoadInvestorFromClient(&inv, line);
    }

    // step 2: the subscription file with accounting.
    FILE *subsFile = fopen("subscription_upa.csv", "r");
    if (subsFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    ++state.currJournalId; // same id for the couple.
    while (fgets(line, sizeof(line), subsFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LedgerEntry entry = {};
        entry.id = state.currJournalId;
        if (i == 1) entry.type = ASSET;
        else if (i == 2) entry.type = EQUITY;
        AccountFromSubs(&entry, line);
        state.ledger[++state.currEntryId] = entry;
        printf("entry name is %s and value is %f\n", entry.accountName, entry.debit);
        i++;
    }

    // step 3: process the bank transfer.
    FILE *bankFile = fopen("bank_transfer.csv", "r");
    if (bankFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    real64 dollarValue;
    while (fgets(line, sizeof(line), bankFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        else if (i == 1)
        {
            char copyLine[1024];
            strcpy(copyLine, line);
            /* Need this because the later rows are not dollarised,
               since they transfer the money to indian inr accounts. */
            dollarValue = getDollarValue(copyLine);
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        ++state.currJournalId;
        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state.currJournalId;
        liabEntry.id = state.currJournalId;
        if (dollarValue == -1)
        {
            printf("bank transfer failed!, abort!\n");
            return -1;
        }
        AccountFromBank(&assetEntry, &liabEntry, dollarValue, line);
        state.ledger[++state.currEntryId] = assetEntry;
        state.ledger[++state.currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }

    // step 4: fund expense investor file.
    FILE *expenseFile = fopen("fund_expense.csv", "r");
    if (expenseFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    i = 0;
    while (fgets(line, sizeof(line), expenseFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        ++state.currJournalId;
        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state.currJournalId;
        liabEntry.id = state.currJournalId;
        AccountFromExpense(&assetEntry, &liabEntry, line);
        state.ledger[++state.currEntryId] = assetEntry;
        state.ledger[++state.currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }

    // step 5: reverse the upa debit account entry.
    FILE *reverseFile = fopen("reverse_upa.csv", "r");
    if (reverseFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    i = 0;
    while (fgets(line, sizeof(line), reverseFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        ++state.currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state.currJournalId;
        AccountFromReverse(&liabEntry, line);
        state.ledger[++state.currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }
    printFundLedger(&state);

    FILE *onboardFile = fopen("onboard_investor.csv", "r");
    if (onboardFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }


    i = 0;
    while (fgets(line, sizeof(line), onboardFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Strategy strat = {};
        /* NOTE(Akhil): strategies[0] implies we are working on
                    one strategy only */
        strat.currPosIndex = -1;
        Investor inv = {};
        // NOTE(akhil) : Same line being traversed twice.
        char copyLine[1024]; 
        strcpy(copyLine, line);
        LoadStrategy(&strat, line);
        LoadInvestor(&inv, copyLine);

        // add strategy to the state if not present.
        int found = 0;
        int foundIndex = -1;
        for (int i = 0; i < state.currStratIndex + 1; i++)
        {
            if (strcmp(state.strategies[i].symbol, strat.symbol) == 0)
            {
                found = 1;
                foundIndex = i;
                break;
            }
        }

        if (found != 1)
        {
            // add investor the strategy.
            strat.investors[++strat.currInvestorIndex] = inv;
            state.strategies[++state.currStratIndex] = strat;
        }
        else
        {
            strat = state.strategies[foundIndex];
            strat.investors[++strat.currInvestorIndex] = inv;
        }

        printf("strategy is %s\n", state.strategies[0].symbol);
        printf("investor is %ld\n", state.strategies[0].investors[0].units);
    }

    /* read the trades pertaining to a particular strategy
           and apply them to the position state. */
    FILE *TradesFile = fopen("trades.csv", "r");
    if (TradesFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    while (fgets(line, sizeof(line), TradesFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Trade trade = {};
        LoadTrade(&trade, line);
        // apply trade to the positions state.
        int found = 0;
        for (int i = 0; i < state.strategies[0].currPosIndex + 1; i++)
        {
            if (strcmp(state.strategies[0].positions[i].symbol, trade.symbol) == 0)
            {
                switch (trade.transType)
                {
                    case MB:
                    case LB:
                        {
                            // you pay more while buying.
                           real64 priceAfterFee =
                            (trade.qty * trade.price *
                            (1.0 + (trade.brokerage + trade.serviceTax) / 100.0))
                            / trade.qty; 


                            state.strategies[0].cash -= trade.qty * priceAfterFee;
                            // NOTE(Akhil): this will break if denom is 0!
                            state.strategies[0].positions[i].price =
                                ((state.strategies[0].positions[i].price *
                                state.strategies[0].positions[i].qty)
                                + (trade.qty * priceAfterFee)) 
                                / (state.strategies[0].positions[i].qty + trade.qty);

                            state.strategies[0].positions[i].qty += trade.qty;
                            break;
                        }

                    default:
                        {
                           printf("trade qty is %f\n", (real64)trade.qty);
                           real64 priceAfterFee =
                            (trade.qty * trade.price *
                            (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                            / trade.qty; 

                            // you always get less after selling.
                            state.strategies[0].cash -= trade.qty * priceAfterFee;
                            state.strategies[0].positions[i].price =
                                ((state.strategies[0].positions[i].price *
                                state.strategies[0].positions[i].qty) +
                                (trade.qty * priceAfterFee)) 
                                / (state.strategies[0].positions[i].qty + trade.qty);

                            state.strategies[0].positions[i].qty += trade.qty;
                            break;
                        }
                } 

                found = 1;
                break;
            }
        }
        
        // not found, add the position.
        if (found != 1)
        {
            // add the position
            printf("adding new position: %s\n", trade.symbol);
            Position pos = {};
            strcpy(pos.symbol, trade.symbol);
            switch(trade.transType)
            {
                case MB:
                case LB:
                    {
                        real64 priceAfterFee =
                            (trade.qty * trade.price *
                            (1.0 + (trade.brokerage + trade.serviceTax) / 100.0))
                            / trade.qty;
                        // you always pay more while buying.
                        state.strategies[0].cash -= trade.qty * priceAfterFee;
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        break;
                    }
                default:
                    {
                        real64 priceAfterFee =
                            (trade.qty * trade.price *
                            (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                            / trade.qty;
                        // you always get less after selling.
                        state.strategies[0].cash += trade.qty * priceAfterFee;
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        break;
                    }
            }
            state.strategies[0].positions[++state.strategies[0].currPosIndex] = pos;
        }
        printf("cash is %f\n", state.strategies[0].cash);
        printf("pos is %s, %d, %f\n",
               state.strategies[0].positions[0].symbol,
               state.strategies[0].positions[0].qty,
               state.strategies[0].positions[0].ltp);
        printf("pos is %s, %d, %f\n",
               state.strategies[0].positions[1].symbol,
               state.strategies[0].positions[1].qty,
               state.strategies[0].positions[1].ltp);

    }

    //upload the bhavcopy.
    FILE *BhavFile = fopen("bhavcopy.csv", "r");
    if (BhavFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    i = 0;
    while (fgets(line, sizeof(line), BhavFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Bhav bhav = {};
        LoadBhav(&bhav, line);
        for (int i = 0; i < state.strategies[0].currPosIndex + 1; i++)
        {
            if (strcmp(bhav.symbol, state.strategies[0].positions[i].symbol) == 0)
            {
                state.strategies[0].positions[i].ltp = bhav.ltp;
            }
        }
        printf("cash after bhav is %f\n", state.strategies[0].cash);
        printf("pos after bhav is %s, %d, %f\n",
               state.strategies[0].positions[0].symbol,
               state.strategies[0].positions[0].qty,
               state.strategies[0].positions[0].ltp);
        printf("pos after bhav is %s, %d, %f\n",
               state.strategies[0].positions[1].symbol,
               state.strategies[0].positions[1].qty,
               state.strategies[0].positions[1].ltp);
    }

    // get total value of the positions held for the strategy.
    real64 totalValue = 0.0;
    for (int i = 0; i < state.strategies[0].currPosIndex + 1; i++)
    {
        Position pos = state.strategies[0].positions[i];
        totalValue  += pos.qty * pos.ltp;
    }

    // get the total units from all the investors for a strategy.
    uint64 totalUnits = 0;
    for (int i = 0; i < state.strategies[0].currInvestorIndex + 1; i++)
    {
        Investor inv = state.strategies[0].investors[i];
        totalUnits += inv.units;
    }

    // calculate the nav = (totalValue + cash) / totalUnits.
    real64 cash = state.strategies[0].cash;
    real64 nav = (totalValue + cash) / totalUnits;
    printf("nav is %f\n", nav);
}
