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

	query = PQexec(dbconn, "LISTEN tbl2");
    if (PQresultStatus(query) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "LISTEN command failed: %s\n", PQerrorMessage(dbconn));
        PQclear(query);
		close_connection(dbconn);  
    }
    PQclear(query);
	
	int sock, rows, cols, i, j;
	fd_set reading;	
	PGnotify   *notify;
	FILE *fp;
	while(1){

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
			fprintf(stderr, "ASYNC NOTIFY of '%s' received from backend PID %d with payload of %s\n", notify->relname, notify->be_pid, notify->extra);
			PQfreemem(notify);			
			PQconsumeInput(dbconn);
		}

		query = PQexec(dbconn, "SELECT * FROM public.re_grp");			
		if (PQresultStatus(query) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "Error while executing the query: %s\n", PQerrorMessage(dbconn));
			PQclear(query);
			close_connection(dbconn);  
		}

		rows = PQntuples(query);
		cols = PQnfields(query);

		fp = fopen("/etc/kamailio/dbtext/re_grp_temp", "w");

		for (i = 0; i < cols; i++) {
			fprintf(fp,"%s\t", PQfname(query, i));
		}
		fprintf(fp,"\n");

		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {            
				fprintf(fp,"%s|", PQgetvalue(query, i, j));
			}
			fprintf(fp,"\n");
		}

		fclose(fp);		
		
		PQclear(query);

		query = PQexec(dbconn, "SELECT * FROM public.dispatcher");
		if (PQresultStatus(query) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "Error while executing the query: %s\n", PQerrorMessage(dbconn));
			PQclear(query);
			close_connection(dbconn);  
		}

		rows = PQntuples(query);
		cols = PQnfields(query);

		fp = fopen("/etc/kamailio/dbtext/dispatcher_temp", "w");

		for (i = 0; i < cols; i++) {
			fprintf(fp,"%s\t", PQfname(query, i));
		}
		fprintf(fp,"\n");

		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {            
				fprintf(fp,"%s|", PQgetvalue(query, i, j));
			}
			fprintf(fp,"\n");
		}

		fclose(fp);

		PQclear(query);

		rename("/etc/kamailio/dbtext/dispatcher_temp", "/etc/kamailio/dbtext/dispatcher");
		rename("/etc/kamailio/dbtext/re_grp_temp", "/etc/kamailio/dbtext/re_grp");

		//no need to rename files - write on it directly - core just read data from cache 		
		//run linux command from c code
		//call kamcmd -s udp:172.18.0.2:3000 db_text.query 'select * from dispatcher'
		//call kamcmd -s udp:172.18.0.2:3000 db_text.query 'select * from re_grp'
		//system("kamcmd -s udp:172.18.0.2:3000 db_text.query 'select * from dispatcher'")
	}

	fprintf(stderr, "Done.\n");

	PQfinish(dbconn);
	
	return 0;
}




