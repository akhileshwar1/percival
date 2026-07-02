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

int globalCounter = -1;
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
    char isin[100];
    char exDate[100];
    char type[100];
    real64 div;
} Dividend;

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
    char sys_id[100];
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
    real64 feesAccrued;
    Investor investors[MAX_INVESTORS];
    PositionEquity positions[MAX_POSITIONS];
    FNO_position fpositions[MAX_POSITIONS];
    Bank_account accs[10];
    int id;
    int currPosIndex;
    int currFPosIndex;
    int currInvestorIndex;
    LedgerEntry ledger[100];
    int currEntryId;
    int currJournalId;
    int currAccIndex;
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

const char* OptTypeStrings[] = {
    "PE",
    "CE",
    "NA"
};

const char* InstrumentTypeStrings[] = {
    "OPTIDX",
    "OPTSTK",
    "FUTIDX",
    "FUTSTK"
};

const char* TransTypeStrings[] = {
    "MB", "MS", "LB", "LS", "MOB", "MCB", "MOS", "MCS", "FSO", "FSC", "FBO", "FBC"
};

void ConvertDbDateToCFormat(const char *db_date, char *output, size_t output_size)
{
    int year = 0, month = 0, day = 0;

    // 1. Extract year, month, and day from 'YYYY-MM-DD'
    if (sscanf(db_date, "%d-%d-%d", &year, &month, &day) == 3)
    {
        // 2. Reformat and safely write to the output buffer as 'DD/MM/YYYY'
        snprintf(output, output_size, "%02d/%02d/%04d", day, month, year);
    }
    else
    {
        // Fallback: Clear string or copy original if parsing fails
        output[0] = '\0'; 
    }
}

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
    strat->currAccIndex = -1;
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

PGresult *
executeQuery(PGconn *conn, char *query)
{
    PGresult *pgResult = PQexec(conn, query);
    char *errorMessage = PQresultErrorMessage(pgResult);
    if (strcmp(errorMessage, "") != 0)
    {
        printf("%s", errorMessage);
    }
    return pgResult;
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
            PGresult *pgResult = executeQuery(conn, query);
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
                char *line, Currency_code startCurr)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    real64 prevValue;
    Currency_code prevDenom = startCurr;
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
            liabEntry->type = LIABILITY;
            printf("account name is %s\n", accountName);
            strcpy(liabEntry->accountName, accountName);
            strcpy(accountName, ""); //reset.
        }
        else if (i ==  5)
        {
            liabEntry->credit = (real64)atof(token);
            liabEntry->currency = prevDenom;
            prevValue = liabEntry->credit;
        }
        else if (i ==  6)
        {
            assetEntry->debit = (real64)atof(token);
            if (prevValue != assetEntry->debit)
            {
                assetEntry->currency = (startCurr == USD) ? INR : USD;
                prevDenom = assetEntry->currency;
            }
            else
            {
                assetEntry->currency = (startCurr == USD) ? USD : INR;
                prevDenom = assetEntry->currency;
            }
        }
        else if (i == 7)
        {
            strcat(accountName, token); 
        }
        else if (i ==  8)
        {
            strcat(accountName, "_"); 
            strcat(accountName, token); 
            printf("account name is %s\n", accountName);
            strcpy(assetEntry->accountName, accountName);
            assetEntry->type = ASSET;
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
processBhav(FILE *bhavFile, PGconn *conn, int dbStratId,
            int stratIndex, State *state)
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
                // persist the matched bhav and update the position.
                char query[512];
                snprintf(query, sizeof(query),
                         "INSERT INTO fno_bhav (strategy_id, symbol, ltp, expiry, strike, opt_type, inst_type) "
                         "VALUES (%d, '%s', %f, to_date('%s', 'DD/MM/YYYY'), %f, '%s', '%s');",
                         dbStratId,
                         bhav.symbol,
                         bhav.ltp,
                         bhav.expiry,
                         bhav.strike,
                         OptTypeStrings[bhav.optType],
                         InstrumentTypeStrings[bhav.instType]
                         );
                PGresult *pgResult = executeQuery(conn, query);
                PQclear(pgResult);

                snprintf(query, sizeof(query),
                         "UPDATE fno_position SET ltp = %f WHERE sys_id = '%s'",
                         bhav.ltp,
                         state->strategies[stratIndex].fpositions[i].sys_id
                         );
                pgResult = executeQuery(conn, query);
                PQclear(pgResult);
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
processTrades(FILE *tradeFile, PGconn *conn, int dbStratId, State *state)
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
        // persist.
        char query[1024];
        snprintf(query, sizeof(query),
                 "INSERT INTO fno_trade (strategy_id, symbol, broker_code, trade_date, strategy_symbol, "
                 "expiry, opt_type, inst_type, qty, price, brokerage, service_tax, strike, trans_type, currency) "
                 "VALUES (%d, '%s', '%s', to_date('%s', 'DD/MM/YYYY'), '%s', to_date('%s', 'DD/MM/YYYY'), "
                 "'%s', '%s', %d, %f, %f, %f, %f, '%s', '%s');",
                 dbStratId, // Found dynamically by strategySymbol lookup
                 trade.symbol,
                 trade.brokerCode,
                 trade.date,
                 trade.strategySymbol,
                 trade.expiry,
                 OptTypeStrings[trade.optType],
                 InstrumentTypeStrings[trade.instType],
                 trade.qty,
                 trade.price,
                 trade.brokerage,
                 trade.serviceTax,
                 trade.strike,
                 TransTypeStrings[trade.transType],
                 trade.currency == USD ? "USD" : "INR" 
                 );
        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);

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
                LedgerEntry assetEntry = {};
                LedgerEntry liabEntry = {};
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

                            /* NOTE(Akhil): 3 because sbirc for options/fut */
                            state->strategies[stratIndex].accs[3].balance -=
                                trade.qty * priceAfterFee;
                            // persist the accs balance.
                            snprintf(query, sizeof(query),
                                     "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                                     state->strategies[stratIndex].accs[3].balance,
                                     state->strategies[stratIndex].accs[3].symbol
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
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
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_POSN");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
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
                            state->strategies[stratIndex].accs[3].balance -=
                                trade.qty * priceAfterFee;
                            // persist the accs balance.
                            snprintf(query, sizeof(query),
                                     "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                                     state->strategies[stratIndex].accs[3].balance,
                                     state->strategies[stratIndex].accs[3].symbol
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
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
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_CASH_USD");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
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
                // persist the updates to price and qty.
                snprintf(query, sizeof(query),
                         "UPDATE fno_position SET price = %f, qty = %d "
                         " WHERE sys_id = '%s'",
                         state->strategies[stratIndex].fpositions[i].price,
                         state->strategies[stratIndex].fpositions[i].qty,
                         state->strategies[stratIndex].fpositions[i].sys_id
                         );
                pgResult = executeQuery(conn, query);
                PQclear(pgResult);
                char query[1024];
                snprintf(query, sizeof(query),
                         "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                         "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                         dbStratId,
                         LedgerEntryTypeStrings[assetEntry.type], // Converts enum integer index to matching string literal
                         assetEntry.accountName,
                         assetEntry.debit,
                         assetEntry.credit,
                         assetEntry.memo,
                         assetEntry.currency == USD ? "USD" : "INR" 
                         );
                PGresult *pgResult = executeQuery(conn, query);
                PQclear(pgResult);

                snprintf(query, sizeof(query),
                         "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                         "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                         dbStratId,
                         LedgerEntryTypeStrings[liabEntry.type], // Converts enum integer index to matching string literal
                         liabEntry.accountName,
                         liabEntry.debit,
                         liabEntry.credit,
                         liabEntry.memo,
                         liabEntry.currency == USD ? "USD" : "INR" 
                         );
                pgResult = executeQuery(conn, query);
                PQclear(pgResult);
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
            sprintf(pos.sys_id, "OPT%d", ++globalCounter);
            LedgerEntry assetEntry = {};
            LedgerEntry liabEntry = {};
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
                            state->strategies[stratIndex].accs[3].balance -=
                                trade.qty * priceAfterFee;
                            // persist the accs balance.
                            snprintf(query, sizeof(query),
                                     "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                                     state->strategies[stratIndex].accs[3].balance,
                                     state->strategies[stratIndex].accs[3].symbol
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            ++state->strategies[state->currStratIndex].currJournalId;
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_POSN");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
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
                            state->strategies[stratIndex].accs[3].balance -=
                                trade.qty * priceAfterFee;
                            // persist the accs balance.
                            snprintf(query, sizeof(query),
                                     "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                                     state->strategies[stratIndex].accs[3].balance,
                                     state->strategies[stratIndex].accs[3].symbol
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(conn, query);
                            PQclear(pgResult);
                            ++state->strategies[state->currStratIndex].currJournalId;
                            strcat(assetEntry.accountName, stratSymbol);
                            strcat(assetEntry.accountName, "_CASH_USD");
                            assetEntry.type = ASSET;
                            assetEntry.currency = trade.currency;
                            assetEntry.debit = abs(trade.qty * priceAfterFee);
                            assetEntry.id = state->strategies[state->currStratIndex].
                                currJournalId;
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
            // persist the updates to price and qty.
            snprintf(query, sizeof(query),
                     "INSERT INTO fno_position (sys_id, strategy_id, symbol, qty, price, ltp, expiry, strike, opt_type, inst_type) "
                     "VALUES ('%s', %d, '%s', %d, %f, %f, to_date('%s', 'DD/MM/YYYY'), %f, '%s', '%s');",
                     pos.sys_id,
                     dbStratId,
                     pos.symbol,
                     pos.qty,
                     pos.price,
                     pos.ltp,
                     pos.expiry,
                     pos.strike,
                     OptTypeStrings[pos.optType],
                     InstrumentTypeStrings[pos.instType]
                     ); 
            pgResult = executeQuery(conn, query);
            PQclear(pgResult);
            char query[1024];
            snprintf(query, sizeof(query),
                     "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                     "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                     dbStratId,
                     LedgerEntryTypeStrings[assetEntry.type], // Converts enum integer index to matching string literal
                     assetEntry.accountName,
                     assetEntry.debit,
                     assetEntry.credit,
                     assetEntry.memo,
                     assetEntry.currency == USD ? "USD" : "INR" 
                     );
            PGresult *pgResult = executeQuery(conn, query);
            PQclear(pgResult);

            snprintf(query, sizeof(query),
                     "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                     "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                     dbStratId,
                     LedgerEntryTypeStrings[liabEntry.type], // Converts enum integer index to matching string literal
                     liabEntry.accountName,
                     liabEntry.debit,
                     liabEntry.credit,
                     liabEntry.memo,
                     liabEntry.currency == USD ? "USD" : "INR" 
                     );
            pgResult = executeQuery(conn, query);
            PQclear(pgResult);
        }
        printf("cash is %f\n", state->strategies[stratIndex].accs[3].balance);
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
/* NOTE(Akhil): Do I need to persist this combined position??
                but then we will have to zero out the qtys of the 
                independent ones
                Do it at the beginning of each day??*/
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
makeVariationSettlements(State *state, PGconn *conn, int dbStratId, int stratIndex)
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
            char query[1024];
            sprintf(query,
                    "UPDATE strategy SET cash = %f where id = %d",
                    state->strategies[stratIndex].cash,
                    dbStratId);
            PGresult *pgResult = executeQuery(conn, query);
            PQclear(pgResult);
            totalVariation += variation;
            // move the ltp now to the price column,
            // so that the next time variation is correct.
            pos.price = pos.ltp;
            sprintf(query,
                    "UPDATE fno_position SET price = %f where sys_id = '%s'",
                    pos.ltp,
                    pos.sys_id);
            pgResult = executeQuery(conn, query);
            PQclear(pgResult);
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
            snprintf(query, sizeof(query),
                     "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                     "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                     dbStratId,
                     LedgerEntryTypeStrings[assetEntry.type], // Converts enum integer index to matching string literal
                     assetEntry.accountName,
                     assetEntry.debit,
                     assetEntry.credit,
                     assetEntry.memo,
                     assetEntry.currency == USD ? "USD" : "INR" 
                     );
            pgResult = executeQuery(conn, query);
            PQclear(pgResult);

            snprintf(query, sizeof(query),
                     "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
                     "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
                     dbStratId,
                     LedgerEntryTypeStrings[liabEntry.type], // Converts enum integer index to matching string literal
                     liabEntry.accountName,
                     liabEntry.debit,
                     liabEntry.credit,
                     liabEntry.memo,
                     liabEntry.currency == USD ? "USD" : "INR" 
                     );
            pgResult = executeQuery(conn, query);
            PQclear(pgResult);
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

real64
getTotalCashUSD(State *state, int stratIndex, Exchange_rate *exRate)
{
    real64 totalCashINR = 0.0;
    real64 totalCashUSD = 0.0;
    // NOTE(Akhil) : starting i from 1 because we skip sbm account.
    for (int i = 1; i <= state->strategies[stratIndex].currAccIndex;
         i++)
    {
        if (state->strategies[stratIndex].accs[i].currency == INR)
        {
            totalCashINR += state->strategies[stratIndex].accs[i].balance;
        }
        else
        {
            totalCashUSD += state->strategies[stratIndex].accs[i].balance;
        }
    }
    totalCashUSD += (totalCashINR / exRate->rate);
    return totalCashUSD;
}

void
printNav(State *state, Exchange_rate *exRate, real64 totalUnits,
         int stratIndex, PGconn *conn, int dbStratId)
{
    // total value of fno positions.
    real64 totalValue = getTotalPositionValue(state, stratIndex); 

    real64 cashUSD = getTotalCashUSD(state, stratIndex, exRate);
    printf("closing cash balance in usd is %f\n", cashUSD);
    real64 totalValueUSD = totalValue / exRate->rate;
    printf("total position value in usd is %f\n", totalValueUSD);
    printf("total market value in usd is %f\n", (totalValueUSD + cashUSD));
    real64 netAssets = totalValueUSD + cashUSD;
    real64 fee = netAssets * (0.01 / 365); // 1% p.a
    state->strategies[stratIndex].feesAccrued += fee;
    real64 feesAccrued = state->strategies[stratIndex].feesAccrued; 
    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE strategy SET fees_accrued = %f WHERE id = %d",
             feesAccrued, 
             dbStratId
             );
    PGresult *pgResult = executeQuery(conn, query);
    PQclear(pgResult);
    printf("fee accrued %f, %f\n", fee, feesAccrued);
    real64 nav = (netAssets - feesAccrued) / totalUnits;
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

void
loadStateFromDB(State *state, PGconn *conn)
{
    char query[1024];
    state->currStratIndex = -1;
    sprintf(query,
            "SELECT * FROM strategy");

    PGresult *pgResult = executeQuery(conn, query);
    int rows = PQntuples(pgResult);
    int cols = PQnfields(pgResult);
    if (rows == 0)
    {
        fprintf(stderr, "No strategy found matching symbol: \n");
        PQclear(pgResult);
        PQfinish(conn);
        return;
    }

    // Load the strategies for each symbol.
    for (int i = 0; i < rows; i++)
    {
        Strategy strat = {};
        strat.currInvestorIndex = -1;
        strat.currFPosIndex = -1;
        strat.currAccIndex = -1;
        for (int j = 0; j < cols; j++)
        {
            char *str = PQgetvalue(pgResult, i, j);
            if (j == 0)
            {
                strat.id = atoi(str);
            }
            else if (j == 2)
            {
                strcpy(strat.symbol, str);
            }
            else if (j == 3)
            {
                strat.cash = atof(str);
            }
            else if (j == 4)
            {
                strat.feesAccrued = atof(str);
            }
            else if (j == 5)
            {
                strat.nav = atof(str);
            }
            else if (j == 6)
            {
                // go for the investors, accs, and positions now.
                sprintf(query,
                        "SELECT * FROM investor WHERE strategy_id = %d",
                        strat.id);

                PGresult *pgResultInv = executeQuery(conn, query);
                int ir = PQntuples(pgResultInv);
                int ic = PQnfields(pgResultInv);
                if (ir == 0)
                {
                    fprintf(stderr, "No investor found matching symbol: \n");
                    PQclear(pgResultInv);
                }
                else
                {
                    for (int a = 0; a < ir; a++)
                    {
                        Investor inv = {};
                        for (int b = 0; b < ic; b++)
                        {
                            char *str = PQgetvalue(pgResultInv, a, b);
                            if (b == 3)
                            {
                                strcpy(inv.name, str);
                            }
                            else if (b == 4)
                            {
                                inv.units = atof(str);
                            }
                        }
                        strat.investors[++strat.currInvestorIndex] = inv; 
                    }
                    PQclear(pgResultInv);
                }

                // go for bank accounts now.
                sprintf(query,
                        "SELECT * FROM bank_account WHERE strategy_id = %d",
                        strat.id);
                PGresult *pgResultAcc = executeQuery(conn, query);
                ir = PQntuples(pgResultAcc);
                ic = PQnfields(pgResultAcc);
                if (ir == 0)
                {
                    fprintf(stderr, "No fno_position found matching symbol: \n");
                    PQclear(pgResultAcc);
                }
                else
                {
                    for (int i = 0; i < ir; i++)
                    {
                        Bank_account acc = {};
                        for (int j = 0; j < ic; j++)
                        {
                            char *str = PQgetvalue(pgResultAcc, i, j);
                            if (j == 1)
                            {
                                strcpy(acc.symbol, str);
                            }
                            else if (j == 2)
                            {
                                acc.balance = atof(str);
                            }
                            else if (j == 3)
                            {
                                acc.currency = strcmp(str, "USD") == 0 ? USD : INR;
                            }
                        }
                        strat.accs[++strat.currAccIndex] = acc; 
                    }
                    PQclear(pgResultAcc);
                }


                // now, add the fno_positions to the strat.
                sprintf(query,
                        "SELECT * FROM fno_position WHERE strategy_id = %d",
                        strat.id);

                PGresult *pgResultPos = executeQuery(conn, query);
                ir = PQntuples(pgResultPos);
                ic = PQnfields(pgResultPos);
                if (ir == 0)
                {
                    fprintf(stderr, "No fno_position found matching symbol: \n");
                    PQclear(pgResultPos);
                }
                else
                {
                    for (int a = 0; a < ir; a++)
                    {
                        FNO_position pos = {};
                        for (int b = 0; b < ic; b++)
                        {
                            char *str = PQgetvalue(pgResultPos, a, b);
                            if (b == 1)
                            {
                                strcpy(pos.sys_id, str);
                            }
                            else if (b == 2)
                            {
                                strcpy(pos.symbol, str);
                            }
                            else if (b == 3)
                            {
                                pos.qty = atoi(str);
                            }
                            else if (b == 4)
                            {
                                pos.price = atof(str);
                            }
                            else if (b == 5)
                            {
                                pos.ltp = atof(str);
                            }
                            else if (b == 6)
                            {
                                printf("expiry is %s\n", str);

                                char formattedExpiry[100];
                                ConvertDbDateToCFormat(str, formattedExpiry,
                                                       sizeof(formattedExpiry));
                                strcpy(pos.expiry, formattedExpiry);
                            }
                            else if (b == 7)
                            {
                                pos.strike = atof(str);
                            }
                            else if (b == 8)
                            {
                                if (strcmp(str, "PE") == 0)
                                {
                                    pos.optType = PE;
                                }
                                else if (strcmp(str, "CE") == 0)
                                {
                                    pos.optType = CE;
                                }
                                else
                            {
                                    pos.optType = NA;
                                }
                            }
                            else if (b == 9)
                            {
                                if (strcmp(str, "OPTIDX") == 0)
                                {
                                    pos.instType = OPTIDX;
                                }
                                else if (strcmp(str, "OPTSTK") == 0)
                                {
                                    pos.instType = OPTSTK;
                                }
                                else if (strcmp(str, "FUTIDX") == 0)
                                {
                                    pos.instType = FUTIDX;
                                }
                                else if (strcmp (str, "FUTSTK") == 0)
                                {
                                    pos.instType = FUTSTK;
                                }
                            }
                        }
                        strat.fpositions[++strat.currFPosIndex] = pos; 
                    }
                    PQclear(pgResultPos);
                }
            }
        }
        state->strategies[++state->currStratIndex] = strat;
    }

    PQclear(pgResult);
}

void formatDateForDiv(const char *in, char *out)
{
    sprintf(out, "%.2s/%.2s/%.4s",
            in + 6,   // DD
            in + 4,   // MM
            in);      // YYYY
}

void
LoadDividend(Dividend *div, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 6)
        {
            strcpy(div->isin, token);
        }
        else if (i == 9)
        {
            char formattedDate[11];  // "DD/MM/YYYY" + '\0'
            formatDateForDiv(token, formattedDate);
            strcpy(div->exDate, formattedDate);
        }
        else if (i ==  13)
        {
            strcpy(div->type, token);
        }
        else if (i ==  16)
        {
            div->div = (real64)atof(token);
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

    FILE *exchangeRateFile = fopen("exchange_rate_21.csv", "r");
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

        PGresult *pgResult = executeQuery(conn, query);
        state.exRates[i - 1] = exRate; // it's a copy here.
        printf("ex rate is %f\n", state.exRates[i - 1].rate);
        PQclear(pgResult);
    }

    // onboard investor and strategy if new.
    // step 0: create new strategy.
    FILE *stratFile = fopen("ab_strat.csv", "r");
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
        // strategy.cash = 14451145.95; // inr
        strategy.cash = 0;
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

        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);
        i++;
    }


    // step 1: the client_master.csv file.
    Investor inv = {};
    FILE *clientFile = fopen("ab_inv.csv", "r");
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

        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);
    }

    // step 2: the subscription file with accounting.
    FILE *subsFile = fopen("ab_subs_upa.csv", "r");
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

            PGresult *pgResult = executeQuery(conn, query);

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
            pgResult = executeQuery(conn, query);
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
        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = entry;
        printf("entry name is %s and value is %f\n", entry.accountName, entry.debit);
        i++;
    }


    // step 3: process the bank transfer.
    FILE *bankFile = fopen("ab_bank.csv", "r");
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
        // /r for the windows files! trailing at the end of line.
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            len--;
        } 
        ++state.strategies[state.currStratIndex].currJournalId;

        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state.strategies[state.currStratIndex].currJournalId;
        liabEntry.id = state.strategies[state.currStratIndex].currJournalId;
        if (i == 3)
        {
            AccountFromBank(&assetEntry, &liabEntry, line, INR);
        }
        else
        {
            AccountFromBank(&assetEntry, &liabEntry, line, USD);
        }
        
        // insert or update the liabEntry bank acc.
        char query[1024];
        sprintf(query,
                "SELECT * FROM bank_account where symbol = '%s'",
                liabEntry.accountName);

        PGresult *pgResult = executeQuery(conn, query);
        int rows = PQntuples(pgResult);
        if (rows == 0)
        {
            fprintf(stderr, "No bank acc found matching symbol: \n");
            PQclear(pgResult);
            // insert the bank account.
            Bank_account acc = {};
            strcpy(acc.symbol, liabEntry.accountName);
            acc.balance = (0 - liabEntry.credit);
            acc.currency = liabEntry.currency;
            snprintf(query, sizeof(query),
                     "INSERT INTO bank_account (strategy_id, symbol, balance, currency) "
                     "VALUES (%d, '%s', %f, '%s');",
                     stratId,
                     acc.symbol,
                     acc.balance,
                     acc.currency == USD ? "USD" : "INR" 
                     );
            PGresult *pgResult = executeQuery(conn, query);
            PQclear(pgResult);

            // insert in memory as well.
            state.strategies[state.currStratIndex]
                .accs[++state.strategies[state.currStratIndex].currAccIndex] = acc;
        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state.strategies[state.currStratIndex].currAccIndex;
                 i++)
            {
                if(strcmp(state.strategies[state.currStratIndex].accs[i].symbol,
                          liabEntry.accountName) == 0)   
                {
                    printf("bal before %f\n", 
                           state.strategies[state.currStratIndex].accs[i].balance);
                    state.strategies[state.currStratIndex].accs[i].balance -=
                    liabEntry.credit;
                    printf("bal after %f\n", 
                           state.strategies[state.currStratIndex].accs[i].balance);

                    // db.
                    snprintf(query, sizeof(query),
                             "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                             state.strategies[state.currStratIndex].accs[i].balance,
                             liabEntry.accountName);
                    PGresult *pgResult = executeQuery(conn, query);
                    PQclear(pgResult);
                }
            }
            PQclear(pgResult);
        }
        // insert or update the assetEntry bank acc.
        printf("asset entry accoutnn name is %s\n", assetEntry.accountName);
        sprintf(query,
                "SELECT * FROM bank_account where symbol = '%s'",
                assetEntry.accountName);

        pgResult = executeQuery(conn, query);
        rows = PQntuples(pgResult);
        if (rows == 0)
        {
            fprintf(stderr, "No bank acc found matching symbol: \n");
            PQclear(pgResult);
            // insert the bank account.
            Bank_account acc = {};
            strcpy(acc.symbol, assetEntry.accountName);
            acc.balance = assetEntry.debit;
            acc.currency = assetEntry.currency;
            snprintf(query, sizeof(query),
                     "INSERT INTO bank_account (strategy_id, symbol, balance, currency) "
                     "VALUES (%d, '%s', %f, '%s');",
                     stratId,
                     acc.symbol,
                     acc.balance,
                     acc.currency == USD ? "USD" : "INR" 
                     );
            PGresult *pgResult = executeQuery(conn, query);
            PQclear(pgResult);

            // insert in memory as well.
            state.strategies[state.currStratIndex]
                .accs[++state.strategies[state.currStratIndex].currAccIndex] = acc;
        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state.strategies[state.currStratIndex].currAccIndex;
            i++)
            {
                if(strcmp(state.strategies[state.currStratIndex].accs[i].symbol,
                          assetEntry.accountName) == 0)   
                {
                    state.strategies[state.currStratIndex].accs[i].balance +=
                        assetEntry.debit;
                    // db.
                    snprintf(query, sizeof(query),
                             "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
                             state.strategies[state.currStratIndex].accs[i].balance,
                             assetEntry.accountName);
                    PGresult *pgResult = executeQuery(conn, query);
                    PQclear(pgResult);
                }
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
    FILE *reverseFile = fopen("ab_subs_upa_rev.csv", "r");
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
        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }


    // step 5: fund cashflow file.
    FILE *cashflowFile = fopen("ab_cashflow.csv", "r");
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
        PGresult *pgResult = executeQuery(conn, query);
        PQclear(pgResult);
        state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = assetEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }


    // step 6 : unit allotment.
    FILE *unitFile = fopen("ab_units.csv", "r");
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

    // step 7: fund expense investor file.
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
    //     char query[1024];
    //     snprintf(query, sizeof(query),
    //              "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
    //              "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
    //              stratId,
    //              LedgerEntryTypeStrings[assetEntry.type], // Converts enum integer index to matching string literal
    //              assetEntry.accountName,
    //              assetEntry.debit,
    //              assetEntry.credit,
    //              assetEntry.memo,
    //              assetEntry.currency == USD ? "USD" : "INR" 
    //              );
    //     PGresult *pgResult = executeQuery(conn, query);
    //     PQclear(pgResult);
    //
    //     snprintf(query, sizeof(query),
    //              "INSERT INTO ledger_entry (strategy_id, type, account_name, debit, credit, memo, currency) "
    //              "VALUES (%d, '%s', '%s', %f, %f, '%s', '%s');",
    //              stratId,
    //              LedgerEntryTypeStrings[liabEntry.type], // Converts enum integer index to matching string literal
    //              liabEntry.accountName,
    //              liabEntry.debit,
    //              liabEntry.credit,
    //              liabEntry.memo,
    //              liabEntry.currency == USD ? "USD" : "INR" 
    //              );
    //     pgResult = executeQuery(conn, query);
    //     PQclear(pgResult);
    //     state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = assetEntry;
    //     state.strategies[state.currStratIndex].ledger[++state.strategies[state.currStratIndex].currEntryId] = liabEntry;
    //     printf("entry name is %s and value is %f\n", assetEntry.accountName,
    //            assetEntry.debit);
    //     i++;
    // }


    int stratIndex = -1;
    for (int i = 0; i < state.currStratIndex + 1; i++)
    {
        if (strcmp("31500012A", state.strategies[i].symbol) == 0)
        {
            stratIndex = i;
            break;
        }
    }
    printf("strat index is %d\n", stratIndex);

    /* process dividend file, just the code for now */
    // FILE *DivFile = fopen("div.csv", "r");
    // if (DivFile == NULL)
    // {
    //     printf("sorry, couldn't upload file!\n");
    //     return -1;
    // }
    //
    // while (fgets(line, sizeof(line), DivFile))
    // {
    //     char *tmp = strchr(line, '\n');
    //     if (tmp) *tmp = '\0';
    //     Dividend div = {};
    //     LoadDividend(&div, line);
    //     // only process if the ex date is today.
    //     if (strcmp(div.exDate, today) == 0)
    //     {
    //         for (int j = 0;
    //         j <= state.strategies[stratIndex].currPosIndex;
    //         j++)
    //         {
    //             PositionEquity pos = state.strategies[stratIndex].positions[j];
    //             if (strcmp(pos.isin, div.isin) == 0)
    //             {
    //                 // make the dividend income.
    //                 // NOTE(Akhil): 2 stands for the equites bank acc.
    //                 state.strategies[stratIndex].accs[2].balance += div.div * pos.qty;
    //                 char query[1024];
    //                 snprintf(query, sizeof(query),
    //                          "UPDATE bank_account SET balance = %f WHERE symbol = '%s'",
    //                          state.strategies[stratIndex].accs[2].balance,
    //                          state.strategies[stratIndex].accs[2].symbol
    //                          );
    //                 PGresult *pgResult = executeQuery(conn, query);
    //                 PQclear(pgResult);
    //                 // make the ledger entries and persist them as well.
    //                 LedgerEntry assetEntry = {};
    //                 LedgerEntry liabEntry = {};
    //                 ++state.strategies[state.currStratIndex].currJournalId;
    //                 strcat(assetEntry.accountName, div.isin);
    //                 strcat(assetEntry.accountName, "_DIV");
    //                 assetEntry.type = ASSET;
    //                 assetEntry.currency = INR;
    //                 assetEntry.debit = abs(pos.qty * div.div);
    //                 assetEntry.id = state.strategies[state.currStratIndex].
    //                     currJournalId;
    //                 strcpy(liabEntry.accountName, "DIV_CASH_USD");
    //                 liabEntry.credit = abs(pos.qty * div.div);
    //                 liabEntry.type = REVENUE;
    //                 liabEntry.id = state.strategies[state.currStratIndex].
    //                     currJournalId;
    //                 liabEntry.currency = INR;
    //                 state.strategies[state.currStratIndex].
    //                     ledger[++state.strategies[state.currStratIndex].
    //                     currEntryId] = assetEntry;
    //                 state.strategies[state.currStratIndex].
    //                     ledger[++state.strategies[state.currStratIndex].
    //                     currEntryId] = liabEntry;
    //             }
    //         }
    //     }
    //
    //     i++;
    // }

    char query[1024];
    PGresult *pgResult;
    printFundLedger(&state);

    /* read the fno trades and make the positions */
    FILE *FTradesFile = fopen("ab_trades_21.csv", "r");
    if (FTradesFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    stratIndex = processTrades(FTradesFile, conn, stratId, &state);

    //upload the bhavcopy for FNO.
    FILE *FBhavFile = fopen("ab_bhav_21.csv", "r");
    if (FBhavFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }
    processBhav(FBhavFile, conn, stratId, stratIndex, &state);

    /* NOTE(Akhil): Update the bhav's of unknown symbols manually here
        usually the guy has a special file 21 price_update_us where
        he gives the ltp against the system generated symbol
        Also remember the uidff format of bse, that we need to be able
        to parse for fno */

    /* run the mtm process, i.e process variation settlements for
       open futures positions: net_qty * (ltp - prev_price) */
    makeVariationSettlements(&state, conn, stratId, stratIndex);

    // get the total units from all the investors for a strategy.
    // real64 totalUnits = 1007.729 + 175.444;
    real64 totalUnits = 0;
    for (int i = 0; i < state.strategies[stratIndex].currInvestorIndex + 1; i++)
    {
        Investor inv = state.strategies[stratIndex].investors[i];
        totalUnits += inv.units;
    }

    pgResult = executeQuery(conn, query);
    PQclear(pgResult);
    printNav(&state, &exRate, totalUnits, stratIndex, conn, stratId);

    // /* 2ND DAY------------------------------------------ */
    // Load the state from the db.
    state = {};
    loadStateFromDB(&state, conn);

    collapsePositions(&state, stratIndex);

    printFPositions(&state, stratIndex);

    FILE *EFile = fopen("exchange_rate_24.csv", "r");
    if (EFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    // update the ex rate for the second day.
    processExRate(EFile, &state, &exRate);

    FILE *FTradessFile = fopen("ab_trades_24.csv", "r");
    if (FTradessFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    // process trades for 12th june.
    stratIndex = processTrades(FTradessFile, conn, stratId, &state);
    FILE *FBhavvFile = fopen("ab_bhav_24.csv", "r");
    if (FBhavvFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    // process bhavcopy of 12th june.
    processBhav(FBhavvFile, conn, stratId, stratIndex, &state);


    makeVariationSettlements(&state, conn, stratId, stratIndex);
    printFundLedger(&state);
    printNav(&state, &exRate, totalUnits, stratIndex, conn, stratId);

    /* 3RD DAY------------------------------------------ */
    state = {};
    loadStateFromDB(&state, conn);

    collapsePositions(&state, stratIndex);

    printFPositions(&state, stratIndex);

    FILE *EeFile = fopen("exchange_rate_28.csv", "r");
    if (EeFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    // update the ex rate for the second day.
    processExRate(EeFile, &state, &exRate);

    FILE *FTradesssFile = fopen("ab_trades_28.csv", "r");
    if (FTradesssFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    stratIndex = processTrades(FTradesssFile, conn, stratId, &state);
    
    makeVariationSettlements(&state, conn, stratId, stratIndex);
    printFundLedger(&state);
    printNav(&state, &exRate, totalUnits, stratIndex, conn, stratId);
    PQfinish(conn);
}
