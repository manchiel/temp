/*
    Kviz server (TCP, vise klijenata preko niti)
    Prevodjenje: gcc server.c -o server -pthread
    Pokretanje:  ./server
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

#define PORT 12345
#define DEFAULT_BUFLEN 512
#define BROJ_PITANJA 5
#define PITANJA_PO_KLIJENTU 3

// Jedno pitanje: tekst sa ponudjenim odgovorima i slovo tacnog odgovora
typedef struct {
    const char *tekst;
    char tacan_odgovor;
} pitanje_t;

// Pitanja su globalna, ali ih niti samo CITAJU, pa ne treba mutex
pitanje_t pitanja[BROJ_PITANJA] = {
    {"Koji protokol garantuje isporuku podataka?\nA) UDP\nB) TCP\nC) IP", 'B'},
    {"Koja funkcija na serveru prihvata konekciju?\nA) connect\nB) listen\nC) accept", 'C'},
    {"Koliko bita ima IPv4 adresa?\nA) 32\nB) 64\nC) 128", 'A'},
    {"Koji port koristi HTTP?\nA) 21\nB) 80\nC) 443", 'B'},
    {"Sta oznacava adresa 127.0.0.1?\nA) Lokalni racunar\nB) Ruter\nC) DNS server", 'A'},
};

// Funkcija koju izvrsava nit za JEDNOG klijenta
void* client_func(void* arg)
{
    // TODO 1: preuzmi fd sa "ceduljice" i baci ceduljicu
    int client_fd = *(int*)arg;
    free(arg);
    printf("Nit pokrenuta za klijenta fd=%d\n", client_fd);

    // TODO 2: lokalno stanje ovog klijenta (svaka nit ima svoje)
    int bodovi = 0;
    int used[BROJ_PITANJA] = {0};

    char buffer[DEFAULT_BUFLEN];        // za primanje odgovora
    char poruka[DEFAULT_BUFLEN * 2];    // poruka koju saljemo
    char povratna[DEFAULT_BUFLEN] = ""; // "Tacno!" / "Netacno!" od prethodnog pitanja
    int read_size;

    // TODO 3: petlja pitanja
    for (int k = 0; k < PITANJA_PO_KLIJENTU; k++)
    {
        // Izaberi nasumicno pitanje koje ovaj klijent jos nije dobio
        int i;
        do {
            i = rand() % BROJ_PITANJA;
        } while (used[i] == 1);
        used[i] = 1;

        // Povratna info od proslog pitanja + novo pitanje, u JEDNOJ poruci
        // (da se dve poruke ne bi slepile u jednom recv kod klijenta)
        sprintf(poruka, "%s\nPitanje %d/%d: %s",
                povratna, k + 1, PITANJA_PO_KLIJENTU, pitanja[i].tekst);

        if (send(client_fd, poruka, strlen(poruka), 0) < 0)
        {
            perror("Send failed");
            close(client_fd);
            return NULL;
        }

        // Cekaj odgovor klijenta
        read_size = recv(client_fd, buffer, DEFAULT_BUFLEN - 1, 0);
        if (read_size <= 0)
        {
            if (read_size == 0)
                printf("Klijent fd=%d se diskonektovao usred kviza\n", client_fd);
            else
                perror("Receive failed");
            close(client_fd);
            return NULL;
        }
        buffer[read_size] = '\0';

        // Proveri odgovor (uzimamo samo prvo slovo, velikim slovom)
        char odgovor = toupper(buffer[0]);
        printf("fd=%d odgovorio %c na pitanje %d\n", client_fd, odgovor, i);

        if (odgovor == pitanja[i].tacan_odgovor)
        {
            bodovi++;
            sprintf(povratna, "Tacno!\n");
        }
        else
        {
            sprintf(povratna, "Netacno! Tacan odgovor je %c.\n", pitanja[i].tacan_odgovor);
        }
    }

    // TODO 4: poslednja povratna info + konacan rezultat, u jednoj poruci
    sprintf(poruka, "%s\nKRAJ. Osvojili ste %d/%d bodova.\n",
            povratna, bodovi, PITANJA_PO_KLIJENTU);
    send(client_fd, poruka, strlen(poruka), 0);
    printf("Klijent fd=%d zavrsio sa %d/%d bodova\n", client_fd, bodovi, PITANJA_PO_KLIJENTU);

    // TODO 5: zatvori vezu
    close(client_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int server_socket_fd;
    int client_socket_fd;
    socklen_t len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    srand(time(NULL));   // jednom, da rand() ne daje uvek isti niz

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1)
    {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        return EXIT_FAILURE;
    }

    listen(server_socket_fd, 3);
    printf("Server ceka klijente na portu %d...\n", PORT);
    len = sizeof(struct sockaddr_in);

    while (1)
    {
        client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &len);
        if (client_socket_fd < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Klijent [%s:%hu] povezan\n",
               inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // TODO 6: nova nit za ovog klijenta
        int *pclient = malloc(sizeof(int));
        if (!pclient)
        {
            perror("Malloc failed");
            close(client_socket_fd);
            continue;
        }
        *pclient = client_socket_fd;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_func, pclient) != 0)
        {
            perror("pthread_create failed");
            free(pclient);
            close(client_socket_fd);
            continue;
        }
        pthread_detach(client_thread);
    }

    close(server_socket_fd);
    return 0;
}
