#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

/**
 * @brief Boucle principale du client : écoute les messages du serveur et permet la saisie utilisateur.
 * @param sockfd Descripteur de fichier du socket client.
 */
void client_chat_boucle(int sockfd, const char username[]) {
    fd_set readfds;
    char buffer[256];
    char message_with_username[280];  // Taille suffisante pour username + message

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);

        // Attente d'une activité sur l'un des descripteurs
        if (select(sockfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erreur lors de select()");
            break;
        }

        // Saisie utilisateur
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            bzero(buffer, 256);
            if (fgets(buffer, 255, stdin) == NULL) {
                // Fin de saisie (Ctrl+D)
                break;
            }

            // Ajoute le nom d'utilisateur devant le message
            snprintf(message_with_username, sizeof(message_with_username), "[%s] %s", username, buffer);

            // Envoie le message avec le nom d'utilisateur
            if (send(sockfd, message_with_username, strlen(message_with_username), 0) < 0) {
                perror("Erreur lors de l'envoi du message");
                break;
            }
        }

        // Message entrant du serveur
        if (FD_ISSET(sockfd, &readfds)) {
            bzero(buffer, 256);
            int n = recv(sockfd, buffer, 255, 0);
            if (n <= 0) {
                printf("Connexion fermée par le serveur.\n");
                break;
            }
            printf("%s", buffer);  // Affiche le message reçu
        }
    }
}

int main(int argc, char *argv[])
{
    char serv_ip_choose[21];
    int serv_port_choose;
    char username[64];

    printf("##### Welcome to Simple TCP Messenger 1.0 #####\n");
    printf("Please, insert the chat server IP address: ");
    scanf("%20s", serv_ip_choose);
    printf("The distant chat server will be joined at: %s\n\n", serv_ip_choose);
    printf("Please, insert the chat server port: ");
    scanf("%d", &serv_port_choose);
    printf("The distant chat server will be joined on port: %d\n\n", serv_port_choose);
    printf("Please insert your username (A-Z ; a-z ; 0-1 ; _  characters accepted): ");
    scanf("%63s", username);
    printf("Your username is: %s\n\n", username);
    printf(">> Initializing connexion...\n");


    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];

    // Création du socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port_choose);
    inet_pton(AF_INET, serv_ip_choose, &serv_addr.sin_addr);

    // Connexion au serveur
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    printf("Connected to chatroom !\n");

    // Réception du message
    bzero(buffer, 256);
    int n = read(sockfd, buffer, 255);
    if (n < 0)
    {
        perror("Failed to receive Welcome message");
        exit(EXIT_FAILURE);
    }
    printf("Message received from chatroom server: %s, %s\n", username, buffer);


    // Boucle messagerie
    client_chat_boucle(sockfd, username);

    // Exit...
    printf("\n >> Closing Simple TCP Messenger v1.0...\n");

    // Fermeture du socket
    close(sockfd);

    return 0;
}
