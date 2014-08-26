#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libnet.h>
#include <pcap.h>
#include <pthread.h>


#define IP_RF 0x8000		
#define IP_DF 0x4000		
#define IP_MF 0x2000		
#define IP_OFFMASK 0x1fff

#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)

#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
#define TH_OFF(th)		(((th)->th_offx2 & 0xf0) >> 4)

/* ֡ͷ��ʽ */
struct sniff_ethernet {
	u_char ether_dhost[ETHER_ADDR_LEN]; /* Ŀ��MAC��ַ */
	u_char ether_shost[ETHER_ADDR_LEN]; /* ԴMAC��ַ */
	u_short ether_type; /* �ϲ�Э������ */
};

/* IP ͷ��ʽ */
struct sniff_ip {
	u_char ip_vhl;		/* ��4λΪIPЭ��汾����4λΪIPͷ���� */
	u_char ip_tos;		/* �������� */
	u_short ip_len;		/* ���� */
	u_short ip_id;		/* ��Ƭ��ʶ */
	u_short ip_off;		/* ��3λ��Ƭ��ʶ����13λΪ��Ƭƫ���� */
	u_char ip_ttl;		/* IP������������ */
	u_char ip_p;		/* �ϲ�Э�����ͱ�ʶ */
	u_short ip_sum;		/* У��� */
	struct in_addr ip_src,ip_dst; /* IPԴ��ַ��IPĿ���ַ */
};
/* ICMP ͷ��ʽ */
struct sniff_icmp
{
    u_int8_t icmp_type;  /* ICMP���� */
    u_int8_t icmp_code;  /* ICMP���� */
    u_int16_t icmp_sum;  /* У��� */
	//u_int16_t icmp_id;	 /* ��ʶ */
	//u_int16_t icmp_seq;	 /* ���к� */
};
/* UDP ͷ��ʽ */
struct sniff_udp
{
    u_int16_t udp_sport;  /* Դ�˿ں� */
    u_int16_t udp_dport;  /* Ŀ�Ķ˿ں� */
    u_int16_t udp_len;    /* ���� */
    u_int16_t udp_sum;	  /* У��� */
};


#define UDP_SCAN 1

#define UNKNOWN 	1
#define OPEN 		2
#define CLOSE		3
/* ɨ����Ϣ�ṹ */
struct scaninfo_struct{
	int scan_type;
	char interface[32]; 
	struct in_addr ipaddr;   
	char ipaddr_string[32];
	int startport;
	int endport;
	int portnum;

	pthread_cond_t *cond;

	int *portstatus;
	int alreadyscan;
};
 /* UDP̽�ⱨ�ķ��ͺ��� */
 void send_udp(struct scaninfo_struct *pscaninfo){
	int i;
	for (i = pscaninfo->startport; i <= pscaninfo->endport; i++)
	{
		usleep(1000000); //����Linuxϵͳ������ICMPĿ�겻�ɴﱨ�ĵķ������ʣ�����Ӧ�ø����������UDP̽�ⱨ�ĵķ������ʡ�
		struct sockaddr_in addr;
		int sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == -1)
		{
			printf("create socket error! \n");
			return ;
		}

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(i);
		inet_pton(AF_INET, pscaninfo->ipaddr_string, &addr.sin_addr);
		
		int retval = sendto(sock,NULL,0,0,(const struct sockaddr *)(&addr), sizeof(addr));//UDP���ķ���
		if (retval<0) 
			printf("Send message to Host Failed !");
		close(sock);	
	}
}

//ICMP���Ĵ�����
void packet_handler(u_char *args,const struct pcap_pkthdr *header,const u_char *packet)
{	
	struct scaninfo_struct *pscaninfo = (struct scaninfo_struct *)args;

	const int SIZE_ETHERNET = 14;

	const struct sniff_ethernet *ethernet; //֡ͷָ��
	const struct sniff_ip *ip;				//IPͷָ��
	const struct sniff_icmp *icmp;			//ICMPָ��
	const struct sniff_ip *ipw;				//�������ı��ĵ�IPͷָ��
	const struct sniff_udp *udp;			//UDPͷָ��

	u_int size_ip;
	u_int size_icmp;
	u_int size_ipw;

	ethernet = (struct sniff_ethernet *)(packet);
	ip = (struct sniff_ip *)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip) * 4;
	if (size_ip < 20) return;
	//�ж�ICMP�����Ƿ�ΪĿ�Ĳ��ɴ�����Ʊ���
	icmp = (struct sniff_icmp *)(packet+SIZE_ETHERNET+size_ip);
	if (icmp->icmp_type != 3) return;
	
	ipw = (struct sniff_ip *)(packet+SIZE_ETHERNET+size_ip+8);
	size_ipw = IP_HL(ipw) * 4;
	if (size_ipw < 20) return;
	//��ȡ�������ı��ĵ�Ŀ��IP��ַ
	struct in_addr ip_check = ipw->ip_dst;
	//��ȡĿ�Ĳ��ɴ�����Ʊ��ĵ�UDP�ײ�
	udp = (struct sniff_udp *)(packet+SIZE_ETHERNET+size_ip+8+size_ipw);
	//��ȡUDP�ײ���Ŀ�Ķ˿ں�
	int dstport = ntohs(udp->udp_dport);
	
	if (ip_check.s_addr == (pscaninfo->ipaddr).s_addr)
	{//�ж�ICMP���ɴﱨ�ĵĲ���ĵ�Ŀ��IP��ַ�Ƿ���UDP���ͱ���һ��
		if (icmp->icmp_code == 3)//�ж�ICMP�����Ƿ�Ϊ�˿ڲ��ɴﱨ��
			pscaninfo->portstatus[dstport - pscaninfo->startport] = CLOSE;
		else
			pscaninfo->portstatus[dstport - pscaninfo->startport] = UNKNOWN;
		pscaninfo->alreadyscan++;
	}

	if (pscaninfo->alreadyscan >= pscaninfo->portnum)
		pthread_cond_signal(pscaninfo->cond);
}
//ICMP���Ĳ����̺߳���
void *receivethread(void *args)
{
	struct scaninfo_struct *pscaninfo = (struct scaninfo_struct *)args;

	bpf_u_int32 net;
	bpf_u_int32 mask;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_lookupnet(pscaninfo->interface, &net, &mask, errbuf);

	pcap_t *handle;
	handle = pcap_open_live(pscaninfo->interface, 100, 1, 0, errbuf);
	if (handle == NULL)
	{
		printf("pcap open device failure \n");
		return NULL;
	}

	struct bpf_program fp;
	char filter[20] = "icmp";	//���˺�����ֻ����ICMP����

	int retval = 0;
	retval = pcap_compile(handle, &fp, filter, 0, net);
	if (retval == -1)		return NULL;
	retval = pcap_setfilter(handle, &fp);
	if (retval == -1) return NULL;

	pcap_loop(handle, 0, packet_handler, (u_char *)pscaninfo);

	return NULL;
}
//�����н�������
int parse_scanpara(int argc, char *argv[],struct scaninfo_struct *pparse_result){
	if (argc != 6) {
		printf("The count of parameters error!\n");
		return 1;
	}
	if (!strcmp(argv[1],"UDP_SCAN"))
		pparse_result->scan_type = UDP_SCAN;
	else {
		printf("An Unsupported scan type!\n");
		return 1;
	}

	strcpy(pparse_result->interface, argv[2]);
	strcpy(pparse_result->ipaddr_string,argv[3]);
	if (inet_aton(argv[3],&pparse_result->ipaddr) ==0 )
	{
		printf("IPaddr format error! please check it! \n");
		return 1;
	} 
	pparse_result->startport = atoi(argv[4]);
	pparse_result->endport = atoi(argv[5]);
	pparse_result->portnum = pparse_result->endport - pparse_result->startport + 1;
	return 0;

}
//ɨ����Ϣ�ṹ��ʼ������
void initial_portstatus(struct scaninfo_struct *pscaninfo){
	int i; 
	pscaninfo->portstatus = (int *) malloc(pscaninfo->portnum * 4);
	for (i = 0; i < pscaninfo->portnum; i++)
		pscaninfo->portstatus[i] = UNKNOWN;
	pscaninfo->alreadyscan = 0;
}
//ɨ�����������
void output_scanresult(struct scaninfo_struct scaninfo){
	int i;
	printf(" Scan result of the host(%s):\n", scaninfo.ipaddr_string);
	printf("    port               status\n");
	for (i = 0; i < scaninfo.portnum; i++) {
		if (scaninfo.portstatus[i] == OPEN)
			printf("	%d   		open\n",scaninfo.startport+i);
		else if (scaninfo.portstatus[i] == CLOSE)
			printf("	%d   		close\n",scaninfo.startport+i);
		else
			printf("	%d   		unknown\n",scaninfo.startport+i);
	}
}
//UDP�˿�ɨ�躯��
void udp_scan(struct scaninfo_struct *pscaninfo){

	pthread_t r_thread;

	pthread_cond_t cond;
	pthread_mutex_t mutex;

	/* ���ó�ʱʱ��ֵ */
	struct timespec to;
	struct timeval now;
	
	/* ������ʼ��  */
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pscaninfo->cond = &cond;

	/* ���������߳� */
	pthread_create(&r_thread, NULL, receivethread, (void *)(pscaninfo));
	/* ����UDP̽�ⱨ�� */
	send_udp(pscaninfo);
	
	/* ����ɨ�賬ʱʱ��  */
	gettimeofday(&now, NULL);
	to.tv_sec = now.tv_sec;
	to.tv_nsec = now.tv_usec * 1000;
	to.tv_sec += 1;
	pthread_cond_timedwait(&cond, &mutex, &to);
	
	/* �ͷ���Դ  */
	pthread_cancel(r_thread);
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
}

int main(int argc,char *argv[]){

	struct scaninfo_struct scaninfo;

	if (parse_scanpara(argc, argv,&scaninfo)) {
		printf("Usage %s UDP_SCAN interface IPaddr startport endport",argv[0]);
		exit(1);
	}
	initial_portstatus(&scaninfo);

	if (scaninfo.scan_type == UDP_SCAN)
	{
		udp_scan(&scaninfo);
	}
	else {
		printf("Unsupported scan type! \n");
		exit(1);
	}	 
	output_scanresult(scaninfo);	
}





