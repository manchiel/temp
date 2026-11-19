/*
    Kviz klijent (TCP)
    Prevodjenje: gcc client.c -o client
    Pokretanje:  ./client   (server mora vec da radi)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT 12345
#define DEFAULT_BUFLEN 512

int main(int argc, char *argv[])
{
    int client_socket_fd;
    struct sockaddr_in server_address;

    client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_fd == -1)
    {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    server_address.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (connect(client_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Failed to connect");
        return EXIT_FAILURE;
    }
    printf("Povezan na server. Pocinje kviz!\n");

    char buffer[DEFAULT_BUFLEN * 2];
    char unos[64];
    int read_size;

    // TODO 7: petlja igre
    while (1)
    {
        // Primi poruku od servera
        read_size = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (read_size <= 0)
        {
            printf("Server je prekinuo vezu.\n");
            break;
        }
        buffer[read_size] = '\0';
        printf("%s\n", buffer);

        // Ako je stigao konacan rezultat, kraj
        if (strstr(buffer, "KRAJ") != NULL)
            break;

        // Ucitaj odgovor; dozvoljeno samo A, B ili C
        char odgovor;
        while (1)
        {
            printf("Vas odgovor (A/B/C): ");
            if (fgets(unos, sizeof(unos), stdin) == NULL)   // Ctrl+D
            {
                close(client_socket_fd);
                return 0;
            }
            odgovor = toupper(unos[0]);
            if (odgovor == 'A' || odgovor == 'B' || odgovor == 'C')
                break;
            printf("Neispravan unos, unesite A, B ili C.\n");
        }

        // Posalji samo jedno slovo
        if (send(client_socket_fd, &odgovor, 1, 0) < 0)
        {
            perror("Send failed");
            break;
        }
    }

    close(client_socket_fd);
    printf("Veza zatvorena.\n");
    return 0;
}
