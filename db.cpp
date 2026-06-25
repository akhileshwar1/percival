#include <libpq-fe.h>

int
main()
{
    printf("hey, there!\n");
    PGconn *conn = PQconnectdb("dbname=percival");
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "%s\n", PQerrorMessage(conn));
        return -1;
    }
    printf("connection established\n");
}
