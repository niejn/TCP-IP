//JASON ZHANG
//860798117
//CS 164 PROJECT


#include <stdio.h>		/*for printf() and fprintf()*/
#include <sys/socket.h>		/*for socket(), bind() and connect()*/
#include <sys/select.h>
#include <arpa/inet.h>		/*for sockaddr_in() and inet_ntoa()*/
#include <stdlib.h>		/*for atoi()*/
#include <string.h>		/*for memset()*/
#include <unistd.h>		/*for close()*/
#include <sys/wait.h>       /* for waitpid() */
#include <iostream>
#include <netdb.h>
#include <sys/time.h>
#include <vector>
#include <math.h>

using namespace std;

#define MAXPENDING 5   /*the maximum number of connetion requests*/
#define RCVBUFFERSIZE 64




int main(int argc,char *argv[])
{

	int rootSock;			/*the socket descriptor for the server socket*/
	int slaveSock;			/*the socket descriptor for the client socket*/
	int midSock;
	fd_set master;
	fd_set serv_set;
	int fdmax;
	int newfd;
	struct sockaddr_in echoServAddr;/*Local - Server address*/
	struct sockaddr_in echoClntAddr;/*Client Address*/
	int echoServPort;		/*Server Port*/
	socklen_t clntLen;			/*Length of Client address data structure*/
	char echoBuffer[RCVBUFFERSIZE];
	int recvMsgSize;

	string sserver_name;
	string sserver_port;	
	struct addrinfo servaddr, *servinfo, *p;
	char serv_addr[INET_ADDRSTRLEN];
	int rv;

	struct timeval now_server;	//time structures declared
	struct timeval now_receive;
	struct timeval transmit;
	struct timeval wait;
    	int rc;

	vector<int>sockets;


	if(argc!=2)
	{
		fprintf(stderr,"Usage : %s <Server port type (root or slave)>\n",argv[0]);
		exit(1);
	}
	string type = argv[1];

	if(type=="root")
	{
		cout << "This will be the root server, please input port number: ";
		cin >> echoServPort;

		if((rootSock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
		{
			perror("socket() failed");
			exit(1);
		}
	
	/*construct the local address for the server*/
		memset(&echoServAddr,0,sizeof(echoServAddr));
		echoServAddr.sin_family=AF_INET;
		echoServAddr.sin_port=htons(echoServPort);
		echoServAddr.sin_addr.s_addr=htonl(INADDR_ANY);


	/*bind the server socket*/
		if(bind(rootSock,(struct sockaddr *)&echoServAddr,sizeof(echoServAddr))<0)
		{
			perror("bind() failed");
			exit(1);
		}
	
	/*make the server to listen for incoming connections*/
		if(listen(rootSock,MAXPENDING)<0)
		{
			perror("listen() failed");
			exit(1);
		}

		FD_SET(rootSock, &master);
		fdmax = rootSock;
		sockets.push_back(rootSock);

		printf("Root Server started, listening on port %d\n", echoServPort);


		for(;;)
		{

			serv_set = master;
 			wait.tv_sec = 5;
			wait.tv_usec = 0;
			if(select(fdmax+1, &serv_set, NULL, NULL, &wait) == -1) //select used with 0 as the timeout, so it does not block
			{
    			perror("select() error");
    			exit(1);
			}
			
 
			for(int i = 0; i <= fdmax; i++) //tests to see which socket has activity
			{
   				 if(FD_ISSET(i, &serv_set))
   			 	{ 
    					if(i == rootSock) //if socket is listening socket, accept connection
    				 	{
        				
       						clntLen=sizeof(echoClntAddr);
						if((newfd=accept(rootSock,(struct sockaddr *)&echoClntAddr,&clntLen))<0)
						{
							perror("accept() failed");
							exit(1);
						}
						else
						{
    							printf("Handling client: %s port: %d\n",inet_ntoa(echoClntAddr.sin_addr), ntohs(echoClntAddr.sin_port));
							sockets.push_back(newfd); //new socket is added to vector
							if(newfd > fdmax)
					
    							fdmax = newfd;
					
						}
					}
				}
			}


			cout << "sending time beacon...\n";
			for(int i=1;i<sockets.size();i++) //iterates through sockets vector and sends time to each
			{
						gettimeofday(&now_server,NULL);
						sprintf(echoBuffer,"%u.%u",now_server.tv_sec,now_server.tv_usec);
             					if(send(sockets[i],echoBuffer,RCVBUFFERSIZE,0)!=RCVBUFFERSIZE)
						{
							perror("send() failed");
							exit(1);
						}
			}

		}
			

		
	}

	else if(type == "slave")
	{

		cout << "This will be a slave server, please input port number: ";
		cin >> echoServPort;
		cout << "Root server name: ";
		cin >> sserver_name;
		cout << "Root server port: ";
		cin >> sserver_port;
		
		const char* server_name = sserver_name.c_str();
		const char* server_port = sserver_port.c_str();

		memset(&servaddr, 0, sizeof servaddr);
		servaddr.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
		servaddr.ai_socktype = SOCK_STREAM;

		if ((rv = getaddrinfo(server_name, server_port, &servaddr, &servinfo)) != 0) {
    		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    		exit(1);
		}

		//connects to root socket
		for(p = servinfo; p != NULL; p = p->ai_next) {
    			if ((midSock = socket(p->ai_family, p->ai_socktype,
          		 p->ai_protocol)) == -1) {
       			 perror("socket");
       			 exit(0);
   		 	}

   			 if (connect(midSock, p->ai_addr, p->ai_addrlen) == -1) {
       			 close(slaveSock);
      		 	 perror("connect");
       		 	exit(0);
   		 	}

    			break; // if we get here, we must have connected successfully
   
		}

		cout << "Connected to server "<< server_name << ":" << server_port << endl;

		if((rootSock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
		{
			perror("socket() failed");
			exit(1);
		}
	
	/*construct the local address for the server*/
		memset(&echoServAddr,0,sizeof(echoServAddr));
		echoServAddr.sin_family=AF_INET;
		echoServAddr.sin_port=htons(echoServPort);
		echoServAddr.sin_addr.s_addr=htonl(INADDR_ANY);


	/*bind the server socket*/
		if(bind(rootSock,(struct sockaddr *)&echoServAddr,sizeof(echoServAddr))<0)
		{
			perror("bind() failed");
			exit(1);
		}
	
	/*make the server to listen for incoming connections*/
		if(listen(rootSock,MAXPENDING)<0)
		{
			perror("listen() failed");
			exit(1);
		}

		FD_SET(rootSock, &master);
		fdmax = rootSock;
		sockets.push_back(rootSock);

		printf("Slave server started, listening on port %d\n", echoServPort);


		for(;;)
		{
			serv_set = master;
 			wait.tv_sec = 0;
			wait.tv_usec = 0;
			if(select(fdmax+1, &serv_set, NULL, NULL, &wait) == -1)
			{
    			perror("select() error");
    			exit(1);
			}
		
 			//same as root server, waits for new connections using select
			for(int i = 0; i <= fdmax; i++)
			{
   				 if(FD_ISSET(i, &serv_set))
   			 	{
    					if(i == rootSock)
    				 	{
        				
       						clntLen=sizeof(echoClntAddr);
						if((newfd=accept(rootSock,(struct sockaddr *)&echoClntAddr,&clntLen))<0)
						{
							perror("accept() failed");
							exit(1);
						}
						else
						{
    							printf("Handling client: %s port: %d\n",inet_ntoa(echoClntAddr.sin_addr), ntohs(echoClntAddr.sin_port));
							sockets.push_back(newfd); 
							if(newfd > fdmax)
				
    							fdmax = newfd;
					
						}
					}
				}
			}


			//converts between char*, time and double, in order to calculate receive time - transmit time
			if((recvMsgSize=recv(midSock,echoBuffer,RCVBUFFERSIZE,0))<0)
			{
				perror("recv() failed");
				exit(1);
			}
			double x = atof(echoBuffer);
			double y;
			double z;
			gettimeofday(&now_server,NULL);
			printf("receive time: %s     server time: %u.%u\n",echoBuffer, now_server.tv_sec,now_server.tv_usec);
			char tempbuffer[RCVBUFFERSIZE];
			sprintf(tempbuffer,"%u.%u",now_server.tv_sec,now_server.tv_usec);
			y = atof(tempbuffer);

			for(int i=1;i<sockets.size();i++)
			{
					
					gettimeofday(&transmit,NULL);
					sprintf(tempbuffer,"%u.%u",transmit.tv_sec,transmit.tv_usec);
					z = atof(tempbuffer);
					x += (z-y); //adds to original beacon and sends to children
					sprintf(echoBuffer,"%f",x);
             				if(send(sockets[i],echoBuffer,RCVBUFFERSIZE,0)!=RCVBUFFERSIZE)
					{
						perror("send() failed");
						exit(1);
					}
			}
		}

	}

	else
	{
		cout << "Server type invalid\n";
		exit (0);
	}

	return 0;


}

	
	
