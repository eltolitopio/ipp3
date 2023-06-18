#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define MYSRV_COMMDOMAIN AF_INET
#define MYSRV_PORT htons(1050)
#define MYSRV_PROTOCOL IPPROTO_TCP

int main(int argc, char **argv) {
    int s = socket(MYSRV_COMMDOMAIN, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
        perror("socket creation failed");
        return errno;
    }

    struct sockaddr_in sa_in;
    char in_host[32] = {0};
    printf("Enter host name/ip: ");
    scanf("%s", in_host);

    struct hostent *hst = gethostbyname(in_host);
    sa_in.sin_family = MYSRV_COMMDOMAIN;
    sa_in.sin_port = MYSRV_PORT;
    memcpy((char *)&(sa_in.sin_addr), hst->h_addr, hst->h_length);

    if (connect(s, (struct sockaddr *)&sa_in, sizeof(sa_in)) == -1) {
        perror("Connection failed");
        goto done;
    }

    do {
        ssize_t bytes_sent;
        int control_word = 0;
        scanf("%d", &control_word);

        if ((bytes_sent = send(s, &control_word, sizeof(control_word), 0)) == -1) {
            perror("Sending control word failed");
            goto done;
        }

        printf("Sent %u bytes\n", bytes_sent);

        if (control_word == 0)
            break;
    } while (1);

done:
    if (shutdown(s, SHUT_RDWR) == -1) {
        perror("Socket shutdown failed");
    }

    if (close(s) == -1) {
        perror("Closing socket fd failed");
    }

    return errno;
}
