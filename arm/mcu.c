#include <algo.h>

void track_send(char *ip,char *buf,int len){
	static int sockfd=0;
	int sendLen;
	struct sockaddr_in serv_addr;
	if(sockfd==0)
	{
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
			printf("socket创建出错！n");
			return;
			}
	}
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(1650);
	serv_addr.sin_addr.s_addr = inet_addr(ip);;
	bzero(&(serv_addr.sin_zero),8);

	if ((sendLen=sendto(sockfd, buf, len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) ==-1) {
		printf("send出错！");
		return;
		}
	printf("Send: %s \n",buf);
	//close(sockfd);
}

void mcu_send(char *ip,char *buf,int len){
	int sockfd, sendLen;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("socket创建出错！n");
		return;
		}
	unsigned long ul=1;
	int rm=ioctl(sockfd,FIONBIO,&ul); 
	if(rm==-1)
	{
		close(sockfd); 
		exit(0);
		return;   
	}
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(8081);
	serv_addr.sin_addr.s_addr = inet_addr(ip);;
	bzero(&(serv_addr.sin_zero),8);
	if (connect(sockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) == 0) {
		printf("connected！\n");
		return;
		}
	 if(errno!=EINPROGRESS) 
    	{
      		printf("cannot connect");
      		return;
    	}     
	struct timeval timeout; 
    fd_set r;          
    FD_ZERO(&r);
    FD_SET(sockfd,&r); 
    timeout.tv_sec=0;    
    timeout.tv_usec=50000;
    int retval = select(sockfd+1,NULL,&r,NULL,&timeout);
    if(retval==-1)
    {
      printf("select\n");
      return;
    }
    else if(retval == 0)
    {
      printf("timeout\n");
      return;
    }
    printf("connected\n");

    unsigned long ul1=0;
    rm=ioctl(sockfd,FIONBIO,(unsigned long*)&ul1); 
    if(rm==-1)
    {
      close(sockfd);
      return;      
    }
	if ((sendLen=send(sockfd, buf, len, 0)) ==-1) {
		printf("send出错！");
		return;
		}
	printf("Send: %s \n",buf);
	close(sockfd);
}

