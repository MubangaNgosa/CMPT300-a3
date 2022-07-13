#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "list.h"
#include <netdb.h>
#include <sys/time.h>
#define STDIN 0


/*
 * Initialize global vars
 */

pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
bool loopSwitch = 1;
char mainBuffer[4000];
int encryption_key = 1;

/* Define thread parameters*/
struct threadParameters {
    List *outputList;
    List *inputList;
    int rSocket;
    int sSocket;
    struct sockaddr_storage rAddress;
    struct addrinfo *sender_servinfo, *sender_p, *receiver_p; // referenced from other code
};

/* Sender Function */
void *sendText(void * ptr) {
    struct threadParameters *params = ptr;
    int i;
    while(loopSwitch) {
        if(List_count(params->outputList)) {
            char * msg = List_remove(params->outputList);
            // encryption
            for(i = 0; i < strlen(msg); i++) {
                if(msg[i] != '\n' && msg[i] != '\0') {
                    msg[i] += encryption_key;
                }
            }
            int numbytes;
            if ((numbytes = sendto(params->sSocket, msg, strlen(msg), 0,(params->sender_p)->ai_addr, (params->sender_p)->ai_addrlen)) == -1) {
                perror("talker: sendto");
                exit(1);
            }
            for(i = 0; i < strlen(msg); i++) {
                if(msg[i] != '\n' && msg[i] != '\0') {
                    msg[i] -= encryption_key;
                }
            }
            if(strcmp(msg, "!exit\n") == 0) {
                loopSwitch = 0;
            }

            if(strcmp(msg, "!status\n") == 0) {
                char buf[4000];
                socklen_t addr_len;
                int numbytes2;
                addr_len = (params->sender_p)->ai_addrlen;
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                if (setsockopt(params->sSocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
                    perror("Error");
                }
                if ((numbytes2 = recvfrom(params->sSocket, buf, 4000 , 0,(params->sender_p)->ai_addr, &addr_len)) == -1) {
                    strcpy(buf, "Offline");
                    printf("Offline\n");
                }
                if(strcmp(buf, "Online") > 0) {
                    buf[numbytes2] = '\0';
                    printf("%s\n", buf);
                }

            }
        }
    }
    pthread_exit(NULL);

}

/* Receiver Function */
void *receiveText(void *pointer) {
    struct threadParameters *params = pointer;
    int i;
    while(loopSwitch) {
        char rBuffer[4000];
        socklen_t addr_len;
        int rBytes;
        addr_len = sizeof params->rAddress;
        if ((rBytes = recvfrom(params-> rSocket, rBuffer, 4000 , 0,(struct sockaddr *)&(params->rAddress), &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        rBuffer[rBytes] = '\0';
        // decryption
        for(i = 0; i < strlen(rBuffer); i++) {
            if(rBuffer[i] != '\n' && rBuffer[i] != '\0') {
                rBuffer[i] -= encryption_key;
            }
        }
        List_add(params->inputList, (char *) rBuffer);
    }
    pthread_exit(NULL);
}

void *printToTerminal(void * ptr) {
    struct threadParameters *params = ptr;
    while(loopSwitch) {
        if(List_count(params->inputList)) {
            char *text = List_remove(params->inputList);
            printf("%s",text);
            fflush(stdout);

            if(strcmp(text,"!exit\n") == 0) {
                loopSwitch = 0;
            }
        }
    }
    pthread_exit(NULL);

}

void *input_msg(void * ptr) {
    struct threadParameters *params = ptr;
    printf("Welcome to lets-talk! Please type your message now\n");
    while(loopSwitch) {
        if(fgets(mainBuffer, 4000, stdin)){
            List_add(params->outputList, (char *)mainBuffer);
        }
    }
    pthread_exit(NULL);
}

int main (int argc, char ** argv)
{
    //RECEIVER
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    struct sockaddr_storage their_addr;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        freeaddrinfo(servinfo);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        freeaddrinfo(servinfo);
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    //SENDER
    int sender_sockfd;
    struct addrinfo sender_hints, *sender_servinfo, *sender_p;
    int sender_rv;

    memset(&sender_hints, 0, sizeof sender_hints);
    sender_hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    sender_hints.ai_socktype = SOCK_DGRAM;

    if ((sender_rv = getaddrinfo(argv[2], argv[3], &sender_hints, &sender_servinfo)) != 0) {
        freeaddrinfo(sender_servinfo);
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sender_rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(sender_p = sender_servinfo; sender_p != NULL; sender_p = sender_p->ai_next) {
        if ((sender_sockfd = socket(sender_p->ai_family, sender_p->ai_socktype,
                                    sender_p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (sender_p == NULL) {
        freeaddrinfo(sender_servinfo);
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    List * send_list;
    send_list = List_create();

    List * receive_list;
    receive_list = List_create();

    struct threadParameters params;
    params.receiver_p = p;
    params.rAddress = their_addr;
    params. rSocket = sockfd;
    params.sSocket = sender_sockfd;
    params.sender_servinfo = sender_servinfo;
    params.sender_p = sender_p;
    params.outputList = send_list;
    params.inputList = receive_list;

    pthread_t receiving_thr, printer_thr, sending_thr, keyboard_thr;

    //create threads for receiving and printing messages
    pthread_create(&keyboard_thr, NULL, input_msg, &params);
    pthread_create(&sending_thr, NULL, sendText, &params);
    pthread_create(&receiving_thr, NULL, receiveText, &params);
    pthread_create(&printer_thr, NULL, printToTerminal, &params);

    while(loopSwitch);
    pthread_cancel(keyboard_thr);
    pthread_cancel(sending_thr);
    pthread_cancel(receiving_thr);
    pthread_cancel(printer_thr);

    pthread_join(keyboard_thr, NULL);
    pthread_join(sending_thr, NULL);
    pthread_join(receiving_thr, NULL);
    pthread_join(printer_thr, NULL);

    freeaddrinfo(servinfo);
    freeaddrinfo(sender_servinfo);

    pthread_exit(0);
}