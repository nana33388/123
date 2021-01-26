#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include "command.h"

int main()
{
	pid_t pid;
	int opt = true;
	int sockfd = 0, connfd = 0, client_socket[30];
	char buffer[PACKETSIZE];

	FILEs *fileptr = (FILEs *) mmap(NULL, sizeof(FILEs) * MAXFILE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	const char* AOS[] = {"chih", "nana", "shih"};
	const char* CSE[] = {"cc", "ss", "ee"};

	struct sockaddr_in serverInfo, clientInfo; //sockaddr_in把14個位元組拆分成sin_port, sin_addr和sin_zero
	socklen_t addrlen = 0;
	PACKET *PCKT = (PACKET *)malloc(sizeof(PACKET));//配置空間

	bzero(&serverInfo, sizeof(serverInfo));//初始化
	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;//每個ip都能連
	serverInfo.sin_port = htons(PORT);
	
	//建立socket
	if((sockfd = socket(AF_INET , SOCK_STREAM , 0)) < 0) //ipv4 TCP
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	//允許多人連線
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    } 

	//把自家地址邦在socket身上
	if(bind(sockfd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	printf("Listen on port: %d\n", PORT);

	//最多10人能連入server
	if(listen(sockfd, 10) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//當accept()被調用時，它會為該請求產生出一個新的Socket，並把這個請求從監聽隊列剔除掉。
	addrlen = sizeof(clientInfo);
	puts("Waiting for connections ..."); 

	while(true)
	{
		if ((connfd = accept(sockfd, (struct sockaddr *)&clientInfo, &addrlen))<0)   
        {   
        	perror("accept");   
            exit(EXIT_FAILURE);   
        }   
             
        printf("New connection , ip : %s , port : %d \n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
		pid = fork();
		if(pid == -1 || pid > 0)//父程序或程序錯誤(繼續等待吉他client連線)
		{
			close(connfd);
			continue;
        }
        else if(pid == 0)//子程序(負責處理client的指令)
        {
            while(true)
			{
				recv(connfd, buffer, PACKETSIZE, 0); //會把接收到的資料塞進buffer,並回傳接收到多少byte
				PCKT = (PACKET *)buffer;

				printf("\nClient傳來packet:\n");
				printf("Username: %s\n", PCKT->username);
				printf("ClientGroup: %s\n", PCKT->group);
				printf("Command: %s\n", PCKT->command);
				printf("Filename: %s\n", PCKT->filename);
				printf("Capability: %s\n", PCKT->capability);
				printf("Message: %s\n\n", PCKT->message);

				if(strcmp(PCKT->command, "exit") == 0)
					break;

				handlecmd(PCKT, AOS, CSE, fileptr);
				printf("\n傳送packet給client:\n");
				printf("Username: %s\n", PCKT->username);
				printf("ClientGroup: %s\n", PCKT->group);
				printf("Command: %s\n", PCKT->command);
				printf("Filename: %s\n", PCKT->filename);
				printf("Capability: %s\n", PCKT->capability);
				printf("Message: %s\n", PCKT->message);
				
				send(connfd, PCKT, PACKETSIZE, 0);
				bzero(PCKT, sizeof(PACKET));
				bzero(buffer, sizeof(PACKET));
			}
            break;
        }
	}
	
	close(connfd);
	close(sockfd);

}
