#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <ctype.h>
#include <time.h>
#include <cjson/cJSON.h>

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;
typedef uint16_t uint16;

#define kilobytes(value) (value*1024LL)
#define megabytes(value) (kilobytes(value*1024LL))
#define gigabytes(value) (megabytes(value*1024LL))

#define PORT 8888
#define POSTBUFFERSIZE 512
#define MAX_STRATEGIES 100
#define MAX_INVESTORS 1000
#define MAX_SECURITIES 1000
#define MAX_POSITIONS 1000
#define MAX_EX_RATES 5
#define MAX_ACCS 10 
#define MAX_LEDGER_ENTRIES 1000 

#define ASKPAGE \
"<html><body>\n" \
"Upload a file, please!<br>\n" \
"There are %u clients uploading at the moment.<br>\n" \
"<form action=\"/filepost\" method=\"post\" enctype=\"multipart/form-data\">\n" \
"<input name=\"file\" type=\"file\">\n" \
"<input type=\"submit\" value=\" Send \"></form>\n" \
"</body></html>"
// static const char *busypage =
// "<html><body>This server is busy, please try again later.</body></html>";
char completepage[] =
"<html><body>The upload has been completed.</body></html>";
char errorpage[] =
"<html><body>This doesn't seem to be right.</body></html>";
char servererrorpage[] =
"<html><body>Invalid request.</body></html>";
/* static const char *fileexistspage =
 "<html><body>This file already exists.</body></html>"; */
char fileioerror[] =
"<html><body>IO error writing to disk.</body></html>";
char invalidfileformaterror[] =
"<html><body>invalid file format error.</body></html>";
char postprocerror[] =
"<html><head><title>Error</title></head><body>Error processing POST data</body></html>";

char exchangeRateHeader[] =
    "currency_code,date,rate,base";
char strategyHeader[] =
    "GROUPCODE,GROUPNAME,GROUPCONTACT,ACCOUNTCODE,SALUTATION,FIRSTNAME,MIDDLENAME,LASTNAME,ADDR1,ADDR2,CITY,STATE,PINCD,PHONE,PHONEWORK,MOBILE,FAX,EMAIL,AUTOEMAILFLAG,OCCUPATION,CLIENTSTATUS,PAN,TAXABLE,INCEPTIONDATE,MATURITYDATE,ACCOUNTTYPE,USERNAME,BANKCD1,BANKACID1,DEPID,DPID,DPCLIENTID,BILLGROUP,MAPIN,MFINVESTNUMBER,REFERENCECODE2,REFERENCECODE3,REFERENCECODE4,SCHEMECODE,INTERCODE,FIRMCODE,ADVISORCODE,RELMGRCODE,BRANCHCODE,COUNTRY,NATIONALITY,CAPITALCOMMITEMENTAMT,PERFREPORTINGDATE,BANKACT_NAME,BANKACT_BRANCH,BANKACT_TYPE,BANKACT_NO,BANKACT_MICR,NEFT,RTGS,H1DEPCODE,H1DPID,H1DPCLIENTID,H1DOB,H1GUARDIAN,H1GENDER,H1FATHER_HUSB,H1WARD,H1CIRCLE,H1TANNO,SHAREREPORTS,H2NAME,H2PANNO,H2RELATION,H2DOB,H2GENDER,H2FATHER_HUSB,BANKACT2_NAME,BANKACT2_BRANCH,BANKACT2_NO,BANKACT2_TYPE,BANKACT2_MICR,H2NEFT,H2RTGS,H2DEPCODE,H2DPID,H2DPNAME,H2DPCLIENTID,H3NAME,H3PANNO,H3RELATION,H3DOB,H3GENDER,H3FATHER_HUSB,BANKACT3_NAME,BANKACT3_BRANCH,BANKACT3_NO,BANKACT3_TYPE,BANKACT3_MICR,H3NEFT,H3RTGS,H3DEPCODE,H3DPID,H3DPNAME,H3DPCLIENTID,POOLMAPINID,BANKNAME,OPERATIONTYPE,MAILH1ADD1,MAILH1ADD2,MAILH1CITY,MAILH1STATE,MAILH1PIN,MAILH1PHONE,MAILH1MOBILE,MAILH1PAGER,MAILH1FAX,MAILH1EMAILID,HOFFLAG,CLIENTID,WEALTHADVISORNAME,ARNNO,H1CKYC,H1AADHAR,H1FATCA,H1UBO,H2CKYC,H2AADHAR,H2FATCA,H2UBO,H3CKYC,H3AADHAR,H3FATCA,H3UBO,ACCOUNTINGTXN,TXNFEETAKENAS,STTTAKENAS,BASEFUNDID,H1MODEOFHOLDING,NOMINEENAME,NOMINEERELATION,NOMINEEADD1,NOMINEEADD2,NOMINEECITY,NOMINEESTATE,NOMINEEPIN,NOMINEEPHONE,NOMINEEFAX,GUARDIANNAME,GUARDIANADD1,GUARDIANADD2,GUARDIANCITY,GUARDIANSTATE,GUARDIANPIN,GUARDIANPHONE,GUARDIANFAX,NOMDOB,NOMPAN,NGDOB,NGPAN,NGAADHAR,NGCKYC,NGFATCA,NGUBO,BANKACTYPE,BANKBRANCH,BANKCODE2,BANKACID2,BANKNAME2,BANKACTYPE2,BANKBRANCH2,INCCATG,MODELPORTFOLIO,PLANALLOWED,RECOPRODUCT,RMRIGHTS,ADVISORRIGHTS,FMRIGHTS,CLIENTRIGHTS,HOFRESTRICTED,SHARES,BOND_DEB,MF,FUTURES,OPTIONS,EQUITY_DEB,OTHER_ASSETS,PERFCLASSIFYDAILY,ONLYLOWESTCLASSIFY,RELMAPPING1,RISKPROFILENAME,CLIENTCATEGORY,ACCREDITEDINVESTOR,REFCODE5,REFCODE6,REFCODE7,REFCODE8,REFCODE9,REFCODE10";
char investorHeader[] =
    "GROUPCODE,GROUPNAME,GROUPCONTACT,ACCOUNTCODE,SALUTATION,FIRSTNAME,MIDDLENAME,LASTNAME,ADDR1,ADDR2,CITY,STATE,PINCD,PHONE,PHONEWORK,MOBILE,FAX,EMAIL,AUTOEMAILFLAG,OCCUPATION,CLIENTSTATUS,PAN,TAXABLE,INCEPTIONDATE,MATURITYDATE,ACCOUNTTYPE,USERNAME,BANKCD1,BANKACID1,DEPID,DPID,DPCLIENTID,BILLGROUP,MAPIN,MFINVESTNUMBER,REFERENCECODE2,REFERENCECODE3,REFERENCECODE4,SCHEMECODE,INTERCODE,FIRMCODE,ADVISORCODE,RELMGRCODE,BRANCHCODE,COUNTRY,NATIONALITY,CAPITALCOMMITEMENTAMT,PERFREPORTINGDATE,BANKACT_NAME,BANKACT_BRANCH,BANKACT_TYPE,BANKACT_NO,BANKACT_MICR,NEFT,RTGS,H1DEPCODE,H1DPID,H1DPCLIENTID,H1DOB,H1GUARDIAN,H1GENDER,H1FATHER_HUSB,H1WARD,H1CIRCLE,H1TANNO,SHAREREPORTS,H2NAME,H2PANNO,H2RELATION,H2DOB,H2GENDER,H2FATHER_HUSB,BANKACT2_NAME,BANKACT2_BRANCH,BANKACT2_NO,BANKACT2_TYPE,BANKACT2_MICR,H2NEFT,H2RTGS,H2DEPCODE,H2DPID,H2DPNAME,H2DPCLIENTID,H3NAME,H3PANNO,H3RELATION,H3DOB,H3GENDER,H3FATHER_HUSB,BANKACT3_NAME,BANKACT3_BRANCH,BANKACT3_NO,BANKACT3_TYPE,BANKACT3_MICR,H3NEFT,H3RTGS,H3DEPCODE,H3DPID,H3DPNAME,H3DPCLIENTID,POOLMAPINID,BANKNAME,OPERATIONTYPE,MAILH1ADD1,MAILH1ADD2,MAILH1CITY,MAILH1STATE,MAILH1PIN,MAILH1PHONE,MAILH1MOBILE,MAILH1PAGER,MAILH1FAX,MAILH1EMAILID,HOFFLAG,CLIENTID,WEALTHADVISORNAME,ARNNO,H1CKYC,H1AADHAR,H1FATCA,H1UBO,H2CKYC,H2AADHAR,H2FATCA,H2UBO,H3CKYC,H3AADHAR,H3FATCA,H3UBO,ACCOUNTINGTXN,TXNFEETAKENAS,STTTAKENAS,BASEFUNDID,H1MODEOFHOLDING,NOMINEENAME,NOMINEERELATION,NOMINEEADD1,NOMINEEADD2,NOMINEECITY,NOMINEESTATE,NOMINEEPIN,NOMINEEPHONE,NOMINEEFAX,GUARDIANNAME,GUARDIANADD1,GUARDIANADD2,GUARDIANCITY,GUARDIANSTATE,GUARDIANPIN,GUARDIANPHONE,GUARDIANFAX,NOMDOB,NOMPAN,NGDOB,NGPAN,NGAADHAR,NGCKYC,NGFATCA,NGUBO,BANKACTYPE,BANKBRANCH,BANKCODE2,BANKACID2,BANKNAME2,BANKACTYPE2,BANKBRANCH2,INCCATG,MODELPORTFOLIO,PLANALLOWED,RECOPRODUCT,RMRIGHTS,ADVISORRIGHTS,FMRIGHTS,CLIENTRIGHTS,HOFRESTRICTED,SHARES,BOND_DEB,MF,FUTURES,OPTIONS,EQUITY_DEB,OTHER_ASSETS,PERFCLASSIFYDAILY,ONLYLOWESTCLASSIFY,RELMAPPING1,RISKPROFILENAME,CLIENTCATEGORY,ACCREDITEDINVESTOR,REFCODE5,REFCODE6,REFCODE7,REFCODE8,REFCODE9,REFCODE10";
char subsHeader[] =
    "BROKERCODE,BROKERACID,SYMBOLCODE,EXCHG,TRANTYPE,DATEPUR_ACQUI,SETDATE,QUANTITY,RATE,BROKERAGEPERSHARE,SERVICETAX,SETDATEFLAG,MKTRATE,CASHSYMBOLCODE,TRANEXPENSE,ACCRUEDINTEREST,BLOCKID,TRANSREF,DESCMEMO,CHEQUENO,CHEQUEDTL,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG,TRFEXCHGRATE,CASHFLAG,BANKREF,CASHSETDATE";
char bankHeader[] =
    "TRANSFERDATE,CLIENTCODE,TRANSTYPE,FROMBANKCODE,FROMBANKACID,TRFBALANCEFROM,TRFBALANCETO,TOBANKCODE,TOBANKACID";
char revHeader[] =
    "BROKERCODE,BROKERACID,SYMBOLCODE,EXCHG,TRANTYPE,DATEPUR_ACQUI,SETDATE,QUANTITY,RATE,BROKERAGEPERSHARE,SERVICETAX,SETDATEFLAG,MKTRATE,CASHSYMBOLCODE,TRANEXPENSE,ACCRUEDINTEREST,BLOCKID,TRANSREF,DESCMEMO,CHEQUENO,CHEQUEDTL,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG,TRFEXCHGRATE,CASHFLAG,BANKREF,CASHSETDATE";
char cashflowHeader [] =
    "PlanId,TranDate,TranType,CashFlowValue,NAV,NAV Date,Bank Code,Bank Ac Id ,Ban Ac type,Chq No,Chq Detl,Remarks,NAV/Unit Flag,Face Value";
char unitsHeader [] =
    "BROKERCODE,BROKERACID,SYMBOLCODE,EXCHG,TRANTYPE,DATEPUR_ACQUI,SETDATE,QUANTITY,RATE,BROKERAGEPERSHARE,SERVICETAX,SETDATEFLAG,MKTRATE,CASHSYMBOLCODE,TRANEXPENSE,ACCRUEDINTEREST,BLOCKID,TRANSREF,DESCMEMO,CHEQUENO,CHEQUEDTL,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG,TRFEXCHGRATE,CASHFLAG,BANKREF,CASHSETDATE";
char expenseHeader [] =
    "BROKERCODE,BROKERACID,SYMBOLCODE,EXCHG,TRANTYPE,DATEPUR_ACQUI,SETDATE,QUANTITY,RATE,BROKERAGEPERSHARE,SERVICETAX,SETDATEFLAG,MKTRATE,CASHSYMBOLCODE,TRANEXPENSE,ACCRUEDINTEREST,BLOCKID,TRANSREF,DESCMEMO,CHEQUENO,CHEQUEDTL,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG,TRFEXCHGRATE,CASHFLAG,BANKREF,CASHSETDATE";
char fnoTradesHeader [] =
    "Broker Code,Broker Account,Parent Symbol,Exchange,Transaction Type,Transaction Date,Settlement date,Quantity,Rate,Brokerage per share,Service tax,Set date flag,Filler,Cash Symbol,STT,Filler,Block Id,Expiry date,Strike price,Option type,Series,BANKREF,DESCMEMO,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG";
char offcashflowHeader [] =
    "TranDate,PlanId,TranType,CashFlowValue,NAV,NAV Date,Bank Code,Bank Ac Id ,Ban Ac type,Chq No,Chq Detl,Remarks,NAV/Unit Flag,Face Value";
char offredeemHeader [] =
    "BROKERCODE,BROKERACID,SYMBOLCODE,EXCHG,TRANTYPE,DATEPUR_ACQUI,SETDATE,QUANTITY,RATE,BROKERAGEPERSHARE,SERVICETAX,SETDATEFLAG,MKTRATE,CASHSYMBOLCODE,TRANEXPENSE,ACCRUEDINTEREST,BLOCKID,TRANSREF,DESCMEMO,CHEQUENO,CHEQUEDTL,MULTICURRENCYTRANFLAG,SETTLEMENTCASHSYMBOLCODE,EXCHGRATE,EXCHGRATEFLAG,TRFEXCHGRATE,CASHFLAG,BANKREF,CASHSETDATE";


/* --------------- Structs ------------------------------------------------*/
typedef enum
{
    GET = 0,
    POST = 1
} ConnectionType;

/**
* Information we keep per connection.
*/
typedef struct
{
    ConnectionType connectiontype;
    /**
     * Handle to the POST processing state.
     */
    struct MHD_PostProcessor *postprocessor;
    /**
     * File handle where we write uploaded data.
     */
    FILE *fp;
    /**
     * HTTP response body we will return, NULL if not yet known.
     */
    char *answerstring;
    /**
     * HTTP status code we will return, 0 for undecided.
     */
    unsigned int answercode;

    char strategySymbol[100];
    char hurdlerate[100];
    char frequency[100];
    char perfFee[100];
    char billSymbol[100];
    char invName[100];

    char date[100];
    char fromDate[100];
    char toDate[100];
    char rollbackDate[100];
    char snapshotDate[100];
} connection_info_struct;

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

/* struct used to upload db balances
   from another platform */
typedef struct
{
    char symbol[100];
    real64 balance;
} Balance;

typedef struct
{
    char symbol[100];
    real64 ltp;
    char date[100];
} Bhav;

typedef struct
{
    char isin[100];
    char symbol[100];
    char date[100];
    char name[100];
    char sys_id[100];
} Security;

typedef enum
{
    ANNIVERSARY,
    ANNUAL,
    FIN, 
} Bill_group_frequency;

typedef struct
{
    char symbol[100];
    char date[100];
    real64 hurdlerate;
    real64 perfFee;
    Bill_group_frequency frequency;
} Bill_group;

typedef enum
{
    INVESTOR_PENDING,
    INVESTOR_ONBOARDED,
    INVESTOR_OFFBOARDED
} InvestorStatus;

typedef struct
{
    int id;
    char name[100];
    char inceptionDate[100];
    char lastPerfFeeDate[100];
    char billGroup[100];
    real64 units;
    real64 lastNav;
    InvestorStatus status;
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
    char date[100];
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
    real64 pnl;
    char sys_id[100];
} PositionEquity;

typedef enum
{
    PENDING,
    DONE
} Div_status;

typedef struct
{
    char isin[100];
    char exDate[100];
    char type[100]; //TODO(Akhil) : change to enum later.
    real64 div;
    char externalId[100]; // corp action id.
    Div_status status;
    char paymentDate[100]; // TODO(Akhil): Need to figure out where this will come from.
} Dividend;

typedef struct
{
    char symbol[100];
    int qty;
    real64 price;
    real64 ltp;
    real64 pnl;
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
    int id; // refers to the journal id.
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
    real64 receivable;
    Investor investors[MAX_INVESTORS];
    PositionEquity positions[MAX_POSITIONS];
    FNO_position fpositions[MAX_POSITIONS];
    Bank_account accs[MAX_ACCS];
    int id;
    int currPosIndex;
    int currFPosIndex;
    int currInvestorIndex;
    LedgerEntry ledger[MAX_LEDGER_ENTRIES];
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
    int currSecIDCount;
    int currOptIDCount;
    PGconn *db;
} State;

/* --------------- Utils ------------------------------------------------*/
bool
isDateValid(char *lastDate, char *currDate, Bill_group_frequency freq)
{
    int day, month, year;
    int lastDay, lastMonth, lastYear;
    printf("last date date %s %s is \n", lastDate, currDate);
    if (sscanf(lastDate, "%d/%d/%d", &lastDay, &lastMonth, &lastYear) != 3) {
        return false;
    }

    if (sscanf(currDate, "%d/%d/%d", &day, &month, &year) != 3) {
        return false;
    }

    if (freq == ANNIVERSARY)
    {
        return (year - lastYear == 1);   /* one year between two dates */
    }
    else if (freq == ANNUAL)
    {
        return (day == 1 && month == 1); /* 1st jan */
    }
    else
    {
        return (day == 1 && month == 4); /* 1st april */
    }
}

static void shiftDate(const char *input_date, char *output_date, int days) {
    struct tm time_str = {};
    int day, month, year;

    if (sscanf(input_date, "%d/%d/%d", &day, &month, &year) != 3) {
        strcpy(output_date, input_date); // Return original if parsing fails
        return;
    }

    time_str.tm_mday = day;
    time_str.tm_mon = month - 1;
    time_str.tm_year = year - 1900;

    time_str.tm_mday += days;

    mktime(&time_str);

    strftime(output_date, 11, "%d/%m/%Y", &time_str);
}

void incrementDate(const char *input_date, char *output_date) {
    shiftDate(input_date, output_date, 1);
}

void decrementDate(const char *input_date, char *output_date) {
    shiftDate(input_date, output_date, -1);
}

int ValidateCsvHeader(char *firstLine, char *expectedHeader)
{
    if (strcmp(firstLine, expectedHeader) == 0) {
        return 0; // Valid CSV header structure!
    }

    return -1;
}

/* Modifies the string in place to remove leading/trailing spaces, \r, and \n */
char* TrimString(char *str)
{
    if (str == NULL) return str;

    /* 1. Trim trailing whitespace, \r, and \n */
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }

    /* 2. Trim leading whitespace */
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    /* 3. Shift string back to origin if leading spaces were found */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    return str;
}

const char* LedgerEntryTypeStrings[] = {
    "ASSET",
    "EXPENSE",
    "LIABILITY",
    "EQUITY",
    "REVENUE"
};

const char* DivStatusStrings[] = {
    "PENDING",
    "DONE"
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

const char* CurrencyCodeStrings[] = {
    "USD",
    "INR",
};

const char* TransTypeStrings[] = {
    "MB", "MS", "LB", "LS", "MOB", "MCB", "MOS", "MCS", "FSO", "FSC", "FBO", "FBC"
};

const char* InvestorStatusStrings[] = {
    "PENDING",
    "ONBOARDED",
    "OFFBOARDED"
};

/* yyyy-mm-dd to dd/mm/yyyy */
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

/* 30-Jun-2026 to 30/06/2026 */
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

/* mm/dd/yy or mm/dd/yyyy or m/d/yy to dd/mm/yyyy */
void
mmddyyyy_to_ddmmyyyy(const char *in, char *out)
{
    int month, day, year;

    if (sscanf(in, "%d/%d/%d", &month, &day, &year) != 3)
    {
        out[0] = '\0';   // Invalid date
        return;
    }

    // Convert 2-digit year if necessary
    if (year < 100)
        year += 2000;

    sprintf(out, "%02d/%02d/%04d", day, month, year);
}

/* 30-06-2026 to 30/06/2026 */
void ConvertDateSeparator(const char *input_date, char *output, size_t output_size)
{
    int day = 0, month = 0, year = 0;

    /* 1. Extract pieces from DD-MM-YYYY layout */
    if (sscanf(input_date, "%d-%d-%d", &day, &month, &year) == 3)
    {
        /* 2. Format with zero padding into your destination buffer */
        snprintf(output, output_size, "%02d/%02d/%04d", day, month, year);
    }
    else
    {
        /* Fallback: Clear string if parsing fails */
        if (output_size > 0) output[0] = '\0';
    }
}

/* --------------- DB helpers ------------------------------------------------*/
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
DBUpdatePerfFeeInvestor(PGconn *conn,
                        char *invName,
                        char *lastPerfFeedate,
                        real64 lastNav)
{
    char query[4096];
    snprintf(query, sizeof(query),
             "UPDATE investor SET last_perf_fee_date = '%s' "
             ", last_nav = %f WHERE name = '%s';",
             lastPerfFeedate,
             lastNav,
             invName);
    PGresult *pgResult = executeQuery(conn, query);
    PQclear(pgResult);
}

void
DBLoadBillGroup(PGconn *conn, Bill_group *bill, char *billSymbol)
{
    char query[4096];
    snprintf(query, sizeof(query),
             "SELECT * from bill_group WHERE symbol = '%s';",
             billSymbol); 
    strcpy(bill->symbol, billSymbol);

    PGresult *pgResult = executeQuery(conn, query);
    if (PQntuples(pgResult) == 0)
    {
        printf("no bill group found %s\n", billSymbol);
        return;
    }
    bill->perfFee = atof(PQgetvalue(pgResult, 0, 3));
    bill->hurdlerate = atof(PQgetvalue(pgResult, 0, 2));
    char *frequencyStr = PQgetvalue(pgResult, 0, 4);
    if (0 == strcmp("ANNIVERSARY", frequencyStr)) bill->frequency = ANNIVERSARY;
    else if (0 == strcmp("ANNUAL", frequencyStr)) bill->frequency = ANNUAL;
    else if (0 == strcmp("FIN", frequencyStr)) bill->frequency = FIN;
    PQclear(pgResult);
}

void
DBgetBillSymbolFromId(PGconn *conn, int billId, char *billSymbolStr)
{
    char query[4096];
    sprintf(query,
            "SELECT symbol FROM bill_group where id = %d LIMIT 1",
            billId);
    PGresult *pgResult = executeQuery(conn, query);
    if (PQntuples(pgResult) == 0)
    {
        printf("couldn't find the bill id: %d\n", billId);
        PQclear(pgResult);
        billSymbolStr[0] = '\0';
    }
    strcpy(billSymbolStr, PQgetvalue(pgResult, 0, 0));
    PQclear(pgResult);
}

int
DBgetBillId(PGconn *conn, char *billGroupSymbol)
{
    char query[4096];
    snprintf(query, sizeof(query),
             "SELECT * FROM bill_group WHERE symbol = '%s';",
             billGroupSymbol); 

    PGresult *pgResult = executeQuery(conn, query);
    if (PQntuples(pgResult) == 0)
    {
        printf("couldn't find the bill group: %s\n", billGroupSymbol);
        PQclear(pgResult);
        return -1;
    }

    int billId = atoi(PQgetvalue(pgResult, 0, 0)); 
    PQclear(pgResult);
    return billId;
}

void
DBLoadInvestor(PGconn *conn, Investor *inv)
{
    char query[1024];
    sprintf(query,
            "SELECT * FROM investor where name = '%s' LIMIT 1",
            inv->name);

    PGresult *pgResult = executeQuery(conn, query);
    int rows = PQntuples(pgResult);
    if (rows == 0)
    {
        printf("No investor found matching name: %s\n", inv->name);
        PQclear(pgResult);
        return;
    }
    else
    {
        /* there will only be one row */
        char *idStr = PQgetvalue(pgResult, 0, 0);
        int id = atoi(idStr);
        inv->id = id;
        char *nameStr = PQgetvalue(pgResult, 0, 3);
        printf("name str is %s\n", nameStr);
        strcpy(inv->name, nameStr);
        char *unitsStr = PQgetvalue(pgResult, 0, 4);
        real64 units = atof(unitsStr);
        inv->units = units;
        inv->lastNav = atof(PQgetvalue(pgResult, 0, 5));
        ConvertDbDateToCFormat(PQgetvalue(pgResult, 0, 6),
                               inv->inceptionDate,
                               sizeof(inv->inceptionDate));
        ConvertDbDateToCFormat(PQgetvalue(pgResult, 0, 7),
                               inv->lastPerfFeeDate,
                               sizeof(inv->lastPerfFeeDate));
        inv->lastNav = atof(PQgetvalue(pgResult, 0, 5));
        char *billIdStr = PQgetvalue(pgResult, 0, 10);

        char billSymbol[100];
        DBgetBillSymbolFromId(conn, atoi(billIdStr), billSymbol);
        strcpy(inv->billGroup, billSymbol);
        PQclear(pgResult);
    }
}

real64
DBgetNAV(PGconn *conn, char *date, int stratId)
{
    char query[4096];
    sprintf(query,
            "SELECT nav FROM strategy_nav where nav_date = '%s' "
            "AND strategy_id = %d;",
            date,
            stratId);

    PGresult *pgResult = executeQuery(conn, query);
    if (PQntuples(pgResult) == 0)
    {
        PQclear(pgResult);
        return -1;
    }

    return atof(PQgetvalue(pgResult, 0, 0));
}

real64
DBGetLtpFNO(PGconn *conn,
            char *symbol,
            Opt_type optType,
            real64 strike,
            char *expiry,
            char *date,
            int stratId)
{
    char query[1024];
    sprintf(query,
            "SELECT * FROM fno_bhav where date = TO_DATE('%s', 'DD/MM/YYYY') "
            "AND opt_type = '%s' AND strike = %f AND expiry = TO_DATE('%s', 'DD/MM/YYYY') AND strategy_id = %d AND symbol = '%s'; ",
            date,
            OptTypeStrings[optType],
            strike,
            expiry,
            stratId,
            symbol);

    PGresult *pgResult = executeQuery(conn, query);

    if (PQntuples(pgResult) == 0)
    {
        PQclear(pgResult);
        return -1;
    }

    return atof(PQgetvalue(pgResult, 0, 2));
}

real64
DBGetLtpEQ(PGconn *conn, char *symbol, char *date, int stratId)
{
    char query[1024];
    sprintf(query,
            "SELECT * FROM equity_bhav where date = TO_DATE('%s', 'DD/MM/YYYY') "
            "AND strategy_id = %d AND symbol = '%s'; ",
            date,
            stratId,
            symbol);

    PGresult *pgResult = executeQuery(conn, query);

    if (PQntuples(pgResult) == 0)
    {
        PQclear(pgResult);
        return -1;
    }

    return atof(PQgetvalue(pgResult, 0, 1));
}

real64
DBGetExchangeRate(PGconn *conn, char *date, int stratId)
{
    char query[1024];
    sprintf(query,
            "SELECT * FROM exchange_rate where date = TO_DATE('%s', 'DD/MM/YYYY') "
            "AND strategy_id = %d ",
            date,
            stratId);

    PGresult *pgResult = executeQuery(conn, query);

    if (PQntuples(pgResult) == 0)
    {
        PQclear(pgResult);
        return -1;
    }

    char *rate_str = PQgetvalue(pgResult, 0, 2);
    real64 rate = atof(rate_str);
    PQclear(pgResult);
    return rate;
}

void
DBInsertLedgerEntry(PGconn *conn, LedgerEntry *entry, int stratId)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO ledger_entry (journal_id, strategy_id, type, account_name, debit, credit, memo, currency) "
             "VALUES (%d, %d, '%s', '%s', %f, %f, '%s', '%s');",
             entry->id,
             stratId,
             LedgerEntryTypeStrings[entry->type], // Converts enum integer index to matching string literal
             entry->accountName,
             entry->debit,
             entry->credit,
             entry->memo,
             entry->currency == USD ? "USD" : "INR" 
             );
    PGresult *pgResult = executeQuery(conn, query);
    PQclear(pgResult);
}

void
DBUpdateInvestorUnitsAndStatus(PGconn *conn,
                               real64 units,
                               const char *status,
                               char *invName)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE investor SET units = %f, status = '%s' WHERE name = '%s'",
             units,
             status,
             invName
             );
    PGresult *pgResult = executeQuery(conn, query);
    PQclear(pgResult);
}

void
DBInsertBankAcc(PGconn *conn, Bank_account *acc, int stratId)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "INSERT INTO bank_account (strategy_id, symbol, balance, currency) "
             "VALUES (%d, '%s', %f, '%s');",
             stratId,
             acc->symbol,
             acc->balance,
             acc->currency == USD ? "USD" : "INR" 
             ); 
    PGresult *res = executeQuery(conn, query);
    PQclear(res);
}

void
DBInsertSecurity(PGconn *conn, Security *sec, int currSecIDCount)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "BEGIN;"
             "INSERT INTO security (sys_id, isin, symbol, listing_date, name) "
             "VALUES ('%s', '%s', '%s', to_date('%s', 'DD/MM/YYYY'), '%s');"
             "UPDATE global_state SET curr_sec_id_count = %d WHERE id = 1;"
             "COMMIT;",
             sec->sys_id,
             sec->isin,
             sec->symbol,
             sec->date,
             sec->name,
             currSecIDCount); 
    PGresult *res = PQexec(conn, query);
    PQclear(res);   
}

void
DBInsertPosition(PGconn *conn, PositionEquity *pos, int stratId)
{
    char query[2096];
    snprintf(query, sizeof(query),
             "INSERT INTO position_equity (sys_id, isin, symbol, qty, price, ltp, pnl, strategy_id) "
             "VALUES ('%s', '%s', '%s', %d, %f, %f, %f, %d) "
             "ON CONFLICT (sys_id) DO UPDATE SET "
             "qty = EXCLUDED.qty, price = EXCLUDED.price, ltp = EXCLUDED.ltp, updated_at = CURRENT_TIMESTAMP;",
             pos->sys_id,
             pos->isin,
             pos->symbol,
             pos->qty,
             pos->price,
             pos->ltp,
             0.0,
             stratId);
    PGresult *res = executeQuery(conn, query);
    PQclear(res);   
}

void
DBUpdateRecv(PGconn *conn, real64 balance, int stratId)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE strategy SET receivable = %f WHERE id = %d;",
             balance,
             stratId);
    PGresult *res = executeQuery(conn, query);
    PQclear(res);
}

void
DBUpdateBankBalance(PGconn *conn, real64 balance, char *symbol)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE bank_account SET balance = %f "
             "WHERE symbol = '%s'; ",
             balance,
             symbol); 
    PGresult *res = executeQuery(conn, query);
    PQclear(res);
}

void
DBUpdateFee(PGconn *conn, real64 balance, int stratId)
{
    char query[1024];
    snprintf(query, sizeof(query),
             "UPDATE strategy SET fees_accrued = %f WHERE id = %d;",
             balance,
             stratId);
    PGresult *res = executeQuery(conn, query);
    PQclear(res);
}

/* --------------- Loaders/Parsers ------------------------------------------------*/
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
            strcpy(bhav->symbol, token); //symbol here is isin.
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
            char output[11];
            if (convert_date_format(token, output) == 0)
            {
                strcpy(exRate->date, output);
            } 
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
            strcpy(sec->symbol, token);
        }
        else if (i ==  1)
        {
            strcpy(sec->name, token);
        }
        else if (i == 5)
        {
            strcpy(sec->isin , token);
        }
        token = strtok(NULL, ",");
        i++;
    }
    if (strcmp(sec->isin, "FUT") == 0)
    {
        strcat(sec->sys_id, "FUT_");
    }
    else
    {
        strcat(sec->sys_id, "SEC_");
    }
    char str[12];
    snprintf(str, sizeof(str), "%d", ++state->currSecIDCount);
    strcat(sec->sys_id, str);
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

void
allotUnits(State *state, char *line)
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
                            state->strategies[j].investors[k].status =
                                INVESTOR_ONBOARDED;
                            printf("units are %f\n",
                                   state->strategies[j].investors[k].units);
                        }
                    }
                }
            }
            /* persist the units too.
             * NOTE(Akhil) : Assumes no same name investor in diff strategies.*/
            DBUpdateInvestorUnitsAndStatus(state->db,
                                           units,
                                           InvestorStatusStrings[INVESTOR_ONBOARDED],
                                           invName);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromRedeem(LedgerEntry *assetEntry,
                    LedgerEntry *liabEntry,
                    char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    char accountName[100] = "";
    real64 result = 1;
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcat(accountName, token); 
            strcat(accountName, "_CASH_USD"); 
            strcpy(assetEntry->accountName, accountName);
            strcpy(accountName, ""); 
        }
        else if (i == 2)
        {
            strcat(accountName, token); 
            strcat(accountName, "_CASH_USD");
            strcpy(liabEntry->accountName, accountName);
        }
        else if (i == 7)
        {
            result *= atof(token);
        }
        else if (i == 8)
        {
            result *= atof(token); /* nav units * nav rate */
            assetEntry->debit = result;
            liabEntry->credit = result;
        }
        else if (i == 13)
        {
            if (0 == strcmp(token, "CASH_USD"))
            {
                liabEntry->currency = USD;
                liabEntry->type = REVENUE;
                assetEntry->currency = USD;
                assetEntry->type = ASSET;
            }
            else
            {
                liabEntry->currency = INR;
                liabEntry->type = REVENUE;
                assetEntry->currency = INR;
                assetEntry->type = ASSET;
            }
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromCashFlow(LedgerEntry *assetEntry,
                    LedgerEntry *liabEntry,
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
            strcat(accountName, "_CASH_USD"); 
            strcpy(assetEntry->accountName, accountName);
        }
        else if (i == 3)
        {
            assetEntry->debit = (real64)atof(token);
            liabEntry->credit = (real64)atof(token);
            liabEntry->currency = USD;
            liabEntry->type = REVENUE;
            assetEntry->currency = USD;
            assetEntry->type = ASSET;
            strcpy(accountName, ""); 
        }
        else if (i == 6)
        {
            strcat(accountName, token); 
            strcat(accountName, "_"); 
        }
        else if (i == 7)
        {
            strcat(accountName, token); 
            strcpy(liabEntry->accountName, accountName);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
AccountFromReverse(LedgerEntry *assetEntry,
                   LedgerEntry *liabEntry,
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
            strcpy(assetEntry->accountName, strcat(accountName, "REV"));
            liabEntry->type = LIABILITY;
            assetEntry->type = ASSET;
        }
        else if (i == 7)
        {
            liabEntry->credit = (real64)atof(token);
            liabEntry->currency = USD;
            assetEntry->debit = (real64)atof(token);
            assetEntry->currency = USD;
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
AccountFromBank(LedgerEntry *assetEntry,
                LedgerEntry *liabEntry,
                char *line,
                Currency_code startCurr)
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
            strcpy(inv->lastPerfFeeDate, token);
        }
        else if (i ==  32)
        {
            strcpy(inv->billGroup, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
    inv->lastNav = 100;
    inv->status = INVESTOR_PENDING;
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
            char output[11];
            ConvertDateSeparator(token, output, sizeof(output));
            if (output[0] != '\0')
            {
                strcpy(trade->date, output);
            }
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
            /* for some reason here the date format is correct */
            char output[11];
            ConvertDateSeparator(token, output, sizeof(output));
            if (output[0] != '\0')
            {
                strcpy(trade->expiry, output);
            }
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
            char output[11];
            ConvertDateSeparator(token, output, sizeof(output));
            if (output[0] != '\0')
            {
                strcpy(trade->date, output);
            }
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
LoadBalance(Balance *bal, char *line)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i == 0)
        {
            strcpy(bal->symbol, token);
        }
        else if (i ==  1)
        {
            bal->balance = (real64)atof(token);
        }
        token = strtok(NULL, ",");
        i++;
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
        if (i == 2)
        {
            strcpy(pos->isin , token);
        }
        else if (i ==  4)
        {
            pos->qty = (real64)atof(token);
        }
        else if (i == 6)
        {
            pos->price = (real64)atof(token); // NOTE(Akhil): needs to be different.
            pos->ltp = (real64)atof(token);
            pos->pnl = 0.0;
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
loadStateFromDB(State *state)
{
    /* No need to init, overwrites everything on the existing state */
    PGconn *conn = state->db;
    char query[1024];
    /* Fetch the counters */
    sprintf(query,
            "SELECT * FROM global_state");

    PGresult *pgResult = executeQuery(conn, query);
    int rows = PQntuples(pgResult);
    int cols = PQnfields(pgResult);
    if (rows == 0)
    {
        fprintf(stderr, "No state found: \n");
        PQclear(pgResult);
        return;
    }
    char *secIdStr = PQgetvalue(pgResult, 0, 1);
    int secIdCount = atoi(secIdStr);
    char *optIdStr = PQgetvalue(pgResult, 0, 2);
    int optIdCount = atoi(optIdStr);
    state->currSecIDCount = secIdCount;
    state->currOptIDCount = optIdCount;
    PQclear(pgResult);
    /* Load the securities */
    sprintf(query,
            "SELECT * FROM security");

    pgResult = executeQuery(conn, query);
    rows = PQntuples(pgResult);
    cols = PQnfields(pgResult);
    if (rows == 0)
    {
        fprintf(stderr, "No securities found: \n");
        PQclear(pgResult);
    }
    else
    {
        state->currSecIndex = -1;
        for (int i = 0; i < rows; i++)
        {
            Security sec = {};
            for (int j = 0; j < cols; j++)
            {
                if (j == 1)
                {
                    char *str = PQgetvalue(pgResult, i, j);
                    strcpy(sec.sys_id, str);
                }
                else if (j == 2)
                {
                    char *str = PQgetvalue(pgResult, i, j);
                    strcpy(sec.isin , str);
                }
                else if (j == 3)
                {
                    char *str = PQgetvalue(pgResult, i, j);
                    strcpy(sec.symbol, str);
                }
                else if (j == 4)
                {
                    char *str = PQgetvalue(pgResult, i, j);
                    char output[11];
                    convert_date_format(str, output);
                    strcpy(sec.date, output);
                }
                else if (j == 5)
                {
                    char *str = PQgetvalue(pgResult, i, j);
                    strcpy(sec.name, str);
                }
            }
            state->secs[i] = sec;
            ++state->currSecIndex;
        }
        PQclear(pgResult);
    }

    /* Load the strategies */
    state->currStratIndex = -1;
    sprintf(query,
            "SELECT * FROM strategy");

    pgResult = executeQuery(conn, query);
    rows = PQntuples(pgResult);
    cols = PQnfields(pgResult);
    if (rows == 0)
    {
        fprintf(stderr, "No strategy found matching symbol: \n");
        PQclear(pgResult);
    }
    else
    {
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
                else if (j == 7)
                {
                    strat.currFPosIndex = atoi(str);
                }
                else if (j == 8)
                {
                    strat.currInvestorIndex = atoi(str);
                }
                else if (j == 9)
                {
                    strat.currEntryId = atoi(str);
                }
                else if (j == 10)
                {
                    strat.currJournalId = atoi(str);
                }
                else if (j == 11)
                {
                    strat.currAccIndex = atoi(str);
                }
                else if (j == 6)
                {
                    strat.currPosIndex = atoi(str);
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
                                else if (b == 5)
                                {
                                    inv.lastNav = atof(str);
                                }
                                else if (b == 6)
                                {
                                    strcpy(inv.inceptionDate, str);
                                }
                                else if (b == 7)
                                {
                                    strcpy(inv.lastPerfFeeDate, str);
                                }
                                else if (b == 10)
                                {
                                    char billSymbol[100];
                                    DBgetBillSymbolFromId(conn,
                                                          atoi(str),
                                                          billSymbol);
                                    strcpy(inv.billGroup, billSymbol);
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
        if (i == 1)
        {
            strcpy(div->externalId, token);
        }
        else if (i == 6)
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

void
LoadInvNameFromFile(char *line, char *invName)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcpy(invName, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

void
LoadStratSymbolFromFile(char *line, char *stratSymbol)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  1)
        {
            strcpy(stratSymbol, token);
        }
        token = strtok(NULL, ",");
        i++;
    }
}

/* Present at the 2nd column */
void
LoadStratSymbolFromFileSecond(char *line, char *stratSymbol)
{
    char *token;
    token = strtok(line, ",");
    int i = 0;
    while (token != NULL)
    {
        if (i ==  2)
        {
            strcpy(stratSymbol, token);
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

/* --------------- File Processors ------------------------------------------------*/
void
processBhavEq(FILE *bhavFile, char *date, int stratIndex, State *state)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), bhavFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        Bhav bhav = {};
        LoadBhav(&bhav, line);
        strcpy(bhav.date, date);
        /* NOTE(Akhil): The symbol may be present in multiple strats,
                        need to update the posns in all of them. */
        for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
        {
            if (strcmp(bhav.symbol,
                       state->strategies[stratIndex].positions[i].symbol) == 0)
            {
                state->strategies[stratIndex].positions[i].ltp = bhav.ltp;
                char query[512];
                snprintf(query, sizeof(query),
                         "INSERT INTO equity_bhav (symbol, ltp, date) VALUES ('%s', %f, to_date('%s', 'DD/MM/YYYY')) "
                         "ON CONFLICT (symbol) DO UPDATE SET "
                         "ltp = EXCLUDED.ltp, updated_at = CURRENT_TIMESTAMP;",
                         bhav.symbol,
                         bhav.ltp,
                         bhav.date
                         ); 
                PGresult *pgResult = executeQuery(state->db, query);
                PQclear(pgResult);

                snprintf(query, sizeof(query),
                         "UPDATE position_equity SET ltp = %f WHERE sys_id = '%s'",
                         bhav.ltp,
                         state->strategies[stratIndex].positions[i].sys_id
                         );
                pgResult = executeQuery(state->db, query);
                PQclear(pgResult);
            }
        }
        // printf("cash after bhav is %f\n", state->strategies[stratIndex].cash);
    }
}

void
processBhav(FILE *bhavFile, char *date, int dbStratId,
            int stratIndex, State *state)
{
    char line[4096];
    int i = 0;
    while (fgets(line, sizeof(line), bhavFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        FNO_bhav bhav = {};
        LoadFNOBhav(&bhav, line);
        strcpy(bhav.date, date);
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
                         "INSERT INTO fno_bhav (strategy_id, symbol, ltp, expiry, strike, opt_type, inst_type, date) "
                         "VALUES (%d, '%s', %f, to_date('%s', 'DD/MM/YYYY'), %f, '%s', '%s', to_date('%s', 'DD/MM/YYYY'));",
                         dbStratId,
                         bhav.symbol,
                         bhav.ltp,
                         bhav.expiry,
                         bhav.strike,
                         OptTypeStrings[bhav.optType],
                         InstrumentTypeStrings[bhav.instType],
                         bhav.date
                         );
                PGresult *pgResult = executeQuery(state->db, query);
                PQclear(pgResult);

                snprintf(query, sizeof(query),
                         "UPDATE fno_position SET ltp = %f WHERE sys_id = '%s'",
                         bhav.ltp,
                         state->strategies[stratIndex].fpositions[i].sys_id
                         );
                pgResult = executeQuery(state->db, query);
                PQclear(pgResult);
            }
        }
        // printf("cash after bhav is %f\n", state->strategies[stratIndex].cash);
    }
}

int
processTradesEq(FILE *tradeFile, int dbStratId, State *state)
{
    char line[1024];
    int i = 0;
    int stratIndex = -1;
    while (fgets(line, sizeof(line), tradeFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        Trade trade = {};
        LoadTrade(&trade, line);
        /* persist */
        char query[1024]; // High buffer boundary to prevent memory truncation issues
        snprintf(query, sizeof(query),
                 "INSERT INTO equity_trade (strategy_id, symbol, broker_code, trade_date, strategy_symbol, "
                 "qty, price, brokerage, service_tax, trans_type, currency) "
                 "VALUES (%d, '%s', '%s', to_date('%s', 'DD/MM/YYYY'), '%s', %d, %f, %f, %f, '%s', '%s');",
                 dbStratId,
                 trade.symbol,
                 trade.brokerCode,
                 trade.date,
                 trade.strategySymbol,
                 trade.qty,
                 trade.price,
                 trade.brokerage,
                 trade.serviceTax,
                 TransTypeStrings[trade.transType],
                 trade.currency == USD ? "USD" : "INR");
        PGresult *pgResult = executeQuery(state->db, query);
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
        for (int i = 0; i < state->strategies[stratIndex].currPosIndex + 1; i++)
        {
            if (strcmp(state->strategies[stratIndex].positions[i].isin,
                       trade.symbol) == 0)
            {

                LedgerEntry assetEntry = {};
                LedgerEntry liabEntry = {};
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

                            /* NOTE(Akhil): 2 because sbirc for equities */
                            state->strategies[stratIndex].accs[2].balance -=
                                trade.qty * priceAfterFee;
                            /* persist the accs balance. */
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[2].balance,
                                                state->strategies[stratIndex].accs[2].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[2].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
                            PQclear(pgResult);
                            if (state->strategies[stratIndex].positions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].positions[i].pnl =
                                    trade.qty * priceAfterFee +
                                    state->strategies[stratIndex].positions[i].price *
                                    state->strategies[stratIndex].positions[i].qty;
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
                            state->strategies[stratIndex].accs[2].balance -=
                                trade.qty * priceAfterFee; 
                            /* persist the accs balance. */
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[3].balance,
                                                state->strategies[stratIndex].accs[3].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
                            PQclear(pgResult);

                            if (state->strategies[stratIndex].positions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].positions[i].pnl =
                                    trade.qty * priceAfterFee +
                                    state->strategies[stratIndex].positions[i].price *
                                    state->strategies[stratIndex].positions[i].qty;
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
                /* persist the updates to price and qty. */
                snprintf(query, sizeof(query),
                         "UPDATE position_equity SET price = %f, qty = %d , pnl = %f"
                         " WHERE sys_id = '%s'",
                         state->strategies[stratIndex].positions[i].price,
                         state->strategies[stratIndex].positions[i].qty,
                         state->strategies[stratIndex].positions[i].pnl,
                         state->strategies[stratIndex].positions[i].sys_id
                         );
                pgResult = executeQuery(state->db, query);
                PQclear(pgResult);
                DBInsertLedgerEntry(state->db, &assetEntry, dbStratId);
                DBInsertLedgerEntry(state->db, &liabEntry, dbStratId);
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
                    strcpy(pos.sys_id, state->secs[i].sys_id);
                }
            }
            LedgerEntry assetEntry = {};
            LedgerEntry liabEntry = {};
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
                        state->strategies[stratIndex].accs[2].balance -=
                            trade.qty * priceAfterFee; 
                        /* persist the accs balance. */
                        DBUpdateBankBalance(state->db,
                                            state->strategies[stratIndex].accs[2].balance,
                                            state->strategies[stratIndex].accs[2].symbol);
                        snprintf(query, sizeof(query),
                                 "UPDATE strategy SET cash = %f WHERE id = %d",
                                 state->strategies[stratIndex].accs[3].balance,
                                 dbStratId
                                 );
                        pgResult = executeQuery(state->db, query);
                        PQclear(pgResult);
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        pos.pnl = 0.0;
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
                        state->strategies[stratIndex].accs[2].balance -=
                            trade.qty * priceAfterFee;
                        /* persist the accs balance. */
                        DBUpdateBankBalance(state->db,
                                            state->strategies[stratIndex].accs[2].balance,
                                            state->strategies[stratIndex].accs[2].symbol);
                        snprintf(query, sizeof(query),
                                 "UPDATE strategy SET cash = %f WHERE id = %d",
                                 state->strategies[stratIndex].accs[3].balance,
                                 dbStratId
                                 );
                        pgResult = executeQuery(state->db, query);
                        PQclear(pgResult);
                        pos.price = priceAfterFee;
                        pos.qty = trade.qty;
                        pos.pnl = 0.0;
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
            /* persist the updates to price and qty. */
            DBInsertPosition(state->db, &pos, dbStratId);
            DBInsertLedgerEntry(state->db, &assetEntry, dbStratId);
            DBInsertLedgerEntry(state->db, &liabEntry, dbStratId);
        }
        printf("cash is %f\n", state->strategies[stratIndex].accs[2].balance);
        
    }
    return stratIndex;
}

int
processTrades(FILE *tradeFile, int dbStratId, State *state)
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
        /* NOTE(Akhil): there is no trade id here to check for duplicates! */
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
        PGresult *pgResult = executeQuery(state->db, query);
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
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[3].balance,
                                                state->strategies[stratIndex].accs[3].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
                            PQclear(pgResult);
                            if (state->strategies[stratIndex].fpositions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].fpositions[i].pnl =
                                    trade.qty * priceAfterFee +
                                    state->strategies[stratIndex].fpositions[i].price *
                                    state->strategies[stratIndex].fpositions[i].qty;

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
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[3].balance,
                                                state->strategies[stratIndex].accs[3].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
                            PQclear(pgResult);
                            if (state->strategies[stratIndex].fpositions[i].qty + 
                                trade.qty == 0)
                            {
                                state->strategies[stratIndex].fpositions[i].pnl =
                                    trade.qty * priceAfterFee +
                                    state->strategies[stratIndex].fpositions[i].price *
                                    state->strategies[stratIndex].fpositions[i].qty;
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
                // persist the updates to price and qty.
                snprintf(query, sizeof(query),
                         "UPDATE fno_position SET price = %f, qty = %d, pnl = %f "
                         " WHERE sys_id = '%s'",
                         state->strategies[stratIndex].fpositions[i].price,
                         state->strategies[stratIndex].fpositions[i].qty,
                         state->strategies[stratIndex].fpositions[i].pnl,
                         state->strategies[stratIndex].fpositions[i].sys_id
                         );
                pgResult = executeQuery(state->db, query);
                PQclear(pgResult);
                DBInsertLedgerEntry(state->db, &assetEntry, dbStratId);
                DBInsertLedgerEntry(state->db, &liabEntry, dbStratId);

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
            pos.pnl = 0.0;
            sprintf(pos.sys_id, "OPT%d", ++state->currOptIDCount);
            /* Persist the opt counter */
            char query[1024];
            snprintf(query, sizeof(query),
                     "UPDATE global_state SET curr_opt_id_count = %d WHERE id = 1;",
                     state->currOptIDCount);
            PGresult *res = PQexec(state->db, query);
            PQclear(res);
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
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[3].balance,
                                                state->strategies[stratIndex].accs[3].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
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
                            DBUpdateBankBalance(state->db,
                                                state->strategies[stratIndex].accs[3].balance,
                                                state->strategies[stratIndex].accs[3].symbol);
                            snprintf(query, sizeof(query),
                                     "UPDATE strategy SET cash = %f WHERE id = %d",
                                     state->strategies[stratIndex].accs[3].balance,
                                     dbStratId
                                     );
                            pgResult = executeQuery(state->db, query);
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
                     "INSERT INTO fno_position (sys_id, strategy_id, symbol, qty, price, ltp, pnl, expiry, strike, opt_type, inst_type) "
                     "VALUES ('%s', %d, '%s', %d, %f, %f, %f, to_date('%s', 'DD/MM/YYYY'), %f, '%s', '%s');",
                     pos.sys_id,
                     dbStratId,
                     pos.symbol,
                     pos.qty,
                     pos.price,
                     pos.ltp,
                     pos.pnl,
                     pos.expiry,
                     pos.strike,
                     OptTypeStrings[pos.optType],
                     InstrumentTypeStrings[pos.instType]
                     ); 
            pgResult = executeQuery(state->db, query);
            PQclear(pgResult);
            DBInsertLedgerEntry(state->db, &assetEntry, dbStratId);
            DBInsertLedgerEntry(state->db, &liabEntry, dbStratId);
        }
        printf("cash is %f\n", state->strategies[stratIndex].accs[3].balance);
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
makeVariationSettlements(State *state, int dbStratId, int stratIndex)
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
            PGresult *pgResult = executeQuery(state->db, query);
            PQclear(pgResult);
            totalVariation += variation;
            // move the ltp now to the price column,
            // so that the next time variation is correct.
            pos.price = pos.ltp;
            sprintf(query,
                    "UPDATE fno_position SET price = %f where sys_id = '%s'",
                    pos.ltp,
                    pos.sys_id);
            pgResult = executeQuery(state->db, query);
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
            DBInsertLedgerEntry(state->db, &assetEntry, dbStratId);
            DBInsertLedgerEntry(state->db, &liabEntry, dbStratId);
        }
    }
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

    /* add the receivables now */
    totalCashUSD += state->strategies[stratIndex].receivable;
    totalCashUSD += (totalCashINR / exRate->rate);
    return totalCashUSD;
}

real64
printNav(State *state, Exchange_rate *exRate,
         int stratIndex, int dbStratId)
{
    real64 totalUnits = 0;
    for (int i = 0; i < state->strategies[stratIndex].currInvestorIndex + 1; i++)
    {
        Investor inv = state->strategies[stratIndex].investors[i];
        totalUnits += inv.units;
    }
    printf("total units are %f\n", totalUnits);
    printf("rate is %f\n", exRate->rate);

    // total value of fno positions.
    real64 totalValue = getTotalPositionValue(state, stratIndex); 
    printPositions(state, stratIndex);
    real64 cashUSD = getTotalCashUSD(state, stratIndex, exRate);
    printf("closing cash balance with receivables in usd is %f\n", cashUSD);
    real64 totalValueUSD = totalValue / exRate->rate;
    printf("total position value in usd is %f\n", totalValueUSD);
    printf("total market value in usd is %f\n", (totalValueUSD + cashUSD));
    real64 netAssets = totalValueUSD + cashUSD;
    printf("net assets are %f\n", netAssets);
    real64 fee = netAssets * (0.01 / 365); // 1% p.a
    state->strategies[stratIndex].feesAccrued += fee;
    real64 feesAccrued = state->strategies[stratIndex].feesAccrued; 
    DBUpdateFee(state->db, feesAccrued, dbStratId); 
    char query[1024];
    printf("fee accrued %f, %f\n", fee, feesAccrued);
    printf("net %f\n", (netAssets - feesAccrued));
    real64 nav = (netAssets - feesAccrued) / totalUnits;
    /* persist nav in its own seperate table. */
    snprintf(query, sizeof(query),
             "INSERT INTO strategy_nav (strategy_id, nav_date, nav) "
             "VALUES (%d, to_date('%s', 'DD/MM/YYYY'), %f) "
             "ON CONFLICT (strategy_id, nav_date) DO NOTHING;",
             dbStratId,
             exRate->date,
             nav);
    PGresult *pgResult = executeQuery(state->db, query);
    PQclear(pgResult);
    return nav;
}

void
uploadSecurities(FILE *secFile, State *state)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), secFile))
    {
        TrimString(line);
        if (line[0] == '\0') {
            continue; 
        }
        if (i == 0)
        {
            //TODO(Akhil): validate header in a way that res doesn't come here.
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Security sec = {};
        LoadSecurity(&sec, state, line);
        /* Persist the security and the counters */
        DBInsertSecurity(state->db, &sec, state->currSecIDCount);
        state->secs[++state->currSecIndex] = sec;
        // printf("security is %s\n", state->secs[i - 1].name);
    }
}

void
uploadPositions(FILE *secFile, State *state, int stratIndex, int stratId)
{
    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), secFile))
    {
        TrimString(line);
        if (line[0] == '\0') {
            continue; 
        }
        if (i == 0)
        {
            //TODO(Akhil): validate header in a way that res doesn't come here.
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        PositionEquity pos = {};
        LoadOldPosition(&pos, line);
        /* copy the sys_id to the position */
        for (int i = 0; i < state->currSecIndex + 1; i++)
        {
            if (0 == strcmp(pos.isin, state->secs[i].isin))
            {
                strcpy(pos.sys_id, state->secs[i].sys_id);
                strcpy(pos.symbol, state->secs[i].symbol);
            }
        }

        /* modify the in memory state */
        state->strategies[stratIndex]
            .positions[++state->strategies[stratIndex].currPosIndex] = pos;

        DBInsertPosition(state->db, &pos, stratId);
        // printf("security is %s\n", state->secs[i - 1].name);
    }
}

/* from the db */
int
getStratId(char *stratSymbol, PGconn *conn)
{
    char query[1024];
    sprintf(query,
            "SELECT id FROM strategy where symbol = '%s' LIMIT 1",
            stratSymbol);

    PGresult *pgResult = executeQuery(conn, query);

    if (PQntuples(pgResult) == 0)
    {
        fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
        PQclear(pgResult);
        return -1;
    }

    char *id_str = PQgetvalue(pgResult, 0, 0);
    int stratId = atoi(id_str);
    PQclear(pgResult);
    return stratId;
}

/* from the memory, not the db */
int
getStratIndex(State *state, char *stratSymbol)
{
    int stratIndex = -1;
    for (int i = 0; i < state->currStratIndex + 1; i++)
    {
        if (strcmp(stratSymbol, state->strategies[i].symbol) == 0)
        {
            stratIndex = i;
            break;
        }
    }
    printf("strat index is %d\n", stratIndex);
    return stratIndex;
}

/* --------------- Api Handlers ------------------------------------------------*/
void
handleLinkBillGroup(State *state,
                    char *invName,
                    char *stratSymbol,
                    char *billSymbol,
                    char *res)
{
    int billId = DBgetBillId(state->db, billSymbol); 

    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    char query[4096];
    snprintf(query, sizeof(query),
             "UPDATE investor SET bill_group_id = %d WHERE name = '%s' "
             "AND strategy_id = %d;",
             billId,
             invName,
             stratId); 

    PGresult *pgResult = executeQuery(state->db, query);
    PQclear(pgResult);
}

void
handleGetBillGroups(State *state,
                    char *res)
{
    char query[4096];
    snprintf(query, sizeof(query),
             "SELECT * FROM bill_group;"); 

    PGresult *pgResult = executeQuery(state->db, query);
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return;
    cJSON *billgrps = cJSON_CreateArray();
    int rows = PQntuples(pgResult);
    if (rows == 0)
    {
        printf("0 rows\n");
        strcpy(res, "no bill groups\n");
        PQclear(pgResult);
        cJSON_AddItemToObject(json, "bill-groups", billgrps);
    }

    for (int i = 0; i < rows; i++)
    {
        cJSON *bill = cJSON_CreateObject();
        cJSON_AddItemToObject(bill,
                              "id",
                              cJSON_CreateNumber(atoi(PQgetvalue(pgResult,i,0))));
        cJSON_AddItemToObject(bill,
                              "symbol",
                              cJSON_CreateString(PQgetvalue(pgResult,i,1)));
        cJSON_AddItemToObject(bill,
                              "hurdlerate",
                              cJSON_CreateNumber(atof(PQgetvalue(pgResult,i,2))));
        cJSON_AddItemToObject(bill,
                              "perfFee",
                              cJSON_CreateNumber(atof(PQgetvalue(pgResult,i,3))));
        cJSON_AddItemToObject(bill,
                              "frequency",
                              cJSON_CreateString((PQgetvalue(pgResult,i,3))));
        cJSON_AddItemToArray(billgrps, bill);
    }

    cJSON_AddItemToObject(json, "bill-groups", billgrps);
    char jsonStr[40960];
    strcpy(jsonStr, cJSON_Print(json));
    PQclear(pgResult);
    strcpy(res, jsonStr);
}

void
handleCreateBillGroup(State *state,
                      real64 hurdlerate,
                      real64 perfFee,
                      char *frequency,
                      char *billSymbol,
                      char *date,
                      char *res)
{
    char query[4096];
    snprintf(query, sizeof(query),
             "INSERT INTO bill_group (hurdlerate, perf_fee, frequency, symbol, date) "
             "VALUES (%f, '%f', '%s', '%s','%s');",
             hurdlerate,
             perfFee,
             frequency,
             billSymbol,
             date); 

    PGresult *pgResult = executeQuery(state->db, query);
    PQclear(pgResult);
    strcpy(res, "completed");
}

/* perf fee is caculated as % of profit per unit
 * for instance, inception nav = 100, now = 110,
 * and units = 1000, then 20% of (10(profit per unit) * 1000(units)) */
void
handlePerfFee(State *state,
              char *invName,
              char *stratSymbol,
              char *date,
              char *res)
{
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    real64 nav = DBgetNAV(state->db, date, stratId);
    if (nav == -1)
    {
       fprintf(stderr, "NAV nav not available for date %s\n", date);
        sprintf(res, "NAV nav not available for date %s\n", date);
        return; 
    }

    /* get the investor */
    Investor inv = {};
    strcpy(inv.name, invName);
    DBLoadInvestor(state->db, &inv);

    /* need the initial nav and fee %
     * there has to be a strp before this where we add these attrs
     * to the investor */

    Bill_group bill = {};
    DBLoadBillGroup(state->db, &bill, inv.billGroup);

    /* if the date is invalid, don't apply the fees */
    if (!isDateValid(inv.lastPerfFeeDate, date, bill.frequency))
    {
        printf("Today is not the date : %s\n", date);
        sprintf(res, "Today is not the date : %s\n", date);
        return;
    }

    real64 profitPerUnit = nav - inv.lastNav;
    if (profitPerUnit <= bill.hurdlerate)
    {
        printf("not past hurdle rate, no fee : %f\n", profitPerUnit);
        sprintf(res, "not past hurdle rate, no fee : %f\n", profitPerUnit);
        return;
    }

    real64 profit = profitPerUnit * inv.units;
    real64 fee = (bill.perfFee / 100) * profit;
    printf("fee applied is %f\n", fee);

    /* update last nav and perf fee date for investor, mem and db */
    int stratIndex = getStratIndex(state, stratSymbol);
    if(stratIndex >= 0)
    {
        for (int i = 0; i < state->strategies[stratIndex].currInvestorIndex; i++)
        {
            if (0 == strcmp(state->strategies[stratIndex].investors[i].name,
                            invName))
            {
                state->strategies[stratIndex].investors[i].lastNav = nav;
                strcpy(state->strategies[stratIndex].investors[i].lastPerfFeeDate,
                       date);
            }

        }
    }
    DBUpdatePerfFeeInvestor(state->db, invName, date, nav);
}

void
handlePosReport(State *state,
                char *stratSymbol,
                char *date,
                char *res)
{
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return;
    cJSON *cJPositions = cJSON_CreateArray();
    char query[4096];
    sprintf(query,
            "SELECT * FROM strategy_state_snapshot "
            "WHERE strategy_id = %d AND snapshot_date = TO_DATE('%s', 'DD/MM/YYYY');",
            stratId,
            date);

    PGresult *pgResult = executeQuery(state->db, query);

    int rows = PQntuples(pgResult);
    if (rows == 0)
    {
        /* not found for current date, meaning it has to be latest date
         * so give out th current positions
         * NOTE(Akhil): we can do this better.*/
        fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
        PQclear(pgResult);
        sprintf(query,
                "SELECT * FROM position_equity "
                "WHERE strategy_id = %d;",
                stratId);

        pgResult = executeQuery(state->db, query);

        /* iterate through the positions */
        for (int i = 0; i < rows; i++)
        {
            char *symbol = PQgetvalue(pgResult, i, 3);    
            int qty = atoi(PQgetvalue(pgResult, i, 4));    
            real64 ltp = atof(PQgetvalue(pgResult, i, 6));    
            real64 pnl = atof(PQgetvalue(pgResult, i, 7));    
            cJSON *position = cJSON_CreateObject();
            cJSON_AddItemToObject(position, "symbol", cJSON_CreateString(symbol));
            cJSON_AddItemToObject(position, "qty", cJSON_CreateNumber(qty));
            cJSON_AddItemToObject(position, "ltp", cJSON_CreateNumber(ltp));
            cJSON_AddItemToObject(position, "pnl", cJSON_CreateNumber(pnl));
            cJSON_AddItemToArray(cJPositions, position);
        }
        sprintf(query,
                "SELECT * FROM fno_position "
                "WHERE strategy_id = %d;",
                stratId);

        pgResult = executeQuery(state->db, query);

        rows = PQntuples(pgResult);
        if (rows == 0)
        {
            fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
            PQclear(pgResult);
        }

        /* iterate through the fno positions */
        for (int i = 0; i < rows; i++)
        {
            char *symbol = PQgetvalue(pgResult, i, 2);    
            int qty = atoi(PQgetvalue(pgResult, i, 3));    
            real64 ltp = atof(PQgetvalue(pgResult, i, 5));    
            real64 pnl = atof(PQgetvalue(pgResult, i, 6));    
            cJSON *position = cJSON_CreateObject();
            cJSON_AddItemToObject(position, "symbol", cJSON_CreateString(symbol));
            cJSON_AddItemToObject(position, "qty", cJSON_CreateNumber(qty));
            cJSON_AddItemToObject(position, "ltp", cJSON_CreateNumber(ltp));
            cJSON_AddItemToObject(position, "pnl", cJSON_CreateNumber(pnl));
            cJSON_AddItemToArray(cJPositions, position);
        }

        cJSON_AddItemToObject(json, "positions", cJPositions);
        char *jsonStr = cJSON_Print(json);
        strcpy(res, jsonStr);
    }
    else
    {
        char *positions = PQgetvalue(pgResult, 0, 4);    
        char *fpositions = PQgetvalue(pgResult, 0, 5);
        cJSON_AddItemToObject(json, "positions", cJSON_CreateString(positions));
        cJSON_AddItemToObject(json, "fno_positions", cJSON_CreateString(fpositions));
        char *jsonStr = cJSON_Print(json);
        strcpy(res, jsonStr);
        PQclear(pgResult);
    }
}

void
handleNAVReport(State *state,
                char *stratSymbol,
                char *fromDate,
                char *toDate,
                char *res)
{
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) return;
    /* loop for all the dates starting from fromDate till toDate */
    while (0 != strcmp(toDate, ""))
    {
        cJSON *report = cJSON_CreateObject();
        if (report == NULL) return;
        if (0 == strcmp(fromDate, toDate))
        {
            strcpy(toDate, ""); // ensures the exit condition.
        }
        real64 nav = DBgetNAV(state->db, fromDate, stratId);
        if (nav == -1)
        {
            fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
            nav = 0;
        }
        char query[4096];
        printf("nav is %f\n", nav);
        cJSON *cjsonNAV= cJSON_CreateNumber(nav);
        if (cjsonNAV == NULL) return;
        cJSON_AddItemToObject(report, "nav", cjsonNAV);

        /* iterate through the positions and get the 
         * gains for the day both realized and unrealized (curr and price) */
        sprintf(query,
                "SELECT * FROM position_equity "
                "WHERE strategy_id = %d;",
                stratId);

        PGresult *pgResult = executeQuery(state->db, query);

        int rows = PQntuples(pgResult);
        if (rows == 0)
        {
            fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
            PQclear(pgResult);
        }

        real64 totalPriceGain = 0;
        real64 totalCurrencyGain = 0;
        real64 totalUPriceGain = 0;
        real64 totalUCurrencyGain = 0;
        for (int i = 0; i < rows; i++)
        {
            /* fetch isin, qty, price, ltp */
            char symbol[100];
            int qty;
            real64 ltp;
            real64 pnl;
            strcpy(symbol, PQgetvalue(pgResult, i, 3));
            qty = atoi(PQgetvalue(pgResult, i, 4));
            ltp = atof(PQgetvalue(pgResult, i, 6));
            pnl = atof(PQgetvalue(pgResult, i, 7));
            /* get current date exchange rate */
            real64 currExRate = DBGetExchangeRate(state->db, fromDate, stratId);
            char prevDate[100];
            decrementDate(fromDate, prevDate);
            /* TODO(Akhil): what if there is no prev exchange rate? */
            real64 prevExRate = DBGetExchangeRate(state->db, prevDate, stratId);
            real64 prevLtp = DBGetLtpEQ(state->db, symbol, prevDate, stratId);
            real64 totalGain;
            real64 priceGain;
            real64 currencyGain;
            // get the total gain in usd.
            // then keep curr same and get price gain.
            // currency gain = total gain - price gain.
            if (prevLtp < 0) continue;
            if (pnl != 0)
            {
                totalGain = pnl / currExRate; 
                priceGain = pnl / prevExRate;
                currencyGain = totalGain - priceGain;
                totalPriceGain += priceGain;
                totalCurrencyGain += currencyGain;
            }
            else
            {
                totalGain = ((qty * ltp)/ currExRate) - ((qty * prevLtp)/ prevExRate);
                priceGain = ((qty * ltp)/ prevExRate) - ((qty * prevLtp)/ prevExRate);
                currencyGain = totalGain - priceGain;
                totalUPriceGain += priceGain;
                totalUCurrencyGain += currencyGain;
            }
        }

        /* iterate through the fno positions now, doing the same thing */
        sprintf(query,
                "SELECT * FROM fno_position "
                "WHERE strategy_id = %d;",
                stratId);

        pgResult = executeQuery(state->db, query);

        rows = PQntuples(pgResult);
        if (rows == 0)
        {
            fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
            PQclear(pgResult);
            return;
        }
        printf("computing for fno...\n");
        for (int i = 0; i < rows; i++)
        {
            /* fetch isin, qty, price, ltp */
            char symbol[100];
            int qty;
            real64 ltp;
            real64 pnl;
            real64 strike;
            Opt_type optType;
            char expiry[100];
            strcpy(symbol, PQgetvalue(pgResult, i, 2));
            qty = atoi(PQgetvalue(pgResult, i, 3));
            ltp = atof(PQgetvalue(pgResult, i, 5));
            pnl = atof(PQgetvalue(pgResult, i, 6));
            char outputExpiry[11];
            ConvertDbDateToCFormat(PQgetvalue(pgResult, i, 7), outputExpiry, sizeof(outputExpiry));
            strcpy(expiry, outputExpiry);
            strike = atof(PQgetvalue(pgResult, i, 8));
            char *type = (PQgetvalue(pgResult, i, 9));
            if (0 == strcmp(type, "CE"))
            {
                optType = CE;
            }
            else if (0 == strcmp(type, "PE"))
            {
                optType = PE;
            }
            else
            {
                optType = NA;
            }
            /* get current date exchange rate */
            real64 currExRate = DBGetExchangeRate(state->db, fromDate, stratId);
            char prevDate[100];
            decrementDate(fromDate, prevDate);
            /* TODO(Akhil): what if there is no prev exchange rate? */
            real64 prevExRate = DBGetExchangeRate(state->db, prevDate, stratId);
            real64 prevLtp = DBGetLtpFNO(state->db, symbol, optType, strike, expiry, prevDate, stratId);
            printf("prevs are %f , %f\n", prevExRate, prevLtp);
            real64 totalGain;
            real64 priceGain;
            real64 currencyGain;
            // get the total gain in usd.
            // then keep curr same and get price gain.
            // currency gain = total gain - price gain.
            if (prevLtp < 0) continue;
            printf("computing....\n");
            if (pnl != 0)
            {
                totalGain = pnl / currExRate; 
                priceGain = pnl / prevExRate;
                currencyGain = totalGain - priceGain;
                totalPriceGain += priceGain;
                totalCurrencyGain += currencyGain;
            }
            else
            {
                totalGain = ((qty * ltp)/ currExRate) - ((qty * prevLtp)/ prevExRate);
                priceGain = ((qty * ltp)/ prevExRate) - ((qty * prevLtp)/ prevExRate);
                currencyGain = totalGain - priceGain;
                totalUPriceGain += priceGain;
                totalUCurrencyGain += currencyGain;
            }
        }

        printf("price gain is %f\n", totalPriceGain);
        printf("price ugain is %f\n", totalUPriceGain);
        printf("currency gain is %f\n", totalCurrencyGain);
        printf("currency ugain is %f\n", totalUCurrencyGain);

        cJSON *cjsonTPriceGain = cJSON_CreateNumber(totalPriceGain);
        if (cjsonTPriceGain == NULL) return;
        cJSON_AddItemToObject(report, "total_price_gain", cjsonTPriceGain);
        cJSON *cjsonTUPriceGain = cJSON_CreateNumber(totalUPriceGain);
        if (cjsonTUPriceGain == NULL) return;
        cJSON_AddItemToObject(report, "total_unrealized_price_gain", cjsonTUPriceGain);
        cJSON *cjsonTCurrGain = cJSON_CreateNumber(totalCurrencyGain);
        if (cjsonTCurrGain == NULL) return;
        cJSON_AddItemToObject(report, "total_currency_gain", cjsonTCurrGain);
        cJSON *cjsonTUCurrGain = cJSON_CreateNumber(totalUCurrencyGain);
        if (cjsonTUCurrGain == NULL) return;
        cJSON_AddItemToObject(report, "total_unrealized_currency_gain", cjsonTUCurrGain);
        cJSON_AddItemToObject(json, fromDate, report);

        /* move the date forward by 1 day */
        incrementDate(fromDate, fromDate);
    }
   
    char *jsonStr = cJSON_Print(json);
    // put the values in the json array.
    strcpy(res, jsonStr);
    free(jsonStr);
}

/* Platform switch work 
 * balances as of a date, includes banks as well as
 * fees, receivables etc */
void
handleBalances(State *state, char *stratSymbol, char *res)
{
    FILE *upFile = fopen("tmp.csv", "r");
    if (upFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    int stratIndex = getStratIndex(state, stratSymbol);
    /* get strat id from db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    char line[1024];
    int i = 0;
    while (fgets(line, sizeof(line), upFile))
    {
        TrimString(line);
        if (line[0] == '\0') {
            continue; 
        }
        if (i == 0)
        {
            //TODO(Akhil): validate header in a way that res doesn't come here.
            i++;
            continue; // ignore the top heading row.
        }
        char *tmp = strchr(line, '\n');
        if (tmp) *tmp = '\0';
        Balance bal = {};
        LoadBalance(&bal, line);
        if(0 == strcmp(bal.symbol, "feesAccrued"))
        {
            /* modify the in memory state */
            state->strategies[stratIndex].feesAccrued = bal.balance;

            DBUpdateFee(state->db, bal.balance, stratId); 
        }
        else if(0 == strcmp(bal.symbol, "receivable"))
        {
            /* modify the in memory state */
            state->strategies[stratIndex].receivable = bal.balance;

            DBUpdateRecv(state->db, bal.balance, stratId); 
        }
        else /* else its a bank account symbol */
        {
            for (int i = 0;
                 state->strategies[stratIndex].currAccIndex + 1;
                 i++)
            {
                if (0 == strcmp(bal.symbol,
                                state->strategies[stratIndex].accs[i].symbol))
                {
                    /* modify the in memory state */
                    state->strategies[stratIndex].accs[i].balance = bal.balance;

                    DBUpdateBankBalance(state->db, bal.balance, bal.symbol); 
                    break;
                }
            }
        }
    }
}

/* upload old positions into the db!
 * Usually done when importing a new strategy from
 * another platform */
void
handleUploadPositions(State *state, char *stratSymbol, char *res)
{
    FILE *upFile = fopen("tmp.csv", "r");
    if (upFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    /* get strat id from db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    int stratIndex = getStratIndex(state, stratSymbol);

    uploadPositions(upFile, state, stratIndex, stratId);
    strcpy(res, "completed");
}

void
handleUploadSecurities(State *state, char *res)
{
    FILE *upFile = fopen("tmp.csv", "r");
    if (upFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    uploadSecurities(upFile, state);
    strcpy(res, "completed");
}

/* state changes on the bank account balance.
 * Finally marks the completion of offboarding of a client.*/
void
handleOffBank(State *state, char *invName, char *res)
{
    char line[4096];
    FILE *bankFile = fopen("tmp.csv", "r");
    
    if (bankFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    int i = 0;
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *bankFileCopy = fopen("tmp.csv", "r");
    if (bankFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), bankFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, bankHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), bankFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {

            i++;
            continue; // ignore the top heading row.
        }
        ++state->strategies[state->currStratIndex].currJournalId;

        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
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

        PGresult *pgResult = executeQuery(state->db, query);
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
            DBInsertBankAcc(state->db, &acc, stratId);
            
            // insert in memory as well.
            state->strategies[state->currStratIndex]
                .accs[++state->strategies[state->currStratIndex].currAccIndex] = acc;
        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state->strategies[state->currStratIndex].currAccIndex;
            i++)
            {
                if(strcmp(state->strategies[state->currStratIndex].accs[i].symbol,
                          liabEntry.accountName) == 0)   
                {
                    printf("bal before %f\n", 
                           state->strategies[state->currStratIndex].accs[i].balance);
                    state->strategies[state->currStratIndex].accs[i].balance -=
                        liabEntry.credit;
                    printf("bal after %f\n", 
                           state->strategies[state->currStratIndex].accs[i].balance);

                    // db.
                    DBUpdateBankBalance(state->db,
                                        state->strategies[state->currStratIndex].accs[i].balance,
                                        liabEntry.accountName);
                }
            }
            PQclear(pgResult);
        }
        // insert or update the assetEntry bank acc.
        printf("asset entry accoutnn name is %s\n", assetEntry.accountName);
        sprintf(query,
                "SELECT * FROM bank_account where symbol = '%s'",
                assetEntry.accountName);

        pgResult = executeQuery(state->db, query);
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
            DBInsertBankAcc(state->db, &acc, stratId);

            // insert in memory as well.
            state->strategies[state->currStratIndex]
                .accs[++state->strategies[state->currStratIndex].currAccIndex] = acc;
        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state->strategies[state->currStratIndex].currAccIndex;
            i++)
            {
                if(strcmp(state->strategies[state->currStratIndex].accs[i].symbol,
                          assetEntry.accountName) == 0)   
                {
                    state->strategies[state->currStratIndex].accs[i].balance +=
                        assetEntry.debit;
                    // db.
                    DBUpdateBankBalance(state->db,
                                        state->strategies[state->currStratIndex].accs[i].balance,
                                        assetEntry.accountName);
                }
            }
            PQclear(pgResult);
        }
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }

    /* update the investor in memory and db */
    for (int j = 0; j < state->currStratIndex + 1; j++)
    {
        if (strcmp(stratSymbol, state->strategies[j].symbol) == 0)
        {
            // find the investor and allot units.
            for (int k = 0;
            k < state->strategies[j].currInvestorIndex + 1;
            k++)
            {
                if (strcmp(invName, state->strategies[j].investors[k].name)
                    == 0)
                {
                    state->strategies[j].investors[k].units = 0;
                    state->strategies[j].investors[k].status =
                        INVESTOR_OFFBOARDED;
                    printf("units are %f\n",
                           state->strategies[j].investors[k].units);
                }
            }
        }
    }
    // persist the units too.
    DBUpdateInvestorUnitsAndStatus(state->db,
                                   0.0,
                                   InvestorStatusStrings[INVESTOR_OFFBOARDED],
                                   invName);
    strcpy(res, "completed"); 
}

/* only for accounting, no state changes */
void
handleOffRedeem(State *state, char *res)
{
    char line[1024];
    int i = 0;
    FILE *redeemFile = fopen("tmp.csv", "r");
    if (redeemFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *redeemFileCopy = fopen("tmp.csv", "r");
    if (redeemFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[1024];
    while (fgets(copyLine, sizeof(copyLine), redeemFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, offredeemHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFileSecond(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), redeemFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        ++state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
        AccountFromRedeem(&assetEntry, &liabEntry, line);
        DBInsertLedgerEntry(state->db, &assetEntry, stratId);
        
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        printf("off cashflow entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        DBInsertLedgerEntry(state->db, &liabEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("off cashflow entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }
    strcpy(res, "completed");
}

/* only for accounting, no state changes */
void
handleOffCashFlow(State *state, char *res)
{
    char line[1024];
    int i = 0;
    FILE *cashflowFile = fopen("tmp.csv", "r");
    if (cashflowFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *cashflowFileCopy = fopen("tmp.csv", "r");
    if (cashflowFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[1024];
    while (fgets(copyLine, sizeof(copyLine), cashflowFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, offcashflowHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), cashflowFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        /* NOTE(Akhil): here we are working on the latest strategy.
                        usually first column discloses the strategy name. */
        ++state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
        AccountFromCashFlow(&assetEntry, &liabEntry, line);
        DBInsertLedgerEntry(state->db, &assetEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        printf("off cashflow entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        DBInsertLedgerEntry(state->db, &liabEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("off cashflow entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }
    strcpy(res, "completed");
}

/* Part of the clawback process, this step restores the previoud day's
   strategy state --from the stored snapshot-- in the db */
void
handleClawNAV(State *state,
              char *stratSymbol,
              char *rollbackDate,
              char *snapshotDate,
              char *res)
{
    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }
    char query[8192];

    snprintf(query, sizeof(query),
             "BEGIN;"

             // -- 1. Wipe the corrupted records and summaries for this rollback date
             "DELETE FROM strategy_nav WHERE strategy_id = %d AND nav_date = '%s';"
             "DELETE FROM fno_trade WHERE strategy_id = %d AND trade_date = '%s';"
             "DELETE FROM equity_trade WHERE strategy_id = %d AND trade_date = '%s';"

             "DELETE FROM position_equity WHERE strategy_id = %d;"
             "DELETE FROM fno_position WHERE strategy_id = %d;"
             "DELETE FROM bank_account WHERE strategy_id = %d;"

             // -- 2. Restore Equity Positions
             "INSERT INTO position_equity (sys_id, strategy_id, isin, symbol, qty, price, ltp) "
             "SELECT elem->>'sys_id', %d, elem->>'isin', elem->>'symbol', "
             "(elem->>'qty')::INTEGER, (elem->>'price')::DOUBLE PRECISION, (elem->>'ltp')::DOUBLE PRECISION "
             "FROM strategy_state_snapshot, jsonb_array_elements(equity_positions) AS elem "
             "WHERE strategy_id = %d AND snapshot_date = '%s';"

             // -- 3. Restore FNO Positions
             "INSERT INTO fno_position (sys_id, strategy_id, symbol, qty, price, ltp, expiry, strike, opt_type, inst_type) "
             "SELECT elem->>'sys_id', %d, elem->>'symbol', (elem->>'qty')::INTEGER, "
             "(elem->>'price')::DOUBLE PRECISION, (elem->>'ltp')::DOUBLE PRECISION, "
             "to_date(elem->>'expiry', 'DD/MM/YYYY'), (elem->>'strike')::DOUBLE PRECISION, "
             "(elem->>'optType')::opt_type, (elem->>'instType')::instrument_type "
             "FROM strategy_state_snapshot, jsonb_array_elements(fno_positions) AS elem "
             "WHERE strategy_id = %d AND snapshot_date = '%s';"

             // -- 4. Restore Bank Account Balances
             "INSERT INTO bank_account (strategy_id, symbol, balance, currency) "
             "SELECT %d, elem->>'symbol', (elem->>'balance')::DOUBLE PRECISION, elem->>'currency_code' "
             "FROM strategy_state_snapshot, jsonb_array_elements(banks) AS elem "
             "WHERE strategy_id = %d AND snapshot_date = '%s';"

             // -- 5. Re-sync basic strategy metadata parameters and volatile counter tracking indexes
             "UPDATE strategy SET "
             "cash = (SELECT (meta_state->>'cash')::DOUBLE PRECISION FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "nav = (SELECT (meta_state->>'nav')::DOUBLE PRECISION FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_pos_index = (SELECT (meta_state->>'currPosIndex')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_f_pos_index = (SELECT (meta_state->>'currFPosIndex')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_investor_index = (SELECT (meta_state->>'currInvestorIndex')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_acc_index = (SELECT (meta_state->>'currAccIndex')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_entry_id = (SELECT (meta_state->>'currEntryId')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "curr_journal_id = (SELECT (meta_state->>'currJournalId')::INTEGER FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s'), "
             "fees_accrued = (SELECT (meta_state->>'feesAccrued')::DOUBLE PRECISION FROM strategy_state_snapshot WHERE strategy_id = %d AND snapshot_date = '%s') "
             "WHERE id = %d;"

             "COMMIT;",

             stratId, rollbackDate,                          // delete strategy_nav
             stratId, rollbackDate,                          // delete fno_trade
             stratId, rollbackDate,                          // delete equity_trade
             stratId,                                        // delete position_equity
             stratId,                                        // delete fno_position
             stratId,                                        // delete bank_account

             stratId, stratId, snapshotDate,                 // insert position_equity
             stratId, stratId, snapshotDate,                 // insert fno_position
             stratId, stratId, snapshotDate,                 // insert bank_account

             stratId, snapshotDate,                          // update strategy cash
             stratId, snapshotDate,                          // update strategy nav
             stratId, snapshotDate,                          // update strategy curr_pos_index
             stratId, snapshotDate,                          // update strategy curr_f_pos_index
             stratId, snapshotDate,                          // update strategy curr_investor_index
             stratId, snapshotDate,                          // update strategy curr_acc_index
             stratId, snapshotDate,                          // update strategy curr_entry_id
             stratId, snapshotDate,                          // update strategy curr_journal_id
             stratId, snapshotDate,                          // update strategy feesAccrued 
             stratId                                         // update strategy target identification key
             );


    // Execute everything in a single round-trip over the network connection wire
    PGresult *pgResult = PQexec(state->db, query);

    // Verify the command execution completed successfully
    if (PQresultStatus(pgResult) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "[ERROR] Rollback transaction failed: %s\n",
                PQerrorMessage(state->db));
    }
    else
    {
        printf("[SUCCESS] Strategy %d safely restored to state snapshot dated %s.\n",
               stratId, snapshotDate);
    }

    PQclear(pgResult);

    /* load the state into the memory from the db since it was changed */
    loadStateFromDB(state);
}

/* the snapshot we store here is used when we want to clawback
 * and recompute a nav, we retrieve the snapshot, apply it to
 * memory and db, and then do the nav process again for that date */
void
saveDailySnapshot(PGconn *conn,
                  int stratId,
                  char *date,
                  Strategy *strat)
{
    printf("Saving strategy snapshot....\n");
    /* 1. Serialize primitive tracker parameters */
    cJSON *meta = cJSON_CreateObject();
    cJSON_AddItemToObject(meta, "cash", cJSON_CreateNumber(strat->cash));
    cJSON_AddItemToObject(meta, "nav", cJSON_CreateNumber(strat->nav));
    cJSON_AddItemToObject(meta, "currPosIndex", cJSON_CreateNumber(strat->currPosIndex));
    cJSON_AddItemToObject(meta, "currFPosIndex", cJSON_CreateNumber(strat->currFPosIndex));
    cJSON_AddItemToObject(meta, "currAccIndex", cJSON_CreateNumber(strat->currAccIndex));
    cJSON_AddItemToObject(meta, "currInvestorIndex", cJSON_CreateNumber(strat->currInvestorIndex));
    cJSON_AddItemToObject(meta, "currJournalId", cJSON_CreateNumber(strat->currJournalId));
    cJSON_AddItemToObject(meta, "currEntryId", cJSON_CreateNumber(strat->currEntryId));
    cJSON_AddItemToObject(meta, "feesAccrued", cJSON_CreateNumber(strat->feesAccrued));
    cJSON_AddItemToObject(meta, "symbol", cJSON_CreateString(strat->symbol));

    /* 2. Serialize structural arrays up to active boundaries */
    cJSON *eq_pos = cJSON_CreateArray();
    for (int i = 0; i <= strat->currPosIndex; i++) {
        cJSON *position = cJSON_CreateObject();
        cJSON_AddItemToObject(position,
                              "symbol",
                              cJSON_CreateString(strat->positions[i].symbol));
        cJSON_AddItemToObject(position,
                              "qty",
                              cJSON_CreateNumber(strat->positions[i].qty));
        cJSON_AddItemToObject(position,
                              "ltp",
                              cJSON_CreateNumber(strat->positions[i].ltp));
        cJSON_AddItemToObject(position,
                              "pnl",
                              cJSON_CreateNumber(strat->positions[i].pnl));
        cJSON_AddItemToObject(position,
                              "price",
                              cJSON_CreateNumber(strat->positions[i].price));
        cJSON_AddItemToObject(position,
                              "isin",
                              cJSON_CreateString(strat->positions[i].isin));
        cJSON_AddItemToObject(position,
                              "sys_id",
                              cJSON_CreateString(strat->positions[i].sys_id));

        cJSON_AddItemToArray(eq_pos, position);
    }

    cJSON *fno_pos = cJSON_CreateArray();
    for (int i = 0; i <= strat->currFPosIndex; i++) {
        cJSON *position = cJSON_CreateObject();
        cJSON_AddItemToObject(position,
                              "symbol",
                              cJSON_CreateString(strat->fpositions[i].symbol));
        cJSON_AddItemToObject(position,
                              "expiry",
                              cJSON_CreateString(strat->fpositions[i].expiry));
        cJSON_AddItemToObject(position,
                              "qty",
                              cJSON_CreateNumber(strat->fpositions[i].qty));
        cJSON_AddItemToObject(position,
                              "strike",
                              cJSON_CreateNumber(strat->fpositions[i].strike));
        cJSON_AddItemToObject(position,
                              "ltp",
                              cJSON_CreateNumber(strat->fpositions[i].ltp));
        cJSON_AddItemToObject(position,
                              "pnl",
                              cJSON_CreateNumber(strat->fpositions[i].pnl));
        cJSON_AddItemToObject(position,
                              "price",
                              cJSON_CreateNumber(strat->fpositions[i].price));
        cJSON_AddItemToObject(position,
                              "optType",
                              cJSON_CreateString(OptTypeStrings[strat->fpositions[i].optType]));
        cJSON_AddItemToObject(position,
                              "instType",
                              cJSON_CreateString(InstrumentTypeStrings[strat->fpositions[i].instType]));

        cJSON_AddItemToObject(position,
                              "sys_id",
                              cJSON_CreateString(strat->positions[i].sys_id));

        cJSON_AddItemToArray(fno_pos, position);
    }

    cJSON *bankAccs = cJSON_CreateArray();
    for (int i = 0; i <= strat->currAccIndex; i++) {
        cJSON *acc = cJSON_CreateObject();
        cJSON_AddItemToObject(acc,
                              "symbol",
                              cJSON_CreateString(strat->accs[i].symbol));
        cJSON_AddItemToObject(acc,
                              "balance",
                              cJSON_CreateNumber(strat->accs[i].balance));
        cJSON_AddItemToObject(acc,
                              "currency_code",
                              cJSON_CreateString(CurrencyCodeStrings[strat->accs[i].currency]));
        cJSON_AddItemToArray(bankAccs, acc);
    }

    cJSON *investors = cJSON_CreateArray();
    for (int i = 0; i <= strat->currInvestorIndex; i++) {
        cJSON *inv = cJSON_CreateObject();
        cJSON_AddItemToObject(inv,
                              "name",
                              cJSON_CreateString(strat->investors[i].name));
        cJSON_AddItemToObject(inv,
                              "inceptionDate",
                              cJSON_CreateString(strat->investors[i].inceptionDate));
        cJSON_AddItemToObject(inv,
                              "lastPerfFeeDate",
                              cJSON_CreateString(strat->investors[i].lastPerfFeeDate));
        cJSON_AddItemToObject(inv,
                              "billGroup",
                              cJSON_CreateString(strat->investors[i].billGroup));
        cJSON_AddItemToObject(inv,
                              "units",
                              cJSON_CreateNumber(strat->investors[i].units));
        cJSON_AddItemToObject(inv,
                              "lastNav",
                              cJSON_CreateNumber(strat->investors[i].lastNav));
        cJSON_AddItemToObject(inv,
                              "status",
                              cJSON_CreateString(InvestorStatusStrings
                                                 [strat->investors[i].status]));
        cJSON_AddItemToArray(investors, inv);
    }

    /* 3. Convert JSON objects to text strings */
    char *meta_str = cJSON_Print(meta);
    char *eq_str = cJSON_Print(eq_pos);
    char *fno_str = cJSON_Print(fno_pos);
    char *acc_str = cJSON_Print(bankAccs);
    char *inv_str = cJSON_Print(investors);

    /* 4. Save to PostgreSQL using an UPSERT pattern */
    char query[8192 * 3];
    snprintf(query, sizeof(query),
             "INSERT INTO strategy_state_snapshot (strategy_id, snapshot_date, meta_state, equity_positions, fno_positions, banks, investors) "
             "VALUES (%d, to_date('%s', 'DD/MM/YYYY'), '%s', '%s', '%s', '%s', '%s') "
             "ON CONFLICT (strategy_id, snapshot_date) DO UPDATE SET "
             "meta_state = EXCLUDED.meta_state, equity_positions = EXCLUDED.equity_positions, fno_positions = EXCLUDED.fno_positions, banks = EXCLUDED.banks;",
             stratId, date, meta_str, eq_str, fno_str, acc_str, inv_str);

    PGresult *res = PQexec(conn, query);
    char *error = PQresultErrorMessage(res);
    if (strcmp(error, "") != 0)
    {
        printf("%s", error);
    }
    PQclear(res);
}

void
handleNAV(State *state, char *stratSymbol, char *date, char *res)
{
    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    /* get the stratIndex from the memory */
    int stratIndex = getStratIndex(state, stratSymbol);

    /* check if the nav is already published for the date */
    real64 nav = DBgetNAV(state->db, date, stratId);
    if (nav != -1)
    {
       fprintf(stderr, "NAV already published for date: %s\n", date);
        sprintf(res, "NAV already published for date: %s\n", date);
        return; 
    }

    /* handle exchange_rate for the date from the db */
    real64 rate = DBGetExchangeRate(state->db, date, stratId);
    
    if(rate == -1)
    {
        fprintf(stderr, "No exchange_rate found matching symbol: %s\n", stratSymbol);
        sprintf(res, "No exchange_rate found matching symbol: %s\n", stratSymbol);
        return;
    }
    Exchange_rate exRate = {};
    exRate.rate = rate;
    strcpy(exRate.date, date);
    nav = printNav(state, &exRate, stratIndex, stratId);
    sprintf(res, "nav is %f", nav);
    
    /* save the state snapshot for the date */
    saveDailySnapshot(state->db,
                      stratId,
                      date,
                      &(state->strategies[stratIndex]));
}

void
handleCorpAction(State *state,
                 char *stratSymbol,
                 char *date,
                 char *res)
{
    /* process dividend file, just the code for now */
    char line[4096];
    FILE *DivFile = fopen("tmp.csv", "r");
    if (DivFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    /* get the stratIndex from the memory */
    int stratIndex = getStratIndex(state, stratSymbol);

    int i = 0;
    while (fgets(line, sizeof(line), DivFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        Dividend div = {};
        LoadDividend(&div, line);
        /* persist the corp action entry */
        char query[4096];

        snprintf(query, sizeof(query),
                 "INSERT INTO corporate_action (strategy_id, external_id, isin, ex_date, payment_date, action_type, dividend_value, status) "
                 "VALUES (%d, '%s', '%s', to_date('%s', 'DD/MM/YYYY'), to_date('%s', 'DD/MM/YYYY'), '%s', %f, '%s') "
                 "ON CONFLICT (external_id) DO UPDATE SET "
                 "status = EXCLUDED.status, "
                 "payment_date = EXCLUDED.payment_date, "
                 "dividend_value = EXCLUDED.dividend_value, "
                 "updated_at = CURRENT_TIMESTAMP;",
                 stratId,
                 div.externalId,
                 div.isin,
                 div.exDate,      
                 div.paymentDate,
                 div.type,
                 div.div,
                 DivStatusStrings[div.status]
                 );

        PGresult *res = PQexec(state->db, query);
        PQclear(res);
        /* only process if the ex date is today. */
        if (strcmp(div.exDate, date) == 0)
        {
            printf("date matched\n");
            for (int j = 0;
                 j <= state->strategies[stratIndex].currPosIndex;
                 j++)
            {
                PositionEquity pos = state->strategies[stratIndex].positions[j];
                printf("processing pos %s...\n", pos.isin);
                if (strcmp(pos.isin, div.isin) == 0)
                {
                    printf("processing dividend %s...\n", div.isin);
                    // make the dividend income.
                    // NOTE(Akhil): 2 stands for the equites bank acc.
                    real64 rate = DBGetExchangeRate(state->db, date, stratId);
                    if (rate == -1)
                    {
                        fprintf(stderr, "No exchange_rate found matching symbol: %s\n", stratSymbol);
                        return; /* TODO(Akhil) : handle error */
                    }
                    
                    real64 receivableUSD = (div.div * pos.qty) / rate; 
                    state->strategies[stratIndex].receivable += receivableUSD;
                    DBUpdateRecv(state->db,
                                 state->strategies[stratIndex].receivable,
                                 stratId); 
                    div.status = DONE;

                    /* mark the corporate actioin as processed */
                    snprintf(query, sizeof(query),
                             "UPDATE corporate_action SET div_status = '%s' "
                             "WHERE external_id = '%s' AND strategy_id = %d;",
                             DivStatusStrings[div.status],
                             div.externalId,
                             stratId
                             );

                    PGresult *res = PQexec(state->db, query);
                    PQclear(res);

                    /* make the ledger entries and persist them as well. */
                    LedgerEntry assetEntry = {};
                    LedgerEntry liabEntry = {};
                    ++state->strategies[state->currStratIndex].currJournalId;
                    strcat(assetEntry.accountName, div.isin);
                    strcat(assetEntry.accountName, "_DIV");
                    assetEntry.type = ASSET;
                    assetEntry.currency = INR;
                    assetEntry.debit = abs(pos.qty * div.div);
                    assetEntry.id = state->strategies[state->currStratIndex].
                        currJournalId;
                    strcpy(liabEntry.accountName, "DIV_CASH_USD");
                    liabEntry.credit = abs(pos.qty * div.div);
                    liabEntry.type = REVENUE;
                    liabEntry.id = state->strategies[state->currStratIndex].
                        currJournalId;
                    liabEntry.currency = INR;
                    state->strategies[state->currStratIndex].
                        ledger[++state->strategies[state->currStratIndex].
                        currEntryId] = assetEntry;
                    state->strategies[state->currStratIndex].
                        ledger[++state->strategies[state->currStratIndex].
                        currEntryId] = liabEntry;
                    DBInsertLedgerEntry(state->db, &assetEntry, stratId);
                    DBInsertLedgerEntry(state->db, &liabEntry, stratId);
                }
            }
        }

        i++;
    }
    sprintf(res, "completed");
}

void
handleMTM(State *state, char *stratSymbol, char *res)
{
    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    /* get the stratIndex from the memory */
    int stratIndex = getStratIndex(state, stratSymbol);

    makeVariationSettlements(state, stratId, stratIndex);
    strcpy(res, "completed");
}

void
handleBhavEq(State *state, char *date, char *stratSymbol, char *res)
{
    FILE *FBhavFile = fopen("tmp.csv", "r");
    if (FBhavFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    /* get the stratIndex from the memory */
    int stratIndex = getStratIndex(state, stratSymbol);

    processBhavEq(FBhavFile, date, stratIndex, state);
    strcpy(res, "completed");
}

void
handleBhavFNO(State *state, char *date, char *stratSymbol, char *res)
{
    FILE *FBhavFile = fopen("tmp.csv", "r");
    if (FBhavFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    /* get the stratIndex from the memory */
    int stratIndex = getStratIndex(state, stratSymbol);
    printf("strat index is %d\n", stratIndex);

    /* TODO(Akhil) : format error for bhav file */
    processBhav(FBhavFile, date, stratId, stratIndex, state);
    strcpy(res, "completed");
}

void
handleTradesEq(State *state, char *res)
{
    FILE *FTradesFile = fopen("tmp.csv", "r");
    if (FTradesFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    int i = 0;
    char stratSymbol[100];
    FILE *FTradesFileCopy = fopen("tmp.csv", "r");
    if (FTradesFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), FTradesFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, fnoTradesHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    int result = processTradesEq(FTradesFile, stratId, state);
    if (result < 0)
    {

        strcpy(res, "couldn't find strategy");
    }
    else
    {
        strcpy(res, "completed");
    }
}

void 
handleTradesFNO(State *state, char *res)
{
    FILE *FTradesFile = fopen("tmp.csv", "r");
    if (FTradesFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    int i = 0;
    char stratSymbol[100];
    FILE *FTradesFileCopy = fopen("tmp.csv", "r");
    if (FTradesFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), FTradesFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, fnoTradesHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    int result = processTrades(FTradesFile, stratId, state);
    if (result < 0)
    {
        strcpy(res, "couldn't find strategy");
    }
    else
    {
        strcpy(res, "completed");
    }
}

void
handleFundExpense(State *state, char *res)
{
    char line[4096];
    int i = 0;
    FILE *expenseFile = fopen("tmp.csv", "r");
    if (expenseFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *expenseFileCopy = fopen("tmp.csv", "r");
    if (expenseFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), expenseFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, expenseHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), expenseFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        ++state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
        AccountFromExpense(&assetEntry, &liabEntry, line);
        DBInsertLedgerEntry(state->db, &assetEntry, stratId);
        DBInsertLedgerEntry(state->db, &liabEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }
    strcpy(res, "completed");
}

void
handleAllotUnits(State *state, char *res)
{
    char line[4096];
    int i = 0;
    FILE *unitFile = fopen("tmp.csv", "r");
    if (unitFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    while (fgets(line, sizeof(line), unitFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            TrimString(line);
            if(ValidateCsvHeader(line, unitsHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        allotUnits(state, line);
        i++;
    }
    strcpy(res, "completed");
}

void
handleCashFlow(State *state, char *res)
{
    char line[4096];
    int i = 0;
    FILE *cashflowFile = fopen("tmp.csv", "r");
    if (cashflowFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *cashflowFileCopy = fopen("tmp.csv", "r");
    if (cashflowFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), cashflowFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, cashflowHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), cashflowFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        /* NOTE(Akhil): here we are working on the latest strategy.
                        usually first column discloses the strategy name. */
        ++state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
        AccountFromCashFlow(&assetEntry, &liabEntry, line);
        DBInsertLedgerEntry(state->db, &assetEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        printf("cashflow entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        DBInsertLedgerEntry(state->db, &liabEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("cashflow entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        i++;
    }
    strcpy(res, "completed");
}

void
handleReverseUPA(State *state, char *res)
{
    char line[4096];
    int i = 0;
    FILE *reverseFile = fopen("tmp.csv", "r");
    if (reverseFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }
    /* TODO(Akhil): this can be abstracted into a func */
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *reverseFileCopy = fopen("tmp.csv", "r");
    if (reverseFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), reverseFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, revHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), reverseFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }

        ++state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry liabEntry = {};
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
        LedgerEntry assetEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        AccountFromReverse(&assetEntry, &liabEntry, line);
        DBInsertLedgerEntry(state->db, &liabEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;
        printf("entry name is %s and value is %f\n", liabEntry.accountName,
               liabEntry.credit);
        DBInsertLedgerEntry(state->db, &assetEntry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.credit);
        i++;
    }
    strcpy(res, "completed");
}

/* TODO(Akhil): Persist the ledger entries here */
void
handleBankTransfer(State *state, char *res)
{
    char line[4096];
    FILE *bankFile = fopen("tmp.csv", "r");
    if (bankFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    int i = 0;
    /* fetch the strat symbol from the file */
    char stratSymbol[100];
    FILE *bankFileCopy = fopen("tmp.csv", "r");
    if (bankFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
        strcpy(res, "couldn't upload file");
        return;
    }

    char copyLine[4096];
    while (fgets(copyLine, sizeof(copyLine), bankFileCopy))
    {
        if (i == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, bankHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        if (i == 1)
        {
            LoadStratSymbolFromFile(copyLine, stratSymbol);
            break;
        }
        i++;
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    i = 0;
    while (fgets(line, sizeof(line), bankFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            
            i++;
            continue; // ignore the top heading row.
        }
        ++state->strategies[state->currStratIndex].currJournalId;

        LedgerEntry assetEntry = {};
        LedgerEntry liabEntry = {};
        assetEntry.id = state->strategies[state->currStratIndex].currJournalId;
        liabEntry.id = state->strategies[state->currStratIndex].currJournalId;
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

        PGresult *pgResult = executeQuery(state->db, query);
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
            DBInsertBankAcc(state->db, &acc, stratId);

            // insert in memory as well.
            state->strategies[state->currStratIndex]
                .accs[++state->strategies[state->currStratIndex].currAccIndex] = acc;

        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state->strategies[state->currStratIndex].currAccIndex;
            i++)
            {
                if(strcmp(state->strategies[state->currStratIndex].accs[i].symbol,
                          liabEntry.accountName) == 0)   
                {
                    printf("bal before %f\n", 
                           state->strategies[state->currStratIndex].accs[i].balance);
                    state->strategies[state->currStratIndex].accs[i].balance -=
                        liabEntry.credit;
                    printf("bal after %f\n", 
                           state->strategies[state->currStratIndex].accs[i].balance);

                    // db.
                    DBUpdateBankBalance(state->db,
                                        state->strategies[state->currStratIndex].accs[i].balance,
                                        liabEntry.accountName);
                }
            }
            PQclear(pgResult);
        }

        // insert or update the assetEntry bank acc.
        printf("asset entry accoutnn name is %s\n", assetEntry.accountName);
        sprintf(query,
                "SELECT * FROM bank_account where symbol = '%s'",
                assetEntry.accountName);

        pgResult = executeQuery(state->db, query);
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
            DBInsertBankAcc(state->db, &acc, stratId);

            // insert in memory as well.
            state->strategies[state->currStratIndex]
                .accs[++state->strategies[state->currStratIndex].currAccIndex] = acc;
        }
        else
        {
            //update the bank acc balance.
            // first, in memory, then in db.
            for (int i = 0; i <= state->strategies[state->currStratIndex].currAccIndex;
            i++)
            {
                if(strcmp(state->strategies[state->currStratIndex].accs[i].symbol,
                          assetEntry.accountName) == 0)   
                {
                    state->strategies[state->currStratIndex].accs[i].balance +=
                        assetEntry.debit;
                    // db.
                    DBUpdateBankBalance(state->db,
                                        state->strategies[state->currStratIndex].accs[i].balance,
                                        assetEntry.accountName);
                }
            }
            PQclear(pgResult);
        }
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = assetEntry;
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = liabEntry;

        /* update the currAccIndex meta field on strategy; */
        snprintf(query, sizeof(query),
                 "UPDATE strategy SET curr_acc_index = %d WHERE id = %d;",
                 state->strategies[state->currStratIndex].currAccIndex,
                 stratId
                 );
        pgResult = executeQuery(state->db, query);
        PQclear(pgResult);
        printf("entry name is %s and value is %f\n", assetEntry.accountName,
               assetEntry.debit);
        i++;
    }
    strcpy(res, "completed");
}

void
handleSubsUPA(State *state, char *res, char *stratSymbol)
{
    char line[4096];
    FILE *subsFile = fopen("tmp.csv", "r");
    if (subsFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    /* open the file again to get the investor name to
     * fetch from the db
    */

    Investor inv = {};
    FILE *subsFileCopy = fopen("tmp.csv", "r");
    if (subsFileCopy == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }
    char copyLine[4096];
    int k = 0;
    while (fgets(copyLine, sizeof(copyLine), subsFileCopy))
    {
        TrimString(copyLine);
        printf("line is %s\n", copyLine);
        if (k == 0)
        {
            TrimString(copyLine);
            if(ValidateCsvHeader(copyLine, subsHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            k++;
            continue;
        }

        if (k == 1)
        {
            /* get the investor from the db. */
            char invName[100];
            LoadInvNameFromFile(copyLine, invName);
            char query[1024];
            sprintf(query,
                    "SELECT * FROM investor where name = '%s' LIMIT 1",
                    invName);

            PGresult *pgResult = executeQuery(state->db, query);
            int rows = PQntuples(pgResult);
            if (rows == 0)
            {
                printf("No investor found matching name: %s\n", invName);
                PQclear(pgResult);
            }
            else
            {
                /* there will only be one row */
                char *idStr = PQgetvalue(pgResult, 0, 0);
                int id = atoi(idStr);
                inv.id = id;
                char *nameStr = PQgetvalue(pgResult, 0, 3);
                printf("name str is %s\n", nameStr);
                strcpy(inv.name, nameStr);
                char *unitsStr = PQgetvalue(pgResult, 0, 4);
                printf("units str is %s\n", unitsStr);
                real64 units = atof(unitsStr);
                inv.units = units;
            }
        }
        k++;
    }


    int i = 0;
    ++state->strategies[state->currStratIndex].currJournalId; // same id for the couple.
     

    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }
    while (fgets(line, sizeof(line), subsFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            i++;
            continue; // ignore the top heading row.
        }
        LedgerEntry entry = {};
        entry.id = state->strategies[state->currStratIndex].currJournalId;
        if (i == 1) entry.type = EQUITY;
        else if (i == 2) entry.type = ASSET;
        AccountFromSubs(&entry, state, inv, stratSymbol, line);
        printf("strat symbol is %s\n", stratSymbol);
        if (i == 2)
        {
            // Link the investor to the strategy in the db as well.
            char query[1024];
            sprintf(query,
                    "SELECT id FROM strategy where symbol = '%s' LIMIT 1",
                    stratSymbol);

            PGresult *pgResult = executeQuery(state->db, query);

            if (PQntuples(pgResult) == 0)
            {
                fprintf(stderr, "No strategy found matching symbol: %s\n", stratSymbol);
                PQclear(pgResult);
                sprintf((char *)res, "No strategy found for symbol %s", stratSymbol);
                return;
            }

            char *id_str = PQgetvalue(pgResult, 0, 0);
            stratId = atoi(id_str);
            PQclear(pgResult);

            sprintf(query,
                    "UPDATE investor SET strategy_id = %d where name = '%s'",
                    stratId,
                    inv.name);
            pgResult = executeQuery(state->db, query);
            PQclear(pgResult);

        }
        DBInsertLedgerEntry(state->db, &entry, stratId);
        state->strategies[state->currStratIndex].ledger[++state->strategies[state->currStratIndex].currEntryId] = entry;
        printf("entry name is %s and value is %f\n", entry.accountName, entry.debit);
        i++;
    }
    strcpy(res, "completed");
}

void
handleAddInvestor(State *state, char *stratSymbol, char *res)
{
    char line[4096];
    Investor inv = {};
    FILE *clientFile = fopen("tmp.csv", "r");
    if (clientFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    int stratIndex = getStratIndex(state, stratSymbol);
    int i = 0;
    PGresult *pgResult;
    while (fgets(line, sizeof(line), clientFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            TrimString(line);
            if(ValidateCsvHeader(line, investorHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        LoadInvestorFromClient(&inv, line);

        /* put it in memory */
        state->strategies[stratIndex].investors
            [++state->strategies[stratIndex].currInvestorIndex] = inv;

        int billId = DBgetBillId(state->db, inv.billGroup);

        /* persist it in db */
        char query[1096];
        sprintf(query,
                "INSERT INTO investor"
                "(name, strategy_id, inception_date, bill_group_id, last_nav, last_perf_fee_date) "
                "VALUES ('%s', %d, '%s', %d, %f, '%s') "
                "ON CONFLICT (name) DO UPDATE SET units = EXCLUDED.units, cash = EXCLUDED.cash;",
                inv.name, stratId, inv.inceptionDate, billId, inv.lastNav, inv.lastPerfFeeDate);

        pgResult = executeQuery(state->db, query);
    }
    /* Assumes there is only one row in the file we read */
    char *error = PQresultErrorMessage(pgResult);
    if (strcmp(error, "") != 0)
    {
        printf("%s", error);
        strcpy(res, error);
    }
    else
    {
        strcpy(res, "completed");
    }
    PQclear(pgResult); 
}

void
handleCreateStrategy(State *state, char *res)
{
    FILE *stratFile = fopen("tmp.csv", "r");
    char line[4096];
    if (stratFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    Strategy strategy = {};
    int i = 0;
    PGresult *pgResult;
    while (fgets(line, sizeof(line), stratFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            TrimString(line);
            if(ValidateCsvHeader(line, strategyHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }
        LoadStrategyFromFile(&strategy, line);
        strategy.cash = 0;
        strategy.id = ++state->currStratIndex;
        state->strategies[state->currStratIndex].currEntryId = -1;
        state->strategies[state->currStratIndex] = strategy;
        printf("strategy id is %d\n", state->strategies[state->currStratIndex].id);
        printf("strategy name is %s\n", state->strategies[state->currStratIndex].symbol);
        char query[512];
        sprintf(query,
                "INSERT INTO strategy"
                "(symbol, cash) "
                "VALUES ('%s', %f) "
                "ON CONFLICT (symbol) DO NOTHING;",
                strategy.symbol,
                strategy.cash);

        pgResult = executeQuery(state->db, query);
        i++;
    }
    /* Assumes there is only one row in the file we read */
    char *error = PQresultErrorMessage(pgResult);
    if (strcmp(error, "") != 0)
    {
        printf("%s", error);
        strcpy(res, error);
    }
    else
    {
        strcpy(res, "completed");
    }
    PQclear(pgResult); 
}

void
handleExchangeRate(State *state, char *stratSymbol, char *res)
{
    FILE *exchangeRateFile = fopen("tmp.csv", "r");
    if (exchangeRateFile == NULL)
    {
        printf("sorry, couldn't upload file!\n");
    }

    /* fetch the strategy's id from the db */
    int stratId = getStratId(stratSymbol, state->db); 
    if (stratId < 0)
    {
        sprintf(res, "No strategy found matching symbol: %s\n", stratSymbol);
        return;
    }

    char line[1024];
    int i = 0;
    Exchange_rate exRate = {};
    PGresult *pgResult;
    while (fgets(line, sizeof(line), exchangeRateFile))
    {
        TrimString(line);

        if (line[0] == '\0') {
            continue; 
        } 

        if (i == 0)
        {
            TrimString(line);
            if(ValidateCsvHeader(line, exchangeRateHeader) != 0)
            {
                strcpy(res, invalidfileformaterror);
                return;
            }
            i++;
            continue; // ignore the top heading row.
        }

        LoadExchangeRate(&exRate, line);
        // Persist the exchange rate in db.
        char query[512];
        /* Prevent duplicate entries */
        sprintf(query,
                "INSERT INTO exchange_rate "
                "(curr, rate, date, base, strategy_id) "
                "VALUES ('%s', %f, '%s', '%s', %d) "
                "ON CONFLICT (curr, base, date) DO UPDATE SET rate = EXCLUDED.rate;",
                exRate.curr == USD ? "USD" : "INR",
                exRate.rate,
                exRate.date,
                exRate.base == USD ? "USD" : "INR",
                stratId);

        pgResult = executeQuery(state->db, query);
        state->exRates[i - 1] = exRate; // it's a copy here.
        printf("ex rate is %f\n", state->exRates[i - 1].rate);
    }
    /* Assumes there is only one row in the file we read */
    char *error = PQresultErrorMessage(pgResult);
    if (strcmp(error, "") != 0)
    {
        printf("%s", error);
        strcpy(res, error);
    }
    else
    {
        strcpy(res, "completed");
    }
    PQclear(pgResult);
}

/* --------------- Http server------------------------------------------------*/
static enum MHD_Result
send_page (struct MHD_Connection *connection,
           const char *page,
           unsigned int status_code)
{
    enum MHD_Result ret;
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen (page),
                                               (void *)page,
                                               MHD_RESPMEM_MUST_COPY);
    if (! response)
        return MHD_NO;
    if (MHD_YES !=
        MHD_add_response_header (response,
                                 MHD_HTTP_HEADER_CONTENT_TYPE,
                                 "application/json"))
    {
        fprintf (stderr,
                 "Failed to set content type header!\n");
    }
    ret = MHD_queue_response (connection,
                              status_code,
                              response);
    MHD_destroy_response (response);
    return ret;
}

static enum MHD_Result
iterate_post (void *coninfo_cls,
              enum MHD_ValueKind kind,
              const char *key,
              const char *filename,
              const char *content_type,
              const char *transfer_encoding,
              const char *data,
              uint64_t off,
              size_t size)
{
    connection_info_struct *con_info = (connection_info_struct *)coninfo_cls;
    /* FILE *fp; */
    (void) kind; /* Unused. Silent compiler warning. */
    (void) content_type; /* Unused. Silent compiler warning. */
    (void) transfer_encoding; /* Unused. Silent compiler warning. */
    (void) filename; /* Unused. Silent compiler warning. */
    if (strcmp(key, "strategySymbol") == 0)
    {
        memcpy(con_info->strategySymbol + off, data, size);
        con_info->strategySymbol[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "hurldlerate") == 0)
    {
        memcpy(con_info->hurdlerate + off, data, size);
        con_info->hurdlerate[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "billSymbol") == 0)
    {
        memcpy(con_info->billSymbol + off, data, size);
        con_info->billSymbol[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "frequency") == 0)
    {
        memcpy(con_info->frequency + off, data, size);
        con_info->frequency[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "perfFee") == 0)
    {
        memcpy(con_info->perfFee + off, data, size);
        con_info->perfFee[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "invName") == 0)
    {
        memcpy(con_info->invName + off, data, size);
        con_info->invName[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "date") == 0)
    {
        memcpy(con_info->date + off, data, size);
        con_info->date[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "fromDate") == 0)
    {
        memcpy(con_info->fromDate + off, data, size);
        con_info->fromDate[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "toDate") == 0)
    {
        memcpy(con_info->toDate + off, data, size);
        con_info->toDate[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "rollbackDate") == 0)
    {
        memcpy(con_info->rollbackDate + off, data, size);
        con_info->rollbackDate[off + size] = '\0';
        return MHD_YES;
    }
    else if (strcmp(key, "snapshotDate") == 0)
    {
        memcpy(con_info->snapshotDate + off, data, size);
        con_info->snapshotDate[off + size] = '\0';
        return MHD_YES;
    }
    else if (0 != strcmp (key, "file"))
    {
        con_info->answerstring = servererrorpage;
        con_info->answercode = MHD_HTTP_BAD_REQUEST;
        return MHD_YES;
    }
    if (! con_info->fp)
    {
        if (0 != con_info->answercode) /* something went wrong */
            return MHD_YES;
        /* if (NULL != (fp = fopen (filename, "rb")))
         * {
         *    fclose (fp);
         *    con_info->answerstring = fileexistspage;
         *    con_info->answercode = MHD_HTTP_FORBIDDEN;
         *    return MHD_YES;
         } */
        /* NOTE: This is technically a race with the 'fopen()' above,
but there is no easy fix, short of moving to open(O_EXCL)
instead of using fopen(). For the example, we do not care. */
        con_info->fp = fopen ("tmp.csv", "w");
        if (! con_info->fp)
        {
            con_info->answerstring = fileioerror;
            con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
            return MHD_YES;
        }
    }
    if (size > 0)
    {
        if (! fwrite (data, sizeof (char), size, con_info->fp))
        {
            con_info->answerstring = fileioerror;
            con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
            return MHD_YES;
        }
    }
    return MHD_YES;
}

static void
request_completed (void *cls,
                   struct MHD_Connection *connection,
                   void **req_cls,
                   enum MHD_RequestTerminationCode toe)
{
    connection_info_struct *con_info = (connection_info_struct *)*req_cls;
    (void) cls; /* Unused. Silent compiler warning. */
    (void) connection; /* Unused. Silent compiler warning. */
    (void) toe; /* Unused. Silent compiler warning. */
    if (NULL == con_info)
        return;
    if (con_info->connectiontype == POST)
    {
        if (NULL != con_info->postprocessor)
        {
            MHD_destroy_post_processor (con_info->postprocessor);
        }
        if (con_info->fp)
            fclose (con_info->fp);
    }
    free (con_info);
    *req_cls = NULL;
}

static enum MHD_Result
answer_to_connection (void *cls,
                      struct MHD_Connection *connection,
                      const char *url,
                      const char *method,
                      const char *version,
                      const char *upload_data,
                      size_t *upload_data_size,
                      void **req_cls)
{
    (void) url; /* Unused. Silent compiler warning. */
    (void) version; /* Unused. Silent compiler warning. */
    State *state = (State *)cls;
    if (NULL == *req_cls)
    {
        /* First call, setup data structures */
        connection_info_struct *con_info;

        con_info = (connection_info_struct *)malloc (sizeof (connection_info_struct));
        if (NULL == con_info)
            return MHD_NO;
        con_info->answercode = 0; /* none yet */
        con_info->fp = NULL;
        if (0 == strcmp (method, MHD_HTTP_METHOD_POST))
        {
            con_info->postprocessor =
                MHD_create_post_processor (connection,
                                           POSTBUFFERSIZE,
                                           &iterate_post,
                                           (void *) con_info);
            if (NULL == con_info->postprocessor)
            {
                free (con_info);
                return MHD_NO;
            }
            con_info->connectiontype = POST;
        }
        else
        {
            con_info->connectiontype = GET;
        }
        *req_cls = (void *) con_info;
        return MHD_YES;
    }
    if (0 == strcmp (method, MHD_HTTP_METHOD_GET))
    {
        connection_info_struct *con_info = (connection_info_struct *)*req_cls;
        /* We just return the standard form for uploads on all GET requests */
        char buffer[40960];
        con_info->answerstring = buffer;
        if (0 == strcmp(url, "/get-bill-groups"))
        {
            handleGetBillGroups(state,
                                con_info->answerstring);
            return send_page (connection,
                              con_info->answerstring,
                              MHD_HTTP_OK);
        }
        else
        {
            snprintf (buffer,
                      sizeof (buffer),
                      ASKPAGE,
                      0);
            return send_page (connection,
                              buffer,
                              MHD_HTTP_OK);
        }
    }
    if (0 == strcmp (method, MHD_HTTP_METHOD_POST))
    {
        connection_info_struct *con_info = (connection_info_struct *)*req_cls;
        if (0 != *upload_data_size)
        {
            /* Upload not yet done */
            if (0 != con_info->answercode)
            {
                /* we already know the answer, skip rest of upload */
                *upload_data_size = 0;
                return MHD_YES;
            }
            if (MHD_YES !=
                MHD_post_process (con_info->postprocessor,
                                  upload_data,
                                  *upload_data_size))
            {
                con_info->answerstring = postprocerror;
                con_info->answercode = MHD_HTTP_INTERNAL_SERVER_ERROR;
            }
            *upload_data_size = 0;
            return MHD_YES;
        }
        /* Upload finished as upload data size is 0 */
        if (NULL != con_info->fp)
        {
            fclose (con_info->fp);
            con_info->fp = NULL;
        }
        if (0 == con_info->answercode)
        {
            /* No errors encountered, declare success */
            con_info->answerstring = completepage;
            con_info->answercode = MHD_HTTP_OK;
        }

        /* json are copied here */
        char buffer[40960];
        con_info->answerstring = buffer;
        /* form data included in con_info struct */
        if (0 == strcmp(url, "/exchange-rate"))
        {
            handleExchangeRate(state, con_info->strategySymbol, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/create-strategy"))
        {
            handleCreateStrategy(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/add-investor"))
        {
            handleAddInvestor(state, con_info->strategySymbol, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/subs-upa"))
        {
            handleSubsUPA(state, con_info->answerstring, con_info->strategySymbol);
        }
        else if (0 == strcmp(url, "/bank-transfer"))
        {
            handleBankTransfer(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/reverse-upa"))
        {
            handleReverseUPA(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/fund-cashflow"))
        {
            handleCashFlow(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/allot-units"))
        {
            handleAllotUnits(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/fund-expense"))
        {
            handleFundExpense(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/trades-fno"))
        {
            handleTradesFNO(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/trades-equity"))
        {
            handleTradesEq(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/bhav-fno"))
        {
            handleBhavFNO(state, con_info->date, con_info->strategySymbol, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/bhav-eq"))
        {
            handleBhavEq(state, con_info->date, con_info->strategySymbol, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/mtm-process"))
        {
            handleMTM(state, con_info->strategySymbol, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/corporate-action"))
        {
            handleCorpAction(state,
                             con_info->strategySymbol,
                             con_info->date,
                             con_info->answerstring);
        }
        else if (0 == strcmp(url, "/process-nav"))
        {
            handleNAV(state,
                      con_info->strategySymbol,
                      con_info->date,
                      con_info->answerstring);
        }
        else if (0 == strcmp(url, "/clawback-nav"))
        {
            handleClawNAV(state,
                          con_info->strategySymbol,
                          con_info->rollbackDate,
                          con_info->snapshotDate,
                          con_info->answerstring);
        }
        else if (0 == strcmp(url, "/offboard-cashflow"))
        {
            handleOffCashFlow(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/offboard-redeem"))
        {
            handleOffRedeem(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/offboard-bank"))
        {
            handleOffBank(state, con_info->invName, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/upload-securities"))
        {
            handleUploadSecurities(state, con_info->answerstring);
        }
        else if (0 == strcmp(url, "/upload-positions"))
        {
            handleUploadPositions(state,
                                  con_info->strategySymbol,
                                  con_info->answerstring);
        }
        else if (0 == strcmp(url, "/upload-balances"))
        {
            handleBalances(state,
                           con_info->strategySymbol,
                           con_info->answerstring);
        }
        else if (0 == strcmp(url, "/nav-report"))
        {
            handleNAVReport(state,
                            con_info->strategySymbol,
                            con_info->fromDate,
                            con_info->toDate,
                            con_info->answerstring);
        }
        else if (0 == strcmp(url, "/positions-report"))
        {
            handlePosReport(state,
                            con_info->strategySymbol,
                            con_info->date,
                            con_info->answerstring);
        }
        else if (0 == strcmp(url, "/performance-fee"))
        {
            handlePerfFee(state,
                          con_info->invName,
                          con_info->strategySymbol,
                          con_info->date,
                          con_info->answerstring);
        }
        else if (0 == strcmp(url, "/bill-group"))
        {
            handleCreateBillGroup(state,
                                  atof(con_info->hurdlerate),
                                  atof(con_info->perfFee),
                                  con_info->frequency,
                                  con_info->billSymbol,
                                  con_info->date,
                                  con_info->answerstring);
        }
        /* NOTE(AKHIL) : useless, since the linking is being 
         * done thru addinvestor route, where billgroyp symbol
         * is passed down thru the file */
        else if (0 == strcmp(url, "/link-bill-investor"))
        {
            handleLinkBillGroup(state,
                                con_info->invName,
                                con_info->strategySymbol,
                                con_info->billSymbol,
                                con_info->answerstring);
        }

        return send_page (connection,
                          con_info->answerstring,
                          con_info->answercode);
    }
    /* Note a GET or a POST, generate error */
    return send_page (connection,
                      errorpage,
                      MHD_HTTP_BAD_REQUEST);
}

/* --------------- Main ------------------------------------------------*/
int
main()
{
    const char *connStr = getenv("PG_CONN");
    if (connStr == NULL)
    {
        fprintf(stderr, "Provide the db connection string\n");
        return -2;
    }
    PGconn *conn = PQconnectdb(connStr);
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

    State *state = (State *)malloc(megabytes(256));
    if (state == NULL)
    {
        printf("couldn't allocate memory for state, abort\n");
        return -1;
    }
    state->db = conn;
    loadStateFromDB(state);

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD,
                               PORT, NULL, NULL,
                               &answer_to_connection, state,
                               MHD_OPTION_NOTIFY_COMPLETED, &request_completed,
                               NULL,
                               MHD_OPTION_END); 
    if (NULL == daemon) {
        fprintf(stderr, "Failed to start MHD daemon\n");
        PQfinish(conn);
        free(state);
        return 1;
    } 

    while(true) {} 
    
    // Graceful cleanup on systemctl stop
    MHD_stop_daemon (daemon);
    PQfinish(conn);
    free(state);
    return 0;
}
