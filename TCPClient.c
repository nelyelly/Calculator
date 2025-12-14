#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

#pragma comment(lib, "ws2_32.lib")

int read_line(SOCKET sock, char* buffer, int buffer_size) {
    int total_received = 0;
    char c;
    
    memset(buffer, 0, buffer_size);
    
    while (total_received < buffer_size - 1) {
        int received = recv(sock, &c, 1, 0);
        
        if (received <= 0) {
            return received;
        }
        
        if (c == '\n') {
            buffer[total_received] = '\0';
            
            if (total_received > 0 && buffer[total_received - 1] == '\r') {
                buffer[total_received - 1] = '\0';
            }
            
            return total_received;
        }
        
        buffer[total_received++] = c;
    }
    
    buffer[buffer_size - 1] = '\0';
    return total_received;
}

int main() {
    char server_ip[100] = "localhost";
    
    printf("Compute Client TCP\n");
    printf("===================\n");
    printf("Enter server IP (or 'localhost' for local): ");
    scanf("%99s", server_ip);
    getchar();
    
    printf("Connection to server %s:%d\n", server_ip, SERVER_PORT);
    
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int iResult;
    
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf(" WSAStartup echoue: %d\n", iResult);
        return 1;
    }
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf(" Erreur creation socket: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    
    if (strcmp(server_ip, "localhost") == 0) {
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        serv_addr.sin_addr.s_addr = inet_addr(server_ip);
        if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
            struct hostent *host = gethostbyname(server_ip);
            if (host == NULL) {
                printf(" Impossible de resoudre %s\n", server_ip);
                printf(" Verifiez:\n");
                printf("  1. L'adresse est correcte\n");
                printf("  2. Vous etes connecte au meme reseau\n");
                closesocket(sock);
                WSACleanup();
                return 1;
            }
            memcpy(&serv_addr.sin_addr, host->h_addr, host->h_length);
        }
    }
    
    printf("Tentative de connexion...\n");
    iResult = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (iResult == SOCKET_ERROR) {
        printf(" Impossible de se connecter: %ld\n", WSAGetLastError());
        printf("Verifiez que:\n");
        printf("  1. Le serveur est lance\n");
        printf("  2. Le port %d est ouvert sur le serveur\n", SERVER_PORT);
        printf("  3. Le pare-feu autorise les connexions\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    
    printf("Connected to server!\n\n");

    while (1) {
        char input1[50], input2[50], op[10];
        char send_buffer[100];
        
        printf("NUMBER: ");
        fflush(stdout);
        if (fgets(input1, sizeof(input1), stdin) == NULL) break;
        input1[strcspn(input1, "\n")] = 0;
        
        if (strcmp(input1, "quit") == 0) break;
        
        printf("NUMBER: ");
        fflush(stdout);
        if (fgets(input2, sizeof(input2), stdin) == NULL) break;
        input2[strcspn(input2, "\n")] = 0;
        
        printf("OPERATOR: ");
        fflush(stdout);
        if (fgets(op, sizeof(op), stdin) == NULL) break;
        op[strcspn(op, "\n")] = 0;
        
        if (strlen(op) != 1 || 
           (op[0] != '+' && op[0] != '-' && 
            op[0] != '*' && op[0] != '/')) {
            printf(" Operateur invalide\n\n");
            continue;
        }
        
        snprintf(send_buffer, sizeof(send_buffer), "NUMBER:%s\nNUMBER:%s\nOPERATOR:%s\n", input1, input2, op);
        int sent = send(sock, send_buffer, (int)strlen(send_buffer), 0);
        if (sent == SOCKET_ERROR) {
            printf(" Erreur send: %ld\n\n", WSAGetLastError());
            break;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        iResult = read_line(sock, buffer, BUFFER_SIZE);
        
        if (iResult <= 0) {
            if (iResult == 0) {
                printf("Server disconnected\n\n");
            } else {
                printf(" Erreur recv: %ld\n\n", WSAGetLastError());
            }
            break;
        }
        
        if (strncmp(buffer, "RESULT:", 7) == 0) {
            printf("\n RESULTAT: %s\n\n", buffer + 7);
        } else if (strncmp(buffer, "ERROR:", 6) == 0) {
            printf("\n ERROR: %s\n\n", buffer + 6);
        } else if (strncmp(buffer, "BYE:", 4) == 0) {
            printf("\n %s\n\n", buffer + 4);
            break;
        } else {
            printf("\n Reponse inattendue: %s\n\n", buffer);
        }
    }
    
    printf("\nDeconnexion...\n");
    closesocket(sock);
    WSACleanup();
    
    return 0;
}