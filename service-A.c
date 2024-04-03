//Asynchronous Notification Interface
//Client-Side
//update the database - dbtext /etc/kamailio/dbtext
//update the corresponding htables

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include <libpq-fe.h>

#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80
#define PORT 8228
#define SA struct sockaddr


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
	
	int sock, rows, cols, i, j, sockfd;
	fd_set reading;	
	PGnotify   *notify;
	FILE *fp;
	struct sockaddr_in servaddr;

	// socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
          printf("socket creation failed...\n");
          exit(0);
        }
        else        
	  printf("Socket successfully created..\n");

        bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr("172.18.0.2");
        servaddr.sin_port = htons(PORT);

	// connect the client socket to server socket
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))!= 0) {
          printf("connection with the server failed...\n");
          exit(0);
        }
        else
          printf("connected to the server..\n");

	char* msg = "8:database,2:is,7:updated";

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

		query = PQexec(dbconn, "SELECT reg_exp, group_id FROM public.re_grp");			
		if (PQresultStatus(query) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "Error while executing the query: %s\n", PQerrorMessage(dbconn));
			PQclear(query);
			close_connection(dbconn);  
		}

		rows = PQntuples(query);
		cols = PQnfields(query);

		// fp = fopen("/etc/kamailio/dbtext/re_grp_temp", "w");
		fp = fopen("/etc/kamailio/dbtext/re_grp", "w");

		/*for (i = 0; i < cols; i++) {
			fprintf(fp,"%s ", PQfname(query, i));
		}*/

		fprintf(fp,"%s ", "reg_exp(string) group_id(int)");
		fprintf(fp,"\n");

		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {            
				fprintf(fp,"%s|", PQgetvalue(query, i, j));
			}
			fprintf(fp,"\n");
		}

		fclose(fp);		
		
		PQclear(query);

		query = PQexec(dbconn, "SELECT setid, destination, flags, priority, attrs FROM public.dispatcher");
		if (PQresultStatus(query) != PGRES_TUPLES_OK)
		{
			fprintf(stderr, "Error while executing the query: %s\n", PQerrorMessage(dbconn));
			PQclear(query);
			close_connection(dbconn);  
		}

		rows = PQntuples(query);
		cols = PQnfields(query);

		// fp = fopen("/etc/kamailio/dbtext/dispatcher_temp", "w");
		fp = fopen("/etc/kamailio/dbtext/dispatcher", "w");

		/*for (i = 0; i < cols; i++) {
			fprintf(fp,"%s ", PQfname(query, i));
		}*/

		fprintf(fp,"%s ", "setid(int) destination(string) flags(int) priority(int) attrs(string)");
		fprintf(fp,"\n");

		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {            
				fprintf(fp,"%s|", PQgetvalue(query, i, j));
			}
			fprintf(fp,"\n");
		}

		fclose(fp);

		PQclear(query);

		// rename("/etc/kamailio/dbtext/dispatcher_temp", "/etc/kamailio/dbtext/dispatcher");
		// rename("/etc/kamailio/dbtext/re_grp_temp", "/etc/kamailio/dbtext/re_grp");

		//no need to rename files - write on it directly - core just read data from cache 		
		//run linux command from c code
		//call kamcmd -s udp:172.18.0.2:3000 db_text.query 'select * from dispatcher'
		//call kamcmd -s udp:172.18.0.2:3000 db_text.query 'select * from re_grp'
		//mechanism used to update cache
		//system("kamcmd -s udp:172.18.0.2:3000 htable.reload ha_re_grp");
		//system("kamcmd -s udp:172.18.0.2:3000 htable.reload ha_dispatcher");

		//write(sockfd, msg, strlen(msg));
		// Send the message to server:
    		if(send(sockfd, msg, strlen(msg), 0) < 0){
        		printf("Unable to send message\n");
        		return -1;
      		}
	}

	fprintf(stderr, "Done.\n");

	PQfinish(dbconn);

	// close the socket
        close(sockfd);
	
	return 0;
}




