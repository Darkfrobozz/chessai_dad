// Client side C program to demonstrate Socket
// programming
#include "client.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#define PORT 8080

char buffer[1024] = { 0 };
char *command = buffer;

struct sockaddr_in serv_addr;
int client_fd;

int client_listen() {
    int valread;
    valread = read(client_fd, buffer,
                   1024 - 1); // subtract 1 for the null
    if (valread < 0) {
        printf("failed");
        return -1;
    } else {
        return 1;
    }
}

void client_send_move(char *move) {
    send(client_fd, move, strlen(move), 0);
}

int chess_client_init() {
    int status;
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((status
         = connect(client_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Non blocking code
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        close(client_fd);
        return 0;
    }
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        close(client_fd);
        return 0;
    }
    return 1;
}

#ifdef TEST
int main(int argc, char const* argv[])
{
    int valread;
    char arr[100] = {0};
    char* hello = "Hello from client";

    if (chess_client_init() != 1) {
        perror("Something went wrong");
        return 0;
    }

    // Sending message

    while (1) {
        scanf("%s", arr);
        client_send_move(arr);
        printf("%s messssage sent\n", arr);

        do {
            memset(buffer, 0, 1024);
            valread = client_listen();
        } while (valread < 0);
                                  // terminator at the end
        printf("%s\n", buffer);
    }

    // closing the connected socket
    close(client_fd);
    return 0;
}
#endif
