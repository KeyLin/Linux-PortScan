#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define SERVER_IP "192.168.174.129"
main()
{
	int s,len,port=60;
	struct sockaddr_in addr;
	int addr_len =sizeof(struct sockaddr_in);
	char buffer[256];
	/* ����socket*/
	if((s = socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket");
		exit(1);
	}
	/* ��дsockaddr_in*/
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	while(port<100){
	if((s = socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket");
		exit(1);
	}
	/* ��дsockaddr_in*/
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);
		bzero(buffer,sizeof(buffer));
		/* �ӱ�׼�����豸ȡ���ַ���*/
		len =read(STDIN_FILENO,buffer,sizeof(buffer));
		/* ���ַ������͸�server��*/
		sendto(s,buffer,len,0,(struct sockaddr *)&addr,addr_len);
		printf("send a udp packet to:%d\n",port);
		/* ����server�˷��ص��ַ���*/
		//len = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *)&addr,&addr_len);
		//printf("receive: %s",buffer);
		port++;
	}
}