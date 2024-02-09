//Asynchronous Notification Interface
//Client-Side

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <libpq-fe.h>


static void close_connection(PGconn *dbconn)
{
    PQfinish(dbconn);
    exit(1);
}

int main(int argc, char const** argv)
{	
	PGconn *dbconn; 
	dbconn=PQconnectdb("postgresql://kamailio:kamailiorw@172.18.0.8:5432/kamailio");
	if (PQstatus(dbconn) != CONNECTION_OK) {
		fprintf(stderr, "%s", PQerrorMessage(dbconn));
		close_connection(dbconn);
	}

	PGresult *query;
	query = PQexec(dbconn, "SELECT pg_catalog.set_config('search_path', '', false)");
	if (PQresultStatus(query) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "SET failed: %s", PQerrorMessage(dbconn));
        PQclear(query);
		close_connection(dbconn);        
    }
	PQclear(query);

	query = PQexec(dbconn, "LISTEN TBL2");
    if (PQresultStatus(query) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "LISTEN command failed: %s", PQerrorMessage(dbconn));
        PQclear(query);
		close_connection(dbconn);  
    }
    PQclear(query);

	int nnotifies = 0;
	int sock;
	fd_set reading;	
	PGnotify   *notify;

	while(nnotifies < 4){	

		sock=PQsocket(dbconn);
		if(sock<0) break;
		
		FD_ZERO(&reading);
		FD_SET(sock, &reading);

		if(select(sock+1, &reading, NULL, NULL, NULL) < 0){
			fprintf(stderr, "select() failed: %s\n", strerror(errno));
			close_connection(dbconn);
		}

		PQconsumeInput(dbconn);
		while((notify = PQnotifies(dbconn)) != NULL){
			//notify->extra notification payload string
			//notify->relname channel name
			fprintf(stderr, "ASYNC NOTIFY of '%s' received from backend PID %d\n", notify->relname, notify->be_pid);
			PQfreemem(notify);
			nnotifies++;
			PQconsumeInput(dbconn);
		}
	}

	fprintf(stderr, "Done.\n");

	PQfinish(dbconn);
	
	return 0;
}




