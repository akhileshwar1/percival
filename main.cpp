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
        printf("ex rate is %f\n", exRate.rate);
    }

    FILE *securityFile = fopen("securities.csv", "r");
    if (securityFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        return -1;
    }

    i = 0;
    // upload the securities.
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
        printf("security is %s\n", sec.name);
    }
}
