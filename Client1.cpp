// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
int main(int argc,char *argv[])
{ 
	WSADATA wsa; 
	SOCKET sockFD; 

	WSAStartup(MAKEWORD(2,2),&wsa); 
	sockFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); 
	struct sockaddr_in server; 
	server.sin_family = AF_INET; 
	server.sin_port = htons(9000); 
	server.sin_addr.s_addr=inet_addr("127.0.0.1"); 
	int err=connect(sockFD,(struct sockaddr *)&server,sizeof(server)); 
	if (err <0)
	{
		printf("\r\nConnect error!");
		return -1;
	}
	else
		{
			int command=0;
			char command1[6]="reset",command2[6]="aduse";
			printf("Connect is ok\n");
			printf("please input command:\n");
			printf("\n\r 1- Reset computer");
			printf("\n\r 2- Create a Admin User");
			printf("\n\r 3- Exit\n");
			printf("\n-------Please input Number:");
			command=getchar();
			switch (command)
			{
				case '1':
					send(sockFD,command1,strlen(command1),0);
						break;
				case '2':
					send(sockFD,command2,strlen(command2),0);
						break;
				case '3':	break;
			}
		}
	closesocket(sockFD); 
	WSACleanup();
	return 0; 
}

