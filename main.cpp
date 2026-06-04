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
    char *date;
    real64 rate;
    Currency_code base;
} Exchange_rate;

typedef struct
{
    char *symbol;
    char *date;
    char *name;
} Security;

typedef struct
{
    int id;
    char *name;
    int units;
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
    char *symbol;
    char *brokerCode;
    char *date;
    int qty;
    real64 price;
    real64 brokerage;
    real64 serviceTax;
    Trans_type transType;
} Trade;

typedef struct
{
    char *symbol;
    real64 amount;
    real64 nav;
    Investor investors[MAX_INVESTORS];
    int currInvestorIndex;
} Strategy;

typedef struct
{
    char *symbol;
    int qty;
    real64 price;
} Position;

typedef struct
{
    Exchange_rate exRates[MAX_EX_RATES];
    Security secs[MAX_SECURITIES];
    Strategy strategies[MAX_STRATEGIES];
    Position positions[MAX_POSITIONS];
    int currStratIndex;
    int currPosIndex;
} State;

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
            exRate->date= token;
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
            sec->symbol = token;
        }
        else if (i ==  1)
        {
            sec->date= token;
        }
        else if (i ==  2)
        {
            sec->name = token;
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
            strat->symbol = token;
        }
        else if (i ==  4)
        {
            strat->amount = (real64)atof(token);
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
            inv->name = token;
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
LoadTrade(Trade *trade, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            trade->symbol = token;
        }
        else if (i ==  1)
        {
            trade->brokerCode = token;
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
            trade->date = token;
        }
        token = strtok(NULL, ",");
        i++;
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
    state.currPosIndex = -1;
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
        Investor inv = {};
        // NOTE(akhil) : Same line being traversed twice.
        char copyLine[1024]; 
        strcpy(copyLine, line);
        LoadStrategy(&strat, line);
        LoadInvestor(&inv, copyLine);

        // add strategy to the state if not present.
        int found = 0;
        int foundIndex = -1;
        for (int i = 0; i < MAX_STRATEGIES; i++)
        {
            if (state.strategies[i].symbol == strat.symbol)
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
        printf("investor is %d\n", state.strategies[0].investors[0].units);
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
        for (int i = 0; i < state.currPosIndex + 1; i++)
        {
            if (strcmp(state.positions[i].symbol, trade.symbol) == 0)
            {
                if (trade.transType == MB ||
                    trade.transType == LB)
                {
                    int qtyAfterFee =
                        (trade.qty * trade.price *
                        (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                        / trade.price;
                    state.positions[i].price = 
                        ((state.positions[i].price *
                        state.positions[i].qty) +
                        (trade.price * qtyAfterFee)) /
                        state.positions[i].qty + qtyAfterFee;
                    state.positions[i].qty += qtyAfterFee;
                }
                else
                {
                    double priceAfterFee =
                        (trade.qty * trade.price *
                        (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                        / trade.qty;
                    state.positions[i].price = 
                        ((state.positions[i].price *
                        state.positions[i].qty) +
                        (trade.qty * priceAfterFee)) /
                        state.positions[i].qty + trade.qty;
                    state.positions[i].qty += trade.qty;
                }

                found = 1;
                break;
            }
        }
        
        // not found, add the position.
        if (found != 1)
        {
            // add the position
            Position pos = {};
            pos.symbol = trade.symbol;
            if (trade.transType == MB ||
                trade.transType == LB)
            {
                int qtyAfterFee =
                    (trade.qty * trade.price *
                    (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                    / trade.price;
                pos.price = trade.price;
                pos.qty = qtyAfterFee;
            }
            else
            {
                double priceAfterFee =
                    (trade.qty * trade.price *
                    (1.0 - (trade.brokerage + trade.serviceTax) / 100.0))
                    / trade.qty;
                pos.price = priceAfterFee;
                pos.qty = trade.qty;
            }
            state.positions[++state.currPosIndex] = pos;
        }
        printf("pos qty is %d\n", state.positions[0].qty);
    }
}
