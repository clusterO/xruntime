#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "net.h"

#define BACKLOG 10

void *getInAddr(struct sockaddr *sa) 
{
    if(sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int getSocketListner(char *port) 
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;    
    }

    //Go through interfaces
    for(p = servinfo; p != NULL; p = p->ai_next) { 
        //Make a socket of this one
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocole)) ==-1) {
            continue;
        }
        
        //Check if addr already in use
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            close(sockfd);
            freeaddrinfo(servinfo);
            return -2;
        }
        
        //Bind this socket to this local IP addr
        if(bind(sockfd, p->ai_addr, p->addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);
    
    if(p == NULL) {
        fprintf(stderr, "webserver: failed to find local address\n");
        return -3;
    }

    if(listen(sockfd, BACKLOG)  == -1) {
        close(sockfd);
        return -4;
    }

    return sockfd;
}
