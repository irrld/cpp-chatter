#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <string>
#include <iostream>
#include <thread>

#define MAX 1500
#define PORT 8888
#define SA struct sockaddr

void doInput(int sockfd)
{
    while (true)
    {
        char *buffer = new char[MAX];
        read(sockfd, buffer, MAX);
        std::cout << buffer << std::endl;
    }
}
void doOutput(int sockfd)
{
    int n = 0;
    while (true)
    {
        char *buffer = new char[MAX];
        n = 2;
        while ((buffer[n++] = getchar()) != '\n')
            ;
        buffer[0] = 0x02;
        buffer[1] = n;
        buffer[n - 1] = '\0';
        write(sockfd, buffer, n);
        delete buffer;
    }
}
void strinsert(char *dst, int len, const char *src, int offset, int finallen)
{
    char temp[finallen];

    strncpy(temp, dst + offset, strlen(dst + offset));
    strncpy(dst + offset, src, strlen(src));
    strcat(dst + offset + strlen(src), temp);
}
int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        std::cout << "Socket creation failed!\n";
        return 0;
    }
    else
    {
        std::cout << "Socket created!\n";
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        std::cout << "Connection failed try again later!\n";
        exit(0);
    }
    else
        std::cout << "Connected!\n";

    std::cout << "Enter username: ";
    char buffer_send[25];
    int n = 2;
    while (n < 25 && (buffer_send[n++] = getchar()) != '\n')
        ;
    buffer_send[0] = 0x01;
    buffer_send[1] = n;
    buffer_send[n - 1] = '\0';
    write(sockfd, buffer_send, n);

    std::thread thread_obj(doInput, sockfd);
    doOutput(sockfd);
}