#include <stdio.h>
#include <zconf.h>
#include <netinet/in.h>
#include <strings.h>
#include <memory.h>
#include <sys/wait.h>
#include <sys/epoll.h>

#define PROCESS_NUM 10
#define BUFFER_SIZE 128
#define EVENTS_NUM 10

void addfd(int epollfd, int fd) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void handle_events(struct epoll_event *events,int number,int epfd,int listenfd) {
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; i++) {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd) {
            //建立链接
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(struct sockaddr_in);
            int connfd = accept(listenfd, (struct sockaddr *) &client_addr, &len);
            addfd(epfd,connfd);
        }
        else if (events[i].data.fd & EPOLLIN) {
            memset(buf,'\0',BUFFER_SIZE);
            int count = recv(sockfd,buf,BUFFER_SIZE,0);
            printf("get %d bytes of content: %s\n", count, buf);
        }
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr,client_addr;
    int sockfd = socket(AF_INET,SOCK_STREAM,0); //面向tcp的socket
    bzero(&server_addr,sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET; //ipv4地址格式
    server_addr.sin_port = htons(8080); //绑定80端口
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //将socket和地址绑定
    bind(sockfd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));

    //sockfd进入到监听状态,等待客户端链接
    listen(sockfd,5);

    struct epoll_event events[EVENTS_NUM];
    struct epoll_event ev;
    //创建10个线程来准备接受链接

    for (int i = 0; i < PROCESS_NUM; i++) {
        int pid = fork();
        if (pid == 0) {
            //子进程
            int epfd = epoll_create(10);
            bzero(&ev,sizeof(struct epoll_event));
            ev.data.fd = sockfd;
//            ev.events = EPOLLIN;
            ev.events = EPOLLIN | EPOLLEXCLUSIVE; //避免惊群的影响
            epoll_ctl(epfd,EPOLL_CTL_ADD,ev.data.fd,&ev);
            while (1) {
                int num;
                num = epoll_wait(epfd,events,EVENTS_NUM,-1);
                printf("handle events\n");
                sleep(3);
                handle_events(events,num,epfd,sockfd);
            }
        }
    }
    int status;
    wait(&status);
    return 0;
}
