// Client Side Socket

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 5432


int main(int argc, char const* argv[])
{
	int fd;
	struct sockaddr_in serv_addr;
	char buffer[1024] = { 0 };

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		printf("\n Socket creation error \n");
		return -1;
	}

	// Server side = postgres DB
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("172.18.0.8");
	serv_addr.sin_port = htons(PORT);

	if ( connect(fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 ) {
		printf("\nConnection Failed \n");
		return -1;
	}

	//for(;;){} ---> infinite loop
	while(1){
		if( read(fd, buffer, sizeof(buffer) - 1) > 0 ){
			printf("%s\n", buffer);
		}

            //update dispatcher.list file 


            //update re_grp table



            //update cache 

	}
  
	// closing the connected socket
	close(fd);
	return 0;
}




