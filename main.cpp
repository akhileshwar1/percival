#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <libpq-fe.h>

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
    char isin[100];
    char symbol[100];
    char date[100];
    char name[100];
    char id[100];
} Security;

typedef struct
{
    int id;
    char name[100];
    char inceptionDate[100];
    real64 units;
} Investor;

typedef struct
{
    char symbol[100];
    real64 balance;
    Currency_code currency;
} Bank_account;

typedef enum 
{
    MB, // market buy
    MS, // market sell
    LB, // limit buy
    LS,  // limit sell
    MOB,
    MCB,
    MOS,
    MCS,
    FSO,
    FSC,
    FBO,
    FBC
} Trans_type;

typedef enum 
{
    PE,
    CE,
    NA
} Opt_type;

typedef enum
{
    OPTIDX,
    OPTSTK,
    FUTIDX,
    FUTSTK
} Instrument_type;

typedef struct
{
    char symbol[100];
    char brokerCode[100];
    char date[100];
    char strategySymbol[100];
    char expiry[100];
    Opt_type optType;
    Instrument_type instType;
    int qty;
    real64 price;
    real64 brokerage;
    real64 serviceTax;
    real64 strike;
    Trans_type transType;
    Currency_code currency;
} FNO_trade;

typedef struct
{
    char symbol[100];
    real64 ltp;
    char expiry[100];
    real64 strike;
    Opt_type optType;
    Instrument_type instType;
} FNO_bhav;

typedef struct
{
    char symbol[100]; // is the isin in our trade file.
    char brokerCode[100];
    char date[100];
    char strategySymbol[100];
    int qty;
    real64 price;
    real64 brokerage;
    real64 serviceTax;
    Trans_type transType;
    Currency_code currency;
} Trade;

typedef struct
{
    char symbol[100];
    char date[100];
    real64 price;
} PriceUpdate;

typedef struct
{
    char isin[100];
    char symbol[100];
    int qty;
    real64 price;
    real64 ltp;
    char id[100];
} PositionEquity;

typedef struct
{
    char symbol[100];
    int qty;
    real64 price;
    real64 ltp;
    char expiry[100];
    real64 strike;
    Opt_type optType;
    Instrument_type instType;
    char id[100];
} FNO_position;

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
    Currency_code currency;
} LedgerEntry;

typedef struct
{
    char symbol[100];
    real64 cash;
    real64 nav;
    Investor investors[MAX_INVESTORS];
    PositionEquity positions[MAX_POSITIONS];
    FNO_position fpositions[MAX_POSITIONS];
    int id;
    int currPosIndex;
    int currFPosIndex;
    int currInvestorIndex;
    LedgerEntry ledger[100];
    int currEntryId;
    int currJournalId;
} Strategy;

typedef struct
{
    Exchange_rate exRates[MAX_EX_RATES];
    Security secs[MAX_SECURITIES];
    Strategy strategies[MAX_STRATEGIES];
    int currStratIndex;
    int currSecIndex;
    int idCount;
} State;

const char* LedgerEntryTypeStrings[] = {
    "ASSET",
    "EXPENSE",
    "LIABILITY",
    "EQUITY",
    "REVENUE"
};

int get_month_number(const char *month_str) {
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    
    for (int i = 0; i < 12; i++) {
        if (strcasecmp(month_str, months[i]) == 0) {
            return i + 1; // Months are 1-indexed (01-12)
        }
    }
    return -1; // Return -1 if the month is invalid
}

int convert_date_format(const char *input_date, char *output_date) {
    int day, year;
    char month_str[4]; // 3 chars + null terminator

    // Parse the input string (e.g., "30-Jun-2026")
    if (sscanf(input_date, "%d-%3[^-]-%d", &day, month_str, &year) != 3) {
        return -1; // Parsing failed
    }

    int month = get_month_number(month_str);
    if (month == -1) {
        return -1; // Invalid month string
    }

    sprintf(output_date, "%02d/%02d/%04d", day, month, year);
    return 0; // Success
}

void
LoadFNOBhav(FNO_bhav *bhav, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            if (strcmp(token, "OPTIDX") == 0)
            {
                bhav->instType = OPTIDX;
            }
            else if (strcmp(token, "OPTSTK") == 0)
            {
                bhav->instType = OPTSTK;
            }
            else if (strcmp(token, "FUTIDX") == 0)
            {
                bhav->instType = FUTIDX;
            }
            else if (strcmp(token, "FUTSTK") == 0)
            {
                bhav->instType = FUTSTK;
            }
        }
        else if (i == 1)
        {
            strcpy(bhav->symbol, token);
        }
        else if (i == 2)
        {
            char output[11]; // "DD/MM/YYYY" requires 10 chars + 1 for null terminator

            if (convert_date_format(token, output) == 0) {
                // printf("Original: %s\n", token);
                // printf("Converted: %s\n", output);
                strcpy(bhav->expiry, output);
            }
        }
        else if (i == 3)
        {
            bhav->strike = (real64)atof(token);
        }
        else if (i == 4)
        {
            if (strcmp(token, "PE") == 0)
            {
                bhav->optType = PE;
            }
            else if (strcmp(token, "CE") == 0)
            {
                bhav->optType = CE;
            }
            else
            {
                bhav->optType = NA;
            }
        }
        else if (i ==  8)
        {
            bhav->ltp = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadBhav(Bhav *bhav, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 3)
        {
            strcpy(bhav->symbol, token);
        }
        else if (i ==  9)
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
LoadSecurity(Security *sec, State *state, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(sec->isin , token);
        }
        else if (i == 1)
        {
            strcpy(sec->symbol, token);
        }
        else if (i ==  2)
        {
            strcpy(sec->date, token);
        }
        else if (i ==  3)
        {
            strcpy(sec->name, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
    if (strcmp(sec->isin, "FUT") == 0)
    {
        strcat(sec->id, "FUT_");
    }
    else
    {
        strcat(sec->id, "SEC_");
    }
    char str[12];
    snprintf(str, sizeof(str), "%d", ++state->idCount);
    strcat(sec->id, str);
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
    strat->currFPosIndex = -1;
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
allotUnits(State *state, PGconn *conn, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char stratName[100] = "";
    char invName[100] = "";
    while (token != NULL)
    {
        if (i == 1)
        {
            strcpy(invName, token); 
            printf("inv name is %s\n", invName);
        }
        else if (i == 2)
        {
            strcpy(stratName, token); 
            printf("strat name is %s %s\n", stratName, state->strategies[1].symbol);
        }
        else if (i ==  7)
        {
            real64 units = (real64)atof(token); 
            for (int j = 0; j < state->currStratIndex + 1; j++)
            {
                if (strcmp(stratName, state->strategies[j].symbol) == 0)
                {
                    // find the investor and allot units.
                    for (int k = 0;
                         k < state->strategies[j].currInvestorIndex + 1;
                         k++)
                        {
                        if (strcmp(invName, state->strategies[j].investors[k].name)
                            == 0)
                        {
                            state->strategies[j].investors[k].units = units;
                            printf("units are %f\n",
                                   state->strategies[j].investors[k].units);
                        }
                    }
                }
            }
            // persist the units too.
            char query[1024];
            snprintf(query, sizeof(query),
                     "UPDATE investor SET units = %f WHERE name = '%s'",
                     units,
                     invName
                     );
            PGresult *pgResult = PQexec(conn, query);
            char *errorMessage = PQresultErrorMessage(pgResult);
            if (strcmp(errorMessage, "") != 0)
            {
                printf("%s", errorMessage);
            }
            PQclear(pgResult);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromCashFlow(LedgerEntry *assetEntry, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    while (token != NULL)
    {
        if (i ==  0)
        {
            strcat(accountName, token); 
            strcat(accountName, "_CASH_USD"); 
            strcpy(assetEntry->accountName, accountName);
        }
        else if (i == 3)
        {
            assetEntry->debit = (real64)atof(token);
            assetEntry->currency = USD;
            assetEntry->type = ASSET;
        }
        token = strtok(NULL, ",");
        i++;
    }
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
            strcat(accountName, "_"); 
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
            liabEntry->currency = USD;
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
            strcat(accountName, "_"); 
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
            assetEntry->currency = USD;
            liabEntry->credit = (real64)atof(token);
            liabEntry->currency = USD;
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromBank(LedgerEntry *assetEntry, LedgerEntry *liabEntry,
                char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    real64 prevValue;
    Currency_code prevDenom = USD;
    while (token != NULL)
    {
        if (i ==  3)
        {
            strcat(accountName, token); 
            strcat(accountName, "_"); 
        }
        else if (i ==  4)
        {
            strcat(accountName, token); 
            assetEntry->type = ASSET;
            strcpy(assetEntry->accountName, accountName);
        }
        else if (i ==  5)
        {
            assetEntry->debit = (real64)atof(token);
            assetEntry->currency = prevDenom;
            strcpy(assetEntry->accountName, accountName);
            prevValue = assetEntry->debit;
        }
        else if (i ==  6)
        {
            liabEntry->credit = (real64)atof(token);
            if (prevValue != liabEntry->credit)
            {
                liabEntry->currency = INR;
                prevDenom = INR;
            }
            else
            {
                liabEntry->currency = USD;
                prevDenom = USD;
            }
            strcpy(assetEntry->accountName, accountName);
        }
        else if (i == 7)
        {
            strcpy(accountName, token); 
        }
        else if (i ==  8)
        {
            strcat(accountName, "_"); 
            strcat(accountName, token); 
            strcpy(liabEntry->accountName, accountName);
            liabEntry->type = LIABILITY;
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromSubs(LedgerEntry *entry, State *state, Investor inv,
                char *symbol, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcpy(entry->accountName, token); 
            for (int j = 0; j < state->currStratIndex + 1; j++)
            {
                // will only trigger for the second line in the csv, the upa line.
                if (strcmp(token, state->strategies[j].symbol) == 0)
                {
                    printf("found strategy\n");
                    state->strategies[j]
                        .investors[++state->strategies[j].currInvestorIndex] = inv;
                    strcpy(symbol, token);
                }
            }
        }
        else if (i == 7)
        {
            switch (entry->type)
            {
                case ASSET:
                case EXPENSE:
                    {
                        entry->debit = abs((real64)atof(token));
                        entry->currency = USD;
                        strcat(entry->accountName, "_UPA"); 
                        break;
                    }
                case LIABILITY:
                case EQUITY:
                case REVENUE:
                    {
                        entry->credit = abs((real64)atof(token));
                        entry->currency = USD;
                        strcat(entry->accountName, "_CASH_USD"); 
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
        if (i ==  3)
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
            inv->units = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadFNOTrade(FNO_trade *trade, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  0)
        {
            strcpy(trade->brokerCode, token);
        }
        else if (i ==  1)
        {
            strcpy(trade->strategySymbol, token);
        }
        else if (i == 2)
        {
            strcpy(trade->symbol, token);
        }
        else if (i ==  4)
        {
            if (strcmp(token, "MOB") == 0) {
                trade->transType = MOB;
            }
            else if (strcmp(token, "MCB") == 0) {
                trade->transType = MCB;
            }
            else if (strcmp(token, "MOS") == 0) {
                trade->transType = MOS;
            }
            else if (strcmp(token, "MCS") == 0) {
                trade->transType = MCS;
            }
            else if (strcmp(token, "FBO") == 0) {
                trade->transType = FBO;
            }
            else if (strcmp(token, "FBC") == 0) {
                trade->transType = FBC;
            }
            else if (strcmp(token, "FSO") == 0) {
                trade->transType = FSO;
            }
            else if (strcmp(token, "FSC") == 0) {
                trade->transType = FSC;
            }
        }
        else if (i == 5)
        {
            strcpy(trade->date, token);
        }
        else if (i == 7)
        {
            trade->qty = (uint64)atoi(token);
        }
        else if (i == 8)
        {
            trade->price = (real64)atof(token);
        }
        else if (i == 9)
        {
            trade->brokerage = (real64)atof(token);
        }
        else if (i == 10)
        {
            trade->serviceTax = (real64)atof(token);
        }
        else if (i == 13)
        {
            trade->currency = (strcmp(token, "CASH_INR") == 0) ?
                              INR : USD;
        }
        else if (i == 17)
        {
            strcpy(trade->expiry, token);
        }
        else if (i == 18)
        {
            trade->strike = (real64)atof(token);
        }
        else if (i == 19)
        {
            if (strcmp(token, "PE") == 0)
            {
                trade->optType = PE;
            }
            else if (strcmp(token, "CE") == 0)
            {
                trade->optType = CE;
            }
            else
            {
                trade->optType = NA;
            }
        }
        else if (i == 20)
        {
            if (strcmp(token, "OPTIDX") == 0)
            {
                trade->instType = OPTIDX;
            }
            else if (strcmp(token, "OPTSTK") == 0)
            {
                trade->instType = OPTSTK;
            }
            else if (strcmp(token, "FUTIDX") == 0)
            {
                trade->instType = FUTIDX;
            }
            else if (strcmp (token, "FUTSTK") == 0)
            {
                trade->instType = FUTSTK;
            }
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
        if (i ==  0)
        {
            strcpy(trade->brokerCode, token);
        }
        else if (i ==  1)
        {
            strcpy(trade->strategySymbol, token);
        }
        else if (i == 2)
        {
            strcpy(trade->symbol, token);
        }
        else if (i ==  4)
        {
            if (strcmp(token, "MBY") == 0) {
                trade->transType = MB;
            }
            else if (strcmp(token, "MSL") == 0) {
                trade->transType = MS;
            }
            else if (strcmp(token, "LB") == 0) {
                trade->transType = LB;
            }
            else if (strcmp(token, "LS") == 0) {
                trade->transType = LS;
            }
        }
        else if (i == 5)
        {
            strcpy(trade->date, token);
        }
        else if (i == 7)
        {
            trade->qty = (uint64)atoi(token);
        }
        else if (i == 8)
        {
            trade->price = (real64)atof(token);
        }
        else if (i == 9)
        {
            trade->brokerage = (real64)atof(token);
        }
        else if (i == 10)
        {
            trade->serviceTax = (real64)atof(token);
        }
        else if (i == 13)
        {
            trade->currency = (strcmp(token, "CASH_INR") == 0) ?
                              INR : USD;
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
printFundLedger(State *state)
{
    // print the ledger.
    for (int i = 0; i < state->strategies[state->currStratIndex].currEntryId + 1; i++)
    {
        LedgerEntry entry = state->strategies[state->currStratIndex].ledger[i];
        printf("name: %s, type: %d, debit: %f, credit: %f, curr: %d\n",
               entry.accountName,
               entry.type,
               entry.debit,
               entry.credit,
               entry.currency);
    }
}

void
processBhavEq(FILE *bhavFile, int stratIndex, State *state)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), bhavFile))
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
        /* NOTE(Akhil): The symbol may be present in multiple strats,
                        need to update the posns in all of them. */
        for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
        {
            if (strcmp(bhav.symbol, state->strategies[stratIndex].positions[i].symbol) == 0)
            {
                state->strategies[stratIndex].positions[i].ltp = bhav.ltp;
            }
            // printf("pos after bhav is %s, %d, %f\n",
            //        state->strategies[stratIndex].positions[1].symbol,
            //        state->strategies[stratIndex].positions[1].qty,
            //        state->strategies[stratIndex].positions[1].ltp);
            // printf("pos after bhav is %s, %d, %f\n",
            //        state->strategies[stratIndex].positions[1].symbol,
            //        state->strategies[stratIndex].positions[1].qty,
            //        state->strategies[stratIndex].positions[1].ltp);
        }
        // printf("cash after bhav is %f\n", state->strategies[stratIndex].cash);
    }
}

void
processBhav(FILE *bhavFile, int stratIndex, State *state)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), bhavFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        FNO_bhav bhav = {};
        LoadFNOBhav(&bhav, line);
        /* NOTE(Akhil): The symbol may be present in multiple strats,
                        need to update the posns in all of them. */
        for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
        {

            if (strcmp(bhav.symbol,
                       state->strategies[stratIndex].fpositions[i].symbol) == 0 &&
                strcmp(bhav.expiry,
                       state->strategies[stratIndex].fpositions[i].expiry) == 0 &&
                bhav.strike == state->strategies[stratIndex].fpositions[i].strike &&
                bhav.optType == state->strategies[stratIndex].fpositions[i].optType &&
                bhav.instType == state->strategies[stratIndex].fpositions[i].instType)
            {
                state->strategies[stratIndex].fpositions[i].ltp = bhav.ltp;
                printf("pos after bhav is %s, %d, %f\n",
                       state->strategies[stratIndex].fpositions[i].symbol,
                       state->strategies[stratIndex].fpositions[i].qty,
                       state->strategies[stratIndex].fpositions[i].ltp);
                printf("pos after bhav is %s, %d, %f\n",
                       state->strategies[stratIndex].fpositions[i].symbol,
                       state->strategies[stratIndex].fpositions[i].qty,
                       state->strategies[stratIndex].fpositions[i].ltp);
            }
        }
        // printf("cash after bhav is %f\n", state->strategies[stratIndex].cash);
    }
}

int
processTradesEq(FILE *tradeFile, State *state)
{
    char line[1024];
    int i = 0;
    int stratIndex = -1;
    while (fgets(line, sizeof(line), tradeFile))
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
        // find the strategy index first.
        for (int i = 0; i < state->currStratIndex + 1; i++)
        {
            if (strcmp(trade.strategySymbol, state->strategies[i].symbol) == 0)
            {
                stratIndex = i;
                break;
            }
        }

        if (stratIndex == -1)
        {
            printf("Couldn't find strategy, aborting!\n");
            return -2;
        }

        // apply trade to the positions state.
        char stratSymbol[100];
        strcpy(stratSymbol, state->strategies[stratIndex].symbol);
        int found = 0;
        for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
        {
            if (strcmp(state->strategies[stratIndex].positions[i].isin,
                       trade.symbol) == 0)
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


                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            if (state->strategies[stratIndex].positions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].positions[i].price = 0.0;
                            }
                            else
                            {
                                state->strategies[stratIndex].positions[i].price =
                                    ((state->strategies[stratIndex].positions[i].price *
                                    state->strategies[stratIndex].positions[i].qty)
                                    + (trade.qty * priceAfterFee)) 
                                    / (state->strategies[stratIndex].positions[i].qty +
                                    trade.qty);
                            }

                            state->strategies[stratIndex].positions[i].qty += trade.qty;

                            // add the entries to the ledger.
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_POSN");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                            currJournalId;
                            LedgerEntry liabEntry = {};
                            strcpy(liabEntry.accountName, stratSymbol);
                            strcpy(liabEntry.accountName, "_CASH_USD");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = REVENUE;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                           currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
                            break;
                        }

                    default:
                        {
                           printf("trade qty is %f\n", (real64)trade.qty);
                           trade.qty = -trade.qty;
                           real64 priceAfterFee =
                            (abs(trade.qty) * trade.price *
                            (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                            / abs(trade.qty); 

                            // you always get less after selling.
                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            if (state->strategies[stratIndex].positions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].positions[i].price = 0.0;
                            }
                            else
                            {
                                state->strategies[stratIndex].positions[i].price =
                                    ((state->strategies[stratIndex].positions[i].price *
                                    state->strategies[stratIndex].positions[i].qty) +
                                    (trade.qty * priceAfterFee)) 
                                    / (state->strategies[stratIndex].positions[i].qty +
                                    trade.qty);
                            }
                            state->strategies[stratIndex].positions[i].qty += trade.qty;
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_CASH_USD");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                            currJournalId;
                            LedgerEntry liabEntry = {};
                            strcpy(liabEntry.accountName, stratSymbol);
                            strcpy(liabEntry.accountName, "_POSN");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = EQUITY;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
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
            PositionEquity pos = {};
            strcpy(pos.isin, trade.symbol);
            /* NOTE(akhil): Assumes you have uploaded the securities in the
                            trade file */
            // populate the rest of the details from the securities data.
            for (int i = 0; i < state->currSecIndex + 1; i++)
            {
                if (strcmp(trade.symbol, state->secs[i].isin) == 0)
                {
                    strcpy(pos.symbol, state->secs[i].symbol);
                    strcpy(pos.id , state->secs[i].id);
                }
            }

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
                        state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        ++state->strategies[state->currStratIndex].currJournalId;
                        LedgerEntry assetEntry = {};
                        strcat(assetEntry.accountName, stratSymbol);
                        strcat(assetEntry.accountName, "_POSN");
                        assetEntry.type = ASSET;
                        assetEntry.currency = trade.currency;
                        assetEntry.debit = abs(trade.qty * priceAfterFee);
                        assetEntry.id = state->strategies[state->currStratIndex].
                            currJournalId;
                        LedgerEntry liabEntry = {};
                        strcat(liabEntry.accountName, stratSymbol);
                        strcat(liabEntry.accountName, "_CASH_USD");
                        liabEntry.credit = abs(trade.qty * priceAfterFee);
                        liabEntry.type = REVENUE;
                        liabEntry.id = state->strategies[state->currStratIndex].
                            currJournalId;
                        liabEntry.currency = trade.currency;
                        state->strategies[state->currStratIndex].
                            ledger[++state->strategies[state->currStratIndex].
                            currEntryId] = assetEntry;
                        state->strategies[state->currStratIndex].
                            ledger[++state->strategies[state->currStratIndex].
                            currEntryId] = liabEntry;
                        break;
                    }
                default:
                    {
                        trade.qty = -trade.qty;
                        real64 priceAfterFee =
                            (abs(trade.qty) * trade.price *
                            (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                            / abs(trade.qty);
                        // you always get less after selling.
                        state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        ++state->strategies[state->currStratIndex].currJournalId;
                        LedgerEntry assetEntry = {};
                        strcat(assetEntry.accountName, stratSymbol);
                        strcat(assetEntry.accountName, "_CASH_USD");
                        assetEntry.type = ASSET;
                        assetEntry.currency = trade.currency;
                        assetEntry.debit = abs(trade.qty * priceAfterFee);
                        assetEntry.id = state->strategies[state->currStratIndex].
                            currJournalId;
                        LedgerEntry liabEntry = {};
                        strcat(liabEntry.accountName, stratSymbol);
                        strcat(liabEntry.accountName, "_POSN");
                        liabEntry.credit = abs(trade.qty * priceAfterFee);
                        liabEntry.type = EQUITY;
                        liabEntry.id = state->strategies[state->currStratIndex].
                            currJournalId;
                        liabEntry.currency = trade.currency;
                        state->strategies[state->currStratIndex].
                            ledger[++state->strategies[state->currStratIndex].
                            currEntryId] = assetEntry;
                        state->strategies[state->currStratIndex].
                            ledger[++state->strategies[state->currStratIndex].
                            currEntryId] = liabEntry;
                        break;
                    }
            }
            state->strategies[stratIndex].positions[++state->strategies[stratIndex].currPosIndex] = pos;
        }
        printf("cash is %f\n", state->strategies[stratIndex].cash);
        printf("pos is %s, %d, %f\n",
               state->strategies[stratIndex].positions[stratIndex].symbol,
               state->strategies[stratIndex].positions[stratIndex].qty,
               state->strategies[stratIndex].positions[stratIndex].ltp);
        printf("pos is %s, %d, %f\n",
               state->strategies[stratIndex].positions[1].symbol,
               state->strategies[stratIndex].positions[1].qty,
               state->strategies[stratIndex].positions[1].ltp);

    }
    return stratIndex;
}

int
processTrades(FILE *tradeFile, State *state)
{
    char line[1024];
    int i = 0;
    int stratIndex = -1;
    while (fgets(line, sizeof(line), tradeFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        FNO_trade trade = {};
        LoadFNOTrade(&trade, line);
        // find the strategy index first.
        for (int i = 0; i < state->currStratIndex + 1; i++)
        {
            if (strcmp(trade.strategySymbol, state->strategies[i].symbol) == 0)
            {
                stratIndex = i;
                break;
            }
        }

        if (stratIndex == -1)
        {
            printf("Couldn't find strategy, aborting!\n");
            return -2;
        }

        // apply trade to the positions state.
        char stratSymbol[100];
        strcpy(stratSymbol, state->strategies[stratIndex].symbol);
        int found = 0;
        for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
        {
            if (strcmp(trade.symbol, "NATURALGAS") == 0)
            {
                printf("here\n");
            }
            if (strcmp(state->strategies[stratIndex].fpositions[i].symbol,
                       trade.symbol) == 0 &&
                strcmp(state->strategies[stratIndex].fpositions[i].expiry,
                       trade.expiry) == 0 &&
                state->strategies[stratIndex].fpositions[i].strike ==
                trade.strike &&
                state->strategies[stratIndex].fpositions[i].optType ==
                trade.optType &&
                state->strategies[stratIndex].fpositions[i].instType !=
                FUTSTK &&
                state->strategies[stratIndex].fpositions[i].instType ==
                trade.instType)
            {
                switch (trade.transType)
                {
                    case MOB:
                    case MCB:
                    case FBO:
                    case FBC:
                        {
                            // you pay more while buying.
                            real64 priceAfterFee =
                                (trade.qty * trade.price *
                                (1.0 + (trade.brokerage + trade.serviceTax) / 100.0))
                                / trade.qty; 


                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            if (state->strategies[stratIndex].fpositions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].fpositions[i].price = 0.0;
                            }
                            else
                            {
                                state->strategies[stratIndex].fpositions[i].price =
                                    ((state->strategies[stratIndex].fpositions[i].price *
                                    state->strategies[stratIndex].fpositions[i].qty)
                                    + (trade.qty * priceAfterFee)) 
                                    / (state->strategies[stratIndex].fpositions[i].qty +
                                    trade.qty);
                            }
                            state->strategies[stratIndex].fpositions[i].qty += trade.qty;

                            // add the entries to the ledger.
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_POSN");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            LedgerEntry liabEntry = {};
                            strcpy(liabEntry.accountName, stratSymbol);
                            strcpy(liabEntry.accountName, "_CASH_USD");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = REVENUE;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
                            break;
                        }

                    default:
                        {
                            printf("trade qty is %f\n", (real64)trade.qty);
                            trade.qty = -trade.qty;
                            real64 priceAfterFee =
                                (abs(trade.qty) * trade.price *
                                (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                                / abs(trade.qty); 

                            // you always get less after selling.
                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            if (state->strategies[stratIndex].fpositions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].fpositions[i].price = 0.0;
                            }
                            else
                            {
                                state->strategies[stratIndex].fpositions[i].price =
                                    ((state->strategies[stratIndex].fpositions[i].price *
                                    state->strategies[stratIndex].fpositions[i].qty) +
                                    (trade.qty * priceAfterFee)) 
                                    / (state->strategies[stratIndex].fpositions[i].qty + trade.qty);
                            }
                            state->strategies[stratIndex].fpositions[i].qty += trade.qty;
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_CASH_USD");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            LedgerEntry liabEntry = {};
                            strcpy(liabEntry.accountName, stratSymbol);
                            strcpy(liabEntry.accountName, "_POSN");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = EQUITY;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
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
            FNO_position pos = {};
            strcpy(pos.symbol, trade.symbol);
            pos.strike = trade.strike;
            strcpy(pos.expiry,trade.expiry);
            pos.optType = trade.optType;
            pos.instType = trade.instType;
            switch(trade.transType)
            {
                case MOB:
                case MCB:
                case FBO:
                case FBC:
                    {
                        real64 priceAfterFee =
                            (trade.qty * trade.price *
                            (1.0 + (trade.brokerage + trade.serviceTax) / 100.0))
                            / trade.qty;
                        // you always pay more while buying.
                        if (pos.instType != FUTSTK)
                        {
                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_POSN");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            LedgerEntry liabEntry = {};
                            strcat(liabEntry.accountName, stratSymbol);
                            strcat(liabEntry.accountName, "_CASH_USD");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = REVENUE;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
                        }
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        break;
                    }
                default:
                    {
                        trade.qty = -trade.qty;
                        real64 priceAfterFee =
                            (abs(trade.qty) * trade.price *
                            (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                            / abs(trade.qty);

                        // you always get less after selling.
                        if (pos.instType != FUTSTK)
                        {
                            state->strategies[stratIndex].cash -= trade.qty * priceAfterFee;
                            ++state->strategies[state->currStratIndex].currJournalId;
                            LedgerEntry assetEntry = {};
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_CASH_USD");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            LedgerEntry liabEntry = {};
                            strcat(liabEntry.accountName, stratSymbol);
                            strcat(liabEntry.accountName, "_POSN");
                            liabEntry.credit = abs(trade.qty * priceAfterFee);
                            liabEntry.type = EQUITY;
                            liabEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
                            liabEntry.currency = trade.currency;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = assetEntry;
                            state->strategies[state->currStratIndex].
                                ledger[++state->strategies[state->currStratIndex].
                                currEntryId] = liabEntry;
                        }
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        break;
                    }
            }
            state->strategies[stratIndex].fpositions[++state->strategies[stratIndex].currFPosIndex] = pos;
        }
        printf("cash is %f\n", state->strategies[stratIndex].cash);
        printf("pos is %s, %d, %f\n",
               state->strategies[stratIndex].positions[stratIndex].symbol,
               state->strategies[stratIndex].positions[stratIndex].qty,
               state->strategies[stratIndex].positions[stratIndex].ltp);
        printf("pos is %s, %d, %f\n",
               state->strategies[stratIndex].positions[1].symbol,
               state->strategies[stratIndex].positions[1].qty,
               state->strategies[stratIndex].positions[1].ltp);
    }

    return stratIndex;
}

void
processExRate(FILE *file, State *state, Exchange_rate *exRate)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), file))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LoadExchangeRate(exRate, line);
        state->exRates[i - 1] = *exRate; // it's a copy here.
        printf("ex rate is %f\n", state->exRates[i - 1].rate);
    }
}

void
printPositions(State *state, int stratIndex)
{
    for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
    {
        PositionEquity pos = state->strategies[stratIndex].positions[i];
        printf("name: %s, qty : %d, price: %f, ltp : %f, \
value : %f\n",
               pos.symbol,
               pos.qty,
               pos.price,
               pos.ltp,
               pos.qty * pos.ltp);
    }
}

void
printFPositions(State *state, int stratIndex)
{
    for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
    {
        FNO_position pos = state->strategies[stratIndex].fpositions[i];
        printf("name: %s, qty : %d, price: %f, ltp : %f, strike  : %f, expiry : %s \
value : %f\n",
               pos.symbol,
               pos.qty,
               pos.price,
               pos.ltp,
               pos.strike,
               pos.expiry,
               pos.qty * pos.ltp);
    }
}

/* collapse multiple independent lots of the same symbol into one.
       for futures only.
       Also move only the nonzero qty symbols to the newer array.*/
void
collapsePositions(State *state, int stratIndex)
{
    FNO_position newPositions[100];
    int currNewPosnIndex = -1;
    for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
    {
        FNO_position pos = state->strategies[stratIndex].fpositions[i];
        if (pos.qty != 0)
        {
            int found = 0;
            for (int j = 0; j < currNewPosnIndex + 1; j++)
            {
                if (strcmp(pos.symbol, newPositions[j].symbol) == 0)
                {
                    newPositions[j].qty += pos.qty;
                    newPositions[j].ltp = pos.ltp;
                    found = 1;
                    break;
                }
            }

            // not found, add to the new positions array.
            if (found != 1)
            {
                newPositions[++currNewPosnIndex] = pos;
            }
        }
    }
    // copy the new positions onto the state.
    state->strategies[stratIndex].currFPosIndex = -1;
    for (int i = 0; i < currNewPosnIndex + 1; i++)
    {
        FNO_position pos = newPositions[i];
        state->strategies[stratIndex].fpositions[i] = pos;
        ++state->strategies[stratIndex].currFPosIndex;
    }
}

void
makeVariationSettlements(State *state, int stratIndex)
{
    real64 totalVariation = 0.0;
    for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
    {
        FNO_position pos = state->strategies[stratIndex].fpositions[i];
        if (pos.instType == FUTSTK)
        {
            real64 variation = pos.qty * (pos.ltp - pos.price); 
            printf("variation of %f against %s\n", variation, pos.symbol);
            state->strategies[stratIndex].cash += variation;
            totalVariation += variation;
            // move the ltp now to the price column,
            // so that the next time variation is correct.
            pos.price = pos.ltp;
            state->strategies[stratIndex].fpositions[i] = pos;
            // make the ledger entries.
            char stratSymbol[100];
            strcpy(stratSymbol, state->strategies[stratIndex].symbol);
            LedgerEntry assetEntry = {};
            strcat(assetEntry.accountName, stratSymbol);
            strcat(assetEntry.accountName, "_CASH_USD");
            assetEntry.type = ASSET;
            assetEntry.currency = INR;
            assetEntry.debit = variation;
            assetEntry.id = state->strategies[state->currStratIndex].
                currJournalId;
            LedgerEntry liabEntry = {};
            strcat(liabEntry.accountName, stratSymbol);
            strcat(liabEntry.accountName, "_VARIATION");
            liabEntry.credit = variation;
            liabEntry.type = EQUITY;
            liabEntry.id = state->strategies[state->currStratIndex].
                currJournalId;
            liabEntry.currency = INR;
            state->strategies[state->currStratIndex].
                ledger[++state->strategies[state->currStratIndex].
                currEntryId] = assetEntry;
            state->strategies[state->currStratIndex].
                ledger[++state->strategies[state->currStratIndex].
                currEntryId] = liabEntry;
        }
    }
    printf("total variation is %f\n", totalVariation);
}

real64
getTotalPositionValue(State *state, int stratIndex)
{
    real64 totalValue = 0.0;
    for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
    {
        PositionEquity pos = state->strategies[stratIndex].positions[i];
        totalValue  += pos.qty * pos.ltp;
    }
    for (int i = 0; i < state->strategies[stratIndex].currFPosIndex + 1; i++)
    {
        FNO_position pos = state->strategies[stratIndex].fpositions[i];
        if (pos.instType != FUTSTK)
        {
            totalValue  += pos.qty * pos.ltp;
        }
    }
    return totalValue;
}

void
printNav(State *state, Exchange_rate *exRate, real64 totalUnits,
         real64 managementFees, int stratIndex)
{
    // total value of fno positions.
    real64 totalValue = getTotalPositionValue(state, stratIndex) + 48000 + 1014.16; 

    real64 cash = state->strategies[stratIndex].cash - 2934628;
    printf("closing inr cash balance is %f\n", cash);
    real64 cashUSD = (cash / exRate->rate) - managementFees;
    printf("closing cash balance in usd is %f\n", cashUSD);
    real64 totalValueUSD = totalValue / exRate->rate;
    printf("total position value in usd is %f\n", totalValueUSD);
    printf("total market value in usd is %f\n", (totalValueUSD + cashUSD));
    real64 nav = (totalValueUSD + cashUSD) / totalUnits;
    printf("nav is %f\n", nav);
}

void
uploadSecurities(FILE *secFile, State *state)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), secFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Security sec = {};
        LoadSecurity(&sec, state, line);
        state->secs[++state->currSecIndex] = sec;
        // printf("security is %s\n", state->secs[i - 1].name);
    }
}

void
LoadOldPosition(PositionEquity *pos, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(pos->isin , token);
        }
        else if (i == 1)
        {
            pos->price = (real64)atof(token); // NOTE(Akhil): needs to be different.
            pos->ltp = (real64)atof(token);
        }
        else if (i ==  2)
        {
            pos->qty = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadPriceUpdate(PriceUpdate *update, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(update->symbol, token);
        }
        else if (i == 1)
        {
            strcpy(update->date, token);
        }
        else if (i ==  2)
        {
            update->price = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

int
main()
{
    PGconn *conn = PQconnectdb("dbname=percival");
    PGresult* res = PQexec(conn, "SET DateStyle TO 'ISO, DMY';");
    char *error = PQresultErrorMessage(res);
    if (strcmp(error, "") != 0)
    {
        printf("%s", error);
    }
    PQclear(res);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "%s\n", PQerrorMessage(conn));
        return -1;
    }

    FILE *exchangeRateFile = fopen("exchange_rate.csv", "r");
    if (exchangeRateFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    State state = {};
    state.currStratIndex = -1;
    state.currSecIndex = -1;
    state.idCount = -1;
    char line[1024];
    int i = 0;
    Exchange_rate exRate = {};
    while (fgets(line, sizeof(line), exchangeRateFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LoadExchangeRate(&exRate, line);
        // Persist the exchange rate in db.
        char query[512];
        sprintf(query,
                "INSERT INTO exchange_rate "
                "(curr, rate, date, base) "
                "VALUES ('%s', %f, '%s', '%s');",
                exRate.curr == USD ? "USD" : "INR",
                exRate.rate,
                exRate.date,
                exRate.base == USD ? "USD" : "INR");

        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }

        state.exRates[i - 1] = exRate; // it's a copy here.
        printf("ex rate is %f\n", state.exRates[i - 1].rate);
        PQclear(pgResult);
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
    int stratId;
    i = 0;
    while (fgets(line, sizeof(line), stratFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        // NOTE(Akhil): here, the headinng is too big, lines split!
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        LoadStrategyFromFile(&strategy, line);
        /* NOTE(Akhil): for manual testing,
                        shouldn't this happen during cashflow? */
        // strategy.cash = 15314483.54; // inr
        strategy.cash = 14451145.95; // inr
        strategy.id = ++state.currStratIndex;
        state.strategies[state.currStratIndex].currEntryId = -1;
        state.strategies[state.currStratIndex] = strategy;
        printf("strategy id is %d\n", state.strategies[state.currStratIndex].id);
        printf("strategy name is %s\n", state.strategies[state.currStratIndex].symbol);
        char query[512];
        sprintf(query,
                "INSERT INTO strategy"
                "(symbol, cash) "
                "VALUES ('%s', %f);",
                strategy.symbol,
                strategy.cash);

        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }
        PQclear(pgResult);
        i++;
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
        char query[512];
        sprintf(query,
                "INSERT INTO investor"
                "(name) "
                "VALUES ('%s');",
                inv.name);

        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }
        PQclear(pgResult);
    }

    // step 2: the subscription file with accounting.
    FILE *subsFile = fopen("subscription_upa.csv", "r");
    if (subsFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    ++state.strategies[state.currStratIndex].currJournalId; // same id for the couple.
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
        entry.id = state.strategies[state.currStratIndex].currJournalId;
        if (i == 1) entry.type = EQUITY;
        else if (i == 2) entry.type = ASSET;
        char stratSymbol[100];
        AccountFromSubs(&entry, &state, inv, stratSymbol, line);
        printf("strat symbol is %s\n", stratSymbol);
        if (i == 2)
        {
            // Link the investor to the strategy in the db as well.
            char query[1024];
            sprintf(query,
                    "SELECT id FROM strategy where symbol = '%s' LIMIT 1",
                    stratSymbol);

            PGresult *pgResult = PQexec(conn, query);
            char *errorMessage = PQresultErrorMessage(pgResult);
            if (strcmp(errorMessage, "") != 0)
            {
                printf("%s", errorMessage);
            }

            if (PQntuples(pgResult) == 0)
            {
                fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
                PQclear(res);
                return -1;
            }

            char *id_str = PQgetvalue(pgResult, 0, 0);
            stratId = atoi(id_str);
            PQclear(pgResult);

            sprintf(query,
                    "UPDATE investor SET strategy_id = %d where name = '%s'",
                    stratId,
                    inv.name);
            pgResult = PQexec(conn, query);
            errorMessage = PQresultErrorMessage(pgResult);
            if (strcmp(errorMessage, "") != 0)
            {
                printf("%s", errorMessage);
            }
            PQclear(pgResult);

        }
        /* NOTE(Akhil): BUG here!!!, need to address the root split csv lines.*/
        char query[1024];
        snprintf(query, sizeof(query),
                 "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                 "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                 stratId,
                 LedgerEntryTypeStrings[entry.type], // Converts enum integer index to matching string literal
                 entry.accountName,
                 entry.debit,
                 entry.credit,
                 entry.memo,
                 entry.currency == USD ? "USD" : "INR" 
                 );
        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = entry;
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
    while (fgets(line, sizeof(line), bankFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        ++state.strategies[state.currStratIndex].currJournalId;

        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state.strategies[state.currStratIndex].currJournalId;
        liabEntry.id = state.strategies[state.currStratIndex].currJournalId;
        AccountFromBank(&assetEntry, &liabEntry, line);
        /* NOTE(Akhil): this is a shortcut, need all bank accounts,
                        not just final one.*/
        if (i == 3)
        {
            Bank_account acc = {};
            strcpy(acc.symbol, liabEntry.accountName);
            acc.balance = liabEntry.credit;
            acc.currency = liabEntry.currency;
            char query[1024];
            snprintf(query, sizeof(query),
                     "INSERT INTO bank_account (strategy_id, symbol, balance, currency) "
                     "VALUES (%d, '%s', %f, '%s');",
                     stratId,
                     acc.symbol,
                     acc.balance,
                     acc.currency == USD ? "USD" : "INR" 
                     );
            PGresult *pgResult = PQexec(conn, query);
            char *errorMessage = PQresultErrorMessage(pgResult);
            if (strcmp(errorMessage, "") != 0)
            {
                printf("%s", errorMessage);
            }
            PQclear(pgResult);
        }
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = assetEntry;
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }


    // step 4: reverse the upa debit account entry.
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
        ++state.strategies[state.currStratIndex].currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state.strategies[state.currStratIndex].currJournalId;
        AccountFromReverse(&liabEntry, line);
        char query[1024];
        snprintf(query, sizeof(query),
                 "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                 "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                 stratId,
                 LedgerEntryTypeStrings[liabEntry.type], // Converts enum integer index to matching string literal
                 liabEntry.accountName,
                 liabEntry.debit,
                 liabEntry.credit,
                 liabEntry.memo,
                 liabEntry.currency == USD ? "USD" : "INR" 
                 );
        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }


    // step 5: fund cashflow file.
    FILE *cashflowFile = fopen("fund_cashflow.csv", "r");
    if (cashflowFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    i = 0;
    while (fgets(line, sizeof(line), cashflowFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        /* NOTE(Akhil): here we are working on the latest strategy.
                        usually first column discloses the strategy name. */
        ++state.strategies[state.currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        assetEntry.id = state.strategies[state.currStratIndex].currJournalId;
        AccountFromCashFlow(&assetEntry, line);
        char query[1024];
        snprintf(query, sizeof(query),
                 "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                 "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                 stratId,
                 LedgerEntryTypeStrings[assetEntry.type], // Converts enum integer index to matching string literal
                 assetEntry.accountName,
                 assetEntry.debit,
                 assetEntry.credit,
                 assetEntry.memo,
                 assetEntry.currency == USD ? "USD" : "INR" 
                 );
        PGresult *pgResult = PQexec(conn, query);
        char *errorMessage = PQresultErrorMessage(pgResult);
        if (strcmp(errorMessage, "") != 0)
        {
            printf("%s", errorMessage);
        }
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = assetEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }


    // step 6 : unit allotment.
    FILE *unitFile = fopen("unit_allotment.csv", "r");
    if (unitFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    while (fgets(line, sizeof(line), unitFile))
    {
        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        allotUnits(&state, conn, line);
        i++;
    }

    PQfinish(conn);
    // // step 7: fund expense investor file.
    // FILE *expenseFile = fopen("fund_expense.csv", "r");
    // if (expenseFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    // i = 0;
    // while (fgets(line, sizeof(line), expenseFile))
    // {
    //     if (i == 0)
    //     {
    //         i++;
    //         continue; // ignore the top heading row.
    //     }
    //     char *tmp = strchr(line, '\n');
    //     if (tmp) *tmp = '\0';
    //     ++state.strategies[state.currStratIndex].currJournalId;
    //     LedgerEntry assetEntry = {};
    //     LedgerEntry liabEntry = {};
    //     assetEntry.id = state.strategies[state.currStratIndex].currJournalId;
    //     liabEntry.id = state.strategies[state.currStratIndex].currJournalId;
    //     AccountFromExpense(&assetEntry, &liabEntry, line);
    //     state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = assetEntry;
    //     state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = liabEntry;
    //     printf("entry name is %s and value is %f\n", assetEntry.accountName,
    //            assetEntry.debit);
    //     i++;
    // }
    //
    // /* store the previous day's i.e 10th june's open positions first */
    // int stratIndexx = -1;
    // for (int i = 0; i < state.currStratIndex + 1; i++)
    // {
    //     if (strcmp("SSFSAMST", state.strategies[i].symbol) == 0)
    //     {
    //         stratIndexx = i;
    //         break;
    //     }
    // }
    // printf("strat index is %d\n", stratIndexx);
    //
    // FNO_position posA = {};
    // strcpy(posA.symbol, "FINNIFTY");
    // strcpy(posA.expiry, "30/06/2026");
    // posA.strike = 25000;
    // posA.optType = CE;
    // posA.instType = OPTIDX;
    // posA.qty = 120;
    // state.strategies[stratIndexx].fpositions
    //     [++state.strategies[stratIndexx].currFPosIndex] = posA;
    //
    // FNO_position posB = {};
    // strcpy(posB.symbol, "NIFTY");
    // strcpy(posB.expiry, "30/06/2026");
    // posB.strike = 23200;
    // posB.optType = PE;
    // posB.instType = OPTIDX;
    // posB.qty = 1040;
    // state.strategies[stratIndexx].fpositions
    //     [++state.strategies[stratIndexx].currFPosIndex] = posB;
    //
    // FNO_position posC = {};
    // strcpy(posC.symbol, "NATURALGAS");
    // strcpy(posC.expiry, "25/06/2026");
    // posC.strike = 0;
    // posC.instType = FUTSTK;
    // posC.optType = NA;
    // posC.price = 305.7;
    // posC.qty = 1250;
    // state.strategies[stratIndexx].fpositions
    //     [++state.strategies[stratIndexx].currFPosIndex] = posC;
    //
    //
    // /* read the fno trades and make the positions */
    // FILE *FTradesFile = fopen("trades_fno.csv", "r");
    // if (FTradesFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    // int stratIndex = processTrades(FTradesFile, &state);
    //
    // //upload the bhavcopy for FNO.
    // FILE *FBhavFile = fopen("bhavcopy_fno.csv", "r");
    // if (FBhavFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    // processBhav(FBhavFile, stratIndex, &state);
    //
    // /* collapse all the open futures positions into the same position
    //    row by marking all the other independen't qtys as zero. */
    // /* read the trades pertaining to a particular strategy
    //        and apply them to the position state. */
    //
    // // get total value of the positions held for the strategy.
    // // real64 totalValue = 0.0;
    // // for (int i = 0; i < state.strategies[stratIndex].currPosIndex + 1; i++)
    // // {
    // //     PositionEquity pos = state.strategies[stratIndex].positions[i];
    // //     totalValue  += pos.qty * pos.ltp;
    // // }
    //
    // /* NOTE(Akhil): Update the bhav's of unknown symbols manually here
    //     usually the guy has a special file 21 price_update_us where
    //     he gives the ltp against the system generated symbol
    //     Also remember the uidff format of bse, that we need to be able
    //     to parse for fno */
    // for (int i = 0; i < state.strategies[stratIndex].currFPosIndex + 1; i++)
    // {
    //
    //     if (strcmp(state.strategies[stratIndex].fpositions[i].symbol,
    //                "NATURALGAS") == 0)
    //     {
    //         state.strategies[stratIndex].fpositions[i].ltp = 294.40; // natural gas.
    //     }
    //     else if (strcmp(state.strategies[stratIndex].fpositions[i].symbol,
    //                "CRUDEOIL") == 0)
    //     {
    //         state.strategies[stratIndex].fpositions[i].ltp = 8344.00; // natural gas.
    //     }
    // }
    // state.strategies[stratIndex].fpositions[6].ltp = 700.45; // sensex.
    //
    // /* run the mtm process, i.e process variation settlements for
    //    open futures positions: net_qty * (ltp - prev_price) */
    // makeVariationSettlements(&state, stratIndex);
    //
    // // get the total units from all the investors for a strategy.
    // // real64 totalUnits = 1007.729 + 175.444;
    // real64 totalUnits = 1183.249;
    // // for (int i = 0; i < state.strategies[stratIndex].currInvestorIndex + 1; i++)
    // // {
    // //     Investor inv = state.strategies[stratIndex].investors[i];
    // //     totalUnits += inv.units;
    // // }
    //
    // // calculate the nav = (totalValue + cash) / totalUnits.
    // state.strategies[stratIndex].cash += 2588560.68;
    // real64 managementFees = 301.54;
    // printNav(&state, &exRate, totalUnits, managementFees, stratIndex);
    //
    // /* 2ND DAY------------------------------------------ */
    //
    // collapsePositions(&state, stratIndex);
    //
    // printFPositions(&state, stratIndex);
    //
    // FILE *EFile = fopen("exchange_rate_12.csv", "r");
    // if (EFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // // update the ex rate for the second day.
    // processExRate(EFile, &state, &exRate);
    //
    // FILE *FTradessFile = fopen("trades_fno_12.csv", "r");
    // if (FTradessFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // // process trades for 12th june.
    // stratIndex = processTrades(FTradessFile, &state);
    // FILE *FBhavvFile = fopen("bhavcopy_fno_12.csv", "r");
    // if (FBhavvFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // // process bhavcopy of 12th june.
    // processBhav(FBhavvFile, stratIndex, &state);
    //
    // for (int i = 0; i < state.strategies[stratIndex].currFPosIndex + 1; i++)
    // {
    //
    //     if (strcmp(state.strategies[stratIndex].fpositions[i].symbol,
    //                "NATURALGAS") == 0)
    //     {
    //         state.strategies[stratIndex].fpositions[i].ltp = 296.70; // natural gas.
    //     }
    //     else if (strcmp(state.strategies[stratIndex].fpositions[i].symbol,
    //                     "CRUDEOIL") == 0)
    //     {
    //         state.strategies[stratIndex].fpositions[i].ltp = 8073.00; // natural gas.
    //     }
    // }
    // state.strategies[stratIndex].fpositions[2].ltp = 226.2; // sensex 73500 pe.
    // state.strategies[stratIndex].fpositions[6].ltp = 1223.55; // sensex 75000 ce.
    //
    // makeVariationSettlements(&state, stratIndex);
    // printFundLedger(&state);
    // managementFees = 306.63;
    // printNav(&state, &exRate, totalUnits, managementFees, stratIndex);
    //
    //
    //
    // /*----------------- New strategy, equity series -----------------*/
    // FILE *strattFile = fopen("strategy_master_eq.csv", "r");
    // if (strattFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // Strategy strat = {};
    // i = 0;
    // while (fgets(line, sizeof(line), strattFile))
    // {
    //     if (i == 0)
    //     {
    //         i++;
    //         continue; // ignore the top heading row.
    //     }
    //     // NOTE(Akhil): here, the headinng is too big, lines split!
    //     char *tmp = strchr(line, '\n');
    //     if (tmp) *tmp = '\0';
    //     LoadStrategyFromFile(&strat, line);
    //     /* NOTE(Akhil): for manual testing,
    //                     shouldn't this happen during cashflow? */
    //     // strategy.cash = 15314483.54; // inr
    //     strat.cash = 53764.80; // inr
    //     strat.id = ++state.currStratIndex;
    //     state.strategies[state.currStratIndex].currEntryId = -1;
    //     state.strategies[state.currStratIndex] = strat;
    //     printf("strategy id is %d\n", state.strategies[state.currStratIndex].id);
    //     printf("strategy name is %s\n", state.strategies[state.currStratIndex].symbol);
    //     i++;
    // }
    //
    // FILE *securityFile = fopen("securities.csv", "r");
    // if (securityFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // uploadSecurities(securityFile, &state);
    // for (int i = 0; i < state.currStratIndex + 1; i++)
    // {
    //     if (strcmp("SSFSAMSTE", state.strategies[i].symbol) == 0)
    //     {
    //         stratIndex = i;
    //         break;
    //     }
    // }
    //
    // if (stratIndex == -1)
    // {
    //     printf("Couldn't find strategy, aborting!\n");
    //     return -2;
    // }
    //
    // state.strategies[stratIndex].cash = 53764.80;
    //
    // // load the previous day's open positions.
    // FILE *posFile = fopen("securities_price_qty.csv", "r");
    // if (posFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // i = 0;
    // while (fgets(line, sizeof(line), posFile))
    // {
    //     if (i == 0)
    //     {
    //         i++;
    //         continue; // ignore the top heading row.
    //     }
    //     char *tmp = strchr(line, '\n');
    //     if (tmp) *tmp = '\0';
    //     PositionEquity pos = {};
    //     LoadOldPosition(&pos, line);
    //     // populate the rest of the details from the securities data.
    //     for (int i = 0; i < state.currSecIndex + 1; i++)
    //     {
    //         if (strcmp(pos.isin, state.secs[i].isin) == 0)
    //         {
    //             strcpy(pos.symbol,
    //                    state.secs[i].symbol);
    //             strcpy(pos.id,
    //                    state.secs[i].id);
    //         }
    //     }
    //     state.strategies[stratIndex].positions
    //         [++state.strategies[stratIndex].currPosIndex] = pos;
    // }
    //
    // state.strategies[stratIndex].currFPosIndex = -1;
    // FNO_position posX = {};
    // strcpy(posX.symbol, "MOTHERSON");
    // strcpy(posX.expiry, "30/06/2026");
    // posX.instType = FUTSTK;
    // posX.optType = NA;
    // posX.strike = 0;
    // posX.qty = 6150;
    // posX.price = 144.67;
    // state.strategies[stratIndex].fpositions
    //     [++state.strategies[stratIndex].currFPosIndex] = posX;
    //
    // FNO_position posY = {};
    // strcpy(posY.symbol, "CGPOWER");
    // strcpy(posY.expiry, "30/06/2026");
    // posY.instType = FUTSTK;
    // posY.optType = NA;
    // posY.strike = 0;
    // posY.qty = 850;
    // posY.price = 958.15;
    // state.strategies[stratIndex].fpositions
    //     [++state.strategies[stratIndex].currFPosIndex] = posY;
    //
    // printf("here %d\n", stratIndex);
    // printPositions(&state, stratIndex);
    // printFPositions(&state, stratIndex);
    //
    // FILE *TradesFile = fopen("trades_eq.csv", "r");
    // if (TradesFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // processTradesEq(TradesFile, &state);
    //
    // FILE *BhavFile = fopen("bhavcopy_eq.csv", "r");
    // if (BhavFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // processBhavEq(BhavFile, stratIndex, &state);
    // FILE *BhavFFile = fopen("bhavcopy_eq_fno.csv", "r");
    // if (BhavFFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    // processBhav(BhavFFile, stratIndex, &state);
    //
    // FILE *priceFile = fopen("price_update.csv", "r");
    // if (priceFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // i = 0;
    // while (fgets(line, sizeof(line), priceFile))
    // {
    //     if (i == 0)
    //     {
    //         i++;
    //         continue; // ignore the top heading row.
    //     }
    //     char *tmp = strchr(line, '\n');
    //     if (tmp) *tmp = '\0';
    //     PriceUpdate update = {};
    //     LoadPriceUpdate(&update, line);
    //     for (int j = 0; j < state.strategies[stratIndex].currPosIndex + 1;
    //         j++)
    //     {
    //         if (strcmp(state.strategies[stratIndex].positions[j].isin,
    //                    update.symbol) == 0)
    //         {
    //             state.strategies[stratIndex].positions[j].ltp = update.price;
    //         }
    //     }
    // }
    //
    // printPositions(&state, stratIndex);
    // printFPositions(&state, stratIndex);
    // makeVariationSettlements(&state, stratIndex);
    // printFundLedger(&state);
    // managementFees = 0;
    // totalUnits = 927.387505;
    // printNav(&state, &exRate, totalUnits, managementFees, stratIndex);
}
