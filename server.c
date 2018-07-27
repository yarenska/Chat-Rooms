#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>

#define SIZE sizeof(struct sockaddr_in)
#define MAX 10
#define true 1
#define false 0

struct ChatRoom {
    int isAvailable;
    int capacity;
    int client1;
    int client2;
    char name[8];
}typedef ChatRoom;

int client[MAX];
char ips[MAX][20];
int ActiveClients = 0;

void findMax(int *maxfd) {
    int i;
    *maxfd = client[0];
    for (i = 1; i < MAX; i++)
        if (client[i] > *maxfd)
            *maxfd = client[i];
}

void getRooms(char buf[], ChatRoom chats[]){
    strcpy(buf, "");
    char roomNumber[2];
    char availableSlot[2];
    roomNumber[1] = '\0';
    availableSlot[1] = '\0';
    strcat(buf, "*");
    
    for(int i = 0; i < 5; i++){
        if(chats[i].isAvailable == true)
        { 
            roomNumber[0] = ('1' + i);
            availableSlot[0] = ('0' + chats[i].capacity);
            strcat(buf, "Room ");
            strcat(buf, roomNumber);
            strcat(buf, " Available Slots:");
            strcat(buf, availableSlot);
            strcat(buf, "\n");
        }
    }
    strcat(buf, "Select your choice: ");
}


int main() {
    ChatRoom chats[5];
    for(int i = 0; i < 5; i++){
        chats[i].isAvailable = true;
        chats[i].capacity = 2;
        chats[i].client1 = 0;
        chats[i].client2 = 0;
    }
    strcpy(chats[0].name ,"Room 1\0");
    strcpy(chats[1].name ,"Room 2\0");
    strcpy(chats[2].name ,"Room 3\0");
    strcpy(chats[3].name ,"Room 4\0");
    strcpy(chats[4].name ,"Room 5\0");
    
    int sockfd, maxfd, nread, found, i, j;
    char choice[2];
    char roomLog[530];
    choice[1] = '\0';
    char buf[512];
    fd_set fds;
    struct sockaddr_in server = { AF_INET, 2000, INADDR_ANY };
    struct sockaddr_in myClient;
    int sin_size = sizeof(struct sockaddr_in);
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Error creating SOCKET\n");
        return (0);
    }
    if (bind(sockfd, (struct sockaddr *) &server, SIZE) == -1) {
        printf("bind failed\n");
        return (0);
    }
    if (listen(sockfd, 5) == -1) {
        printf("listen failed\n");
        return (0);
    }
    
    findMax(&maxfd);
    
    for (;;) {
        findMax(&maxfd);
        maxfd = (maxfd > sockfd ? maxfd : sockfd) + 1;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        for (i = 0; i < MAX; i++)
            if (client[i] != 0)
                FD_SET(client[i], &fds);
            
            /* Wait for some input or connection request. */
            select(maxfd, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);
        
        /*If one of the clients has some input, read and send it to all others.*/
        for (i = 0; i < MAX; i++)
            if (FD_ISSET(client[i], &fds)) {
                nread = recv(client[i], buf, sizeof(buf), 0);
                strcpy(roomLog, "");
                
                /* If error or eof, terminate the connection */
                if (nread < 1) {
                    //  Open the room again
                    for(int cnt = 0; cnt < MAX; cnt++)
                    {
                        if(client[i] == chats[cnt].client1)
                        {
                            chats[cnt].client1 = 0;
                            chats[cnt].capacity++;
                            
                            if(chats[cnt].capacity > 2)
                                chats[cnt].capacity = 2;
                            if(chats[cnt].capacity > 0)
                                chats[cnt].isAvailable = true;
                            if(chats[cnt].client2 != 0)
                            {
                                strcpy(buf, "Partner disconnected.\n\0");
                                send(chats[cnt].client2, buf, sizeof(buf), 0);
                            }
                            break;
                        }
                        else if(client[i] == chats[cnt].client2)
                        {
                            chats[cnt].client2 = 0;
                            chats[cnt].capacity++;
                            
                            if(chats[cnt].capacity > 2)
                                chats[cnt].capacity = 2;
                            if(chats[cnt].capacity > 0)
                                chats[cnt].isAvailable = true;
                            if(chats[cnt].client1 != 0)
                            {
                                strcpy(buf, "Partner disconnected.\n\0");
                                send(chats[cnt].client1, buf, sizeof(buf), 0);
                            }
                            break;
                        }
                        
                        
                    }
                    //
                    close(client[i]);
                    client[i] = 0;
                    printf("%s disconnected.\n", ips[i]);
                    ActiveClients--;
                    
                    
                } else
                    /* broadcast the message to appropriate room */
                    for(int cnt = 0; cnt < MAX; cnt++)
                    {
                        if(client[i] == chats[cnt].client1)
                        {
                            openlog("Chats", LOG_PID|LOG_CONS, LOG_USER);
                            strcat(roomLog, chats[cnt].name);
                            strcat(roomLog, ":: ");
                            strcat(roomLog, buf);
                            syslog(LOG_INFO, roomLog);
                            send(chats[cnt].client2, buf, sizeof(buf), 0);
                            break;
                        }
                        else if(client[i] == chats[cnt].client2)
                        {
                            openlog("Chats", LOG_PID|LOG_CONS, LOG_USER);
                            strcat(roomLog, chats[cnt].name);
                            strcat(roomLog, ":: ");
                            strcat(roomLog, buf);
                            syslog(LOG_INFO, roomLog);
                            send(chats[cnt].client1, buf, sizeof(buf), 0); 
                            break;
                        }
                    }
            }
            
            
            /* if there is a request for a new connection */
            if (FD_ISSET(sockfd, &fds)) {
                /* If no of active clients is less than MAX accept the request */
                if (ActiveClients < MAX) {
                    found = 0;
                    for (i = 0; i < MAX && !found; i++)
                        if (client[i] == 0) {
                            client[i] = accept(sockfd, (struct sockaddr_in *)&myClient, &sin_size);
                            
                            strcpy(ips[i], inet_ntoa(myClient.sin_addr));
                            printf("New client connected. IP is: %s\n", inet_ntoa(myClient.sin_addr));
                            found = 1;
                            ActiveClients++;
                            
                            //
                            getRooms(buf,chats);		  
                            send(client[i], buf, sizeof(buf), 0);
                            recv(client[i], choice, sizeof(choice), 0);
                            // 
                            
                            int roomNumber = choice[0] - '0' - 1;
                            printf("%d is reserved.\n", roomNumber + 1);
                            
                            // Add the client to the room
                            if(chats[roomNumber].capacity != 0)
                            {
                                if(chats[roomNumber].client1 == 0)
                                {
                                    chats[roomNumber].client1 = client[i];
                                    chats[roomNumber].capacity--;
                                    // Warn the person inside the room
                                    if(chats[roomNumber].client2 != 0)
                                    {
                                        strcpy(buf, "A user connected.\n\0");
                                        send(chats[roomNumber].client2, buf, sizeof(buf), 0);
                                    }
                                }
                                else if(chats[roomNumber].client2 == 0)
                                {
                                    chats[roomNumber].client2 = client[i];
                                    chats[roomNumber].capacity--;
                                    // Warn the person inside the room
                                    if(chats[roomNumber].client1 != 0)
                                    {
                                        strcpy(buf, "A user connected.\n\0");
                                        send(chats[roomNumber].client1, buf, sizeof(buf), 0);
                                    }
                                }
                                if(chats[roomNumber].capacity == 0)
                                    chats[roomNumber].isAvailable = false;
                            }
                      }
                }
            }
    } // End of for ever loop
    closelog();
}
