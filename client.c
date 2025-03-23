#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern int errno;
int port;

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;

    if (argc != 4)
    {
        printf("Sintaxa: %s <adresa_srv_olimpiada> <port> <user_name>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);
    char *username = argv[3];

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket()");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Eroare la connect() in client");
        return errno;
    }

    // citim timpul ramas de la server
    int time_left;
    if (read(sd, &time_left, sizeof(time_left)) <= 0)
    {
        printf("Nu te mai poti conecta la serverul de olimpiada. Timpul a expirat.\n");
        close(sd);
        return -1;
    }
    printf("Mai aveti %d secunde pentru a mai trimite solutii\n", time_left);

    if (time_left <= 0)
    {
        printf("Timpul de comunicare cu serverul s-a terminat\n");
        close(sd);
        return 0;
    }

    char buffer[1024];
    int bytes_received;

    // citim enuntul problemei
    bytes_received = read(sd, buffer, sizeof(buffer) - 1);
    if (bytes_received <= 0)
    {
        perror("Eroare la citirea problemei de la srv");
        close(sd);
        return -1;
    }
    buffer[bytes_received] = '\0';
    printf("Enuntul problemei este:\n%s\n", buffer);

    // cat timp mai avem timp de comunicat cu srv
    time_t start_time = time(NULL);
    while (difftime(time(NULL), start_time) < time_left)
    {
        printf("Scrie un mesaj pentru server (sau 'quit' pentru a iesi): ");
        char message[256];
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';

        // dam quit si primim leaderboard
        if (strcmp(message, "quit") == 0)
        {
            printf("Ai ales sa te deconectezi.\n");

            if (write(sd, message, strlen(message)) <= 0)
            {
                perror("Eroare la trimiterea mesajului 'quit' spre server");
                close(sd);
                return -1;
            }

            printf("Asteptam clasamentul de la server...\n");
            bytes_received = read(sd, buffer, sizeof(buffer) - 1);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = '\0';
                printf("Clasamentul primit:\n%s\n", buffer);
            }
            else
            {
                perror("Eroare la citirea clasamentului");
            }

            break;
        }
        if (strlen(message) == 0)
        {
            printf("Trebuie sa introduci o comanda: 'quit' / 'check <username>'\n");
        }
        else
        {
            if (write(sd, message, strlen(message)) <= 0)
            {
                perror("Eroare la trimiterea mesajului spre srv");
                close(sd);
                return -1;
            }
            printf("%s\n", message);
            // citim rasp de la srv
            bytes_received = read(sd, buffer, sizeof(buffer) - 1);
            if (bytes_received <= 0)
            {
                perror("Eroare la citirea raspunsului de la srv");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("Raspuns de la server: %s\n", buffer);
        }
    }

    // dupa timeout asteptam rasp final de la srv
    bytes_received = read(sd, buffer, sizeof(buffer) - 1);
    if (bytes_received > 0)
    {
        printf("Asteptam clasamentul final de la server...\n");
        buffer[bytes_received] = '\0';
        printf("Clasamentul final primit:\n%s\n", buffer);
    }
    // daca srv a incheiat conexiunea
    else if (bytes_received == 0)
    {
        printf("Conexiunea cu serverul s-a terminat\n");
    }
    else
    {
        perror("Eroare la citirea clasamentului final");
    }

    close(sd);
    return 0;
}
