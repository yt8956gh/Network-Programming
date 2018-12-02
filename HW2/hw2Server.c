#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT "9527"
#define BUFFSIZE 8192
#define on 1
#define off 0



struct client
{       
    char username[100];
    pid_t pid;
    struct client *next;
};

typedef struct client ClientNode;


ClientNode *header=NULL;

pthread_mutex_t mutex;



ClientNode* init_node(ClientNode *node)
{
    if(node==NULL)
    {
        node = (ClientNode*)malloc(sizeof(ClientNode));
        node->next = NULL;
    }

    return node;
}

void add(ClientNode *added)
{
    if(header!=NULL)
    {
        added->next = header->next;
        header->next = added;
    }
}

void socketHandle(void *connectFdPtr)
{
    int fd = *((int*)connectFdPtr);
    int openfd=0;
    char	buff[BUFFSIZE+1],command[BUFFSIZE+1],tmp[BUFFSIZE+1];
    char *ptr=NULL,*delim = "\n";
    ssize_t  ret;
    ClientNode *NewNode=NULL,*nodeptr;


    printf("PID:%d\n",getpid());

    if( (ret=read(fd, buff, sizeof(buff))) > 0)
    {
        printf("Client's Username: %s\n",buff);
    }
    else
    {
        fprintf(stderr,"Miss Username\n");
        exit(-1);
    }

    NewNode = init_node(NewNode);
    NewNode->pid = getpid();
    memcpy(NewNode->username,buff,sizeof(buff));

    pthread_mutex_lock(&mutex);

    add(NewNode);

    pthread_mutex_unlock(&mutex);

    nodeptr = NewNode;

    printf("Username:%s\nPID:%d\n",nodeptr->username,nodeptr->pid);


    while((ret=recv(fd, buff, sizeof(buff),0))>0)
    {
        printf("%ld:%s\n",strlen(buff),buff);

        memcpy(command,buff,strlen(buff));

        if(!strncmp(command,"list\n\0",6))
        {
            printf("list all username\n");

            nodeptr = header->next;

            while(nodeptr!=NULL)
            {
                printf("Username:%s\nPID:%d\n",nodeptr->username,nodeptr->pid);
                nodeptr = nodeptr->next;
            }

        }
        else if(!strncmp(command,"file\n\0",6))
        {
            strncpy(tmp,"who will recieve the transfered file ?\n\0",42);

            send(fd,tmp,strlen(tmp),0);
        }



    }


    close(fd);
    return;

    /*
    if(openfd==-1) write(fd, "Failed to open file", 19);

    while( (ret=read(openfd,buff,BUFFSIZE)) > 0)
    {
        write(fd,buff,ret);
    }
  
    close(fd);*/
}





int main()
{
	
    int listenfd, connfd, yes=1;
    socklen_t addr_size;
    struct sockaddr_in ClientInfo,servaddr;
    struct addrinfo hints,*res;
    char tmpstr[BUFFSIZE];
    pid_t childPID;
    pthread_t threads;
    
    header = init_node(header);

    if(header!=NULL)
    {
        printf("header Node Initialization Successfully\n");
    }
    else
    {
        printf("Fail to initialize user list\n");
        exit(-1);
    }
    

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL,PORT,&hints,&res);

    if( (listenfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol)) <0) // 0 for any
    {
        perror("Socket Error:");
        exit(-1);
    } 


    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if( (bind(listenfd, res->ai_addr, res->ai_addrlen)) <0) // link specified port to kernel
    {
        perror("Bind Error:");
        exit(-1);
    }

    listen(listenfd, 1024); //max connection 



    //avoid Zombie Process in the way of notifying kernel that child will be recover by kernel not parent 
    signal(SIGCHLD,SIG_IGN);

    while(1){ 

        memset(&ClientInfo,0,sizeof(ClientInfo));
        addr_size = sizeof(ClientInfo);

        //accept 要塞入 struct sockaddr_in
        connfd = accept(listenfd, (struct sockaddr *) &ClientInfo, &addr_size); //abandon client's info

        if(connfd==-1) //accept fail
        {
            perror("Accept");
            continue;
        }
        else //printf client's info 
        {
            inet_ntop(AF_INET, &(ClientInfo.sin_addr), tmpstr, INET_ADDRSTRLEN);
            printf("[Server's info]:accept from port:%d IP:%s\n",ntohs(ClientInfo.sin_port),tmpstr);
        }

        if( (childPID=fork()) ==0)//child
        {
            close(listenfd); 
            pthread_create(&threads,NULL,(void *)socketHandle,&connfd);
            pthread_join(threads,NULL);
              
            exit(0);
        }
        else close(connfd);
    }

    return 0;
}
