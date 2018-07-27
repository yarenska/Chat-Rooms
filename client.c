#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include<sys/ioctl.h>
//#include<iostream>
#include<errno.h>
#define SIZE sizeof(struct sockaddr_in)


int main() {
    int sockfd, nread;
    char buf[512], enter, resp;
    fd_set fds;
    char choice[2];
    choice[1] = '\0';
    char IP[20];
    char message[250];
    struct sockaddr_in server = { AF_INET, 2000 };
    printf("\n\n\n\n\nEnter IP address of the Server\n");
    scanf("%s%c", IP, &enter);
    server.sin_addr.s_addr = inet_addr(IP);
    char name[30];
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error creating SOCKET\n");
        return (0);
    }
    if (connect(sockfd, (struct sockaddr *) &server, SIZE) == -1) {
        printf("Connect failed\n");
        return (0);
    }
    
    printf("Enter your name: ");
    scanf("%s", name);
    
    printf("Enter a message (E to exit)\n");
    do {
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        FD_SET(0, &fds);
        /* Wait for some input. */
        select(sockfd + 1, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);
        
        /* If either device has some input,read it and copy it to the other. */
        if (FD_ISSET(sockfd, &fds)) {
            nread = recv(sockfd, buf, sizeof(buf), 0);
            
            /* If error or eof, terminate. */
            if (nread < 1) {
                close(sockfd);
                exit(0);
            }
            buf[nread] = 0;
            printf("%s", buf);
            if(buf[0] == '*'){
                scanf(" %c", &choice[0]);
                send(sockfd, choice, sizeof(choice), 0);
            } 
        }
        
        if (FD_ISSET(0, &fds)) {
            nread = read(0, buf, sizeof(buf));
            buf[nread] = 0;
            strcpy(message, name);
            strcat(message, ": ");
            strcat(message, buf);
            /* If error or eof, terminate. */
            if (nread < 1) {
                close(sockfd);
                exit(0);
            } else if ((buf[0] == 'e' || buf[0] == 'E') && nread == 2) {
                close(sockfd);
                exit(0);
            } else
                send(sockfd, message, sizeof(message), 0);
        }
    } while (1);
}
