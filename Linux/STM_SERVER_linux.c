#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define MAX_CLIENTS 40

/**
 * @brief Boucle principale du serveur : écoute et relaie les messages entre clients.
 * @param sockfd Descripteur de fichier du socket serveur.
 * @param client_sockets Tableau des descripteurs de fichiers des clients connectés.
 */
void serveur_relais_boucle(int sockfd, int client_sockets[], const char admin_passwd[]) {
    fd_set readfds;
    char buffer[256];
    char *message_bienvenue = "bienvenue sur le chat !\n";
    char *message_fermeture = "L'hôte a fermé le chatroom\n";

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int max_sd = sockfd;

        // Ajoute les sockets clients à l'ensemble
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Attente d'une activité sur l'un des descripteurs
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erreur lors de select()");
            continue;
        }

        // Nouvelle connexion entrante
        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in cli_addr;
            socklen_t clilen = sizeof(cli_addr);
            int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0) {
                perror("Erreur lors de l'acceptation de la connexion");
                continue;
            }
            printf("Nouveau client connecté.\n");

            // Envoi du message de bienvenue
            if (send(newsockfd, message_bienvenue, strlen(message_bienvenue), 0) < 0) {
                perror("Erreur lors de l'envoi du message de bienvenue");
                close(newsockfd);
                continue;
            }

            // Ajoute le nouveau socket client au tableau
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = newsockfd;
                    break;
                }
            }
        }

        // Lecture des messages des clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                bzero(buffer, 256);
                int n = recv(sd, buffer, 255, 0);
                if (n <= 0) {
                    // Client déconnecté ou erreur
                    printf("Client déconnecté.\n");
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Vérifie si le message contient le mot de passe administrateur
                    if (strstr(buffer, admin_passwd) != NULL) {
                        printf("Mot de passe administrateur détecté. Fermeture du chatroom...\n");
                        // Envoie le message de fermeture à tous les clients
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] != 0) {
                                send(client_sockets[j], message_fermeture, strlen(message_fermeture), 0);
                                close(client_sockets[j]);
                                client_sockets[j] = 0;
                            }
                        }
                        // Ferme le socket serveur
                        close(sockfd);
                        return;  // Quitte la fonction
                    }

                    // Relayage du message à tous les autres clients
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] != 0 && client_sockets[j] != sd) {
                            send(client_sockets[j], buffer, n, 0);
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    int choose_port;
    char admin_passwd[32];
    printf("Welcome, choose the server port: ");
    scanf("%d", &choose_port);
    printf("Port chosen: %d\n", choose_port);
    printf("Enter the administrative password: ");
    scanf("%31s", admin_passwd);
    


    int sockfd;
    int newsockfd[MAX_CLIENTS] = {0};
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr[MAX_CLIENTS];
    socklen_t clilen;
    char buffer[256];
    uint8_t run = 1;
    uint16_t clientNo = 0;

    // Création du socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socked");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(choose_port);

    // Bind du socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    // Écoute sur le socket
    listen(sockfd, MAX_CLIENTS);
    printf("Standing by on port %d...\n", choose_port);

    serveur_relais_boucle(sockfd, newsockfd, admin_passwd);


    close(sockfd);

    return 0;
}
