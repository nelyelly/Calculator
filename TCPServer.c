#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <ctype.h>
#include <process.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define LOG_FILE "log_C.txt"

#pragma comment(lib, "ws2_32.lib")

typedef struct {
    SOCKET client_socket;
    struct sockaddr_in client_addr;
} client_data_t;

HANDLE log_mutex;

void log_message(const char* type, const char* message, struct sockaddr_in client_addr) {
    WaitForSingleObject(log_mutex, INFINITE);
    
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);
    
    FILE* log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        ReleaseMutex(log_mutex);   
        return;
    }

    fprintf(log_file, "[%s] [%s] [Client: %s:%d] %s\n",
            timestamp, type,
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port),
            message);

    fclose(log_file);
    ReleaseMutex(log_mutex);       
    
    printf("[%s] [%s] [Client: %s:%d] %s\n",
           timestamp, type,
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port),
           message);
    
    ReleaseMutex(log_mutex);
}

void calculate(double num1, double num2, char operator, char* result_buffer, size_t buffer_size) {
    switch (operator) {
        case '+':
            snprintf(result_buffer, buffer_size, "%.2f", num1 + num2);
            break;
        case '-':
            snprintf(result_buffer, buffer_size, "%.2f", num1 - num2);
            break;
        case '*':
            snprintf(result_buffer, buffer_size, "%.2f", num1 * num2);
            break;
        case '/':
            if (num2 == 0) {
                strncpy(result_buffer, "ERREUR: Division par zero", buffer_size);
            } else {
                snprintf(result_buffer, buffer_size, "%.2f", num1 / num2);
            }
            break;
        default:
            strncpy(result_buffer, "ERREUR: Operateur inconnu", buffer_size);
            break;
    }
}

int is_valid_number(const char* str) {
    if (str == NULL || str[0] == '\0') return 0;
    
    int dot_count = 0;
    int i = 0;
    int length = strlen(str);
    
    if (str[i] == '-' || str[i] == '+') {
        i++;
        if (i == length) return 0;
    }
    
    int has_digit = 0;
    
    for (; i < length; i++) {
        if (str[i] == '.') {
            dot_count++;
            if (dot_count > 1) return 0;
        } else if (isdigit((unsigned char)str[i])) {
            has_digit = 1;
        } else {
            return 0;
        }
    }
    
    return has_digit;
}

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

unsigned __stdcall handle_client(void* arg) {
    client_data_t* client_data = (client_data_t*)arg;
    SOCKET client_socket = client_data->client_socket;
    struct sockaddr_in client_addr = client_data->client_addr;
    
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char value[100];
    
    double num1 = 0, num2 = 0;
    char operator = 0;
    int step = 0;
    
    log_message("CONNEXION", "Nouveau client connecte", client_addr);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = read_line(client_socket, buffer, BUFFER_SIZE);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                log_message("DECONNEXION", "Client deconnecte", client_addr);
            } else {
                char error_msg[100];
                snprintf(error_msg, sizeof(error_msg), "Erreur recv: %ld", WSAGetLastError());
                log_message("ERREUR", error_msg, client_addr);
            }
            break;
        }
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        if (strncmp(buffer, "NUMBER:", 7) == 0) {
            strcpy(value, buffer + 7);
            
            if (!is_valid_number(value)) {
                snprintf(response, sizeof(response), "ERROR:Format numerique invalide: %s", value);
                send(client_socket, response, strlen(response), 0);
                send(client_socket, "\n", 1, 0);
                log_message("ERREUR", response, client_addr);
                continue;
            }
            
            if (step == 0) {
                num1 = atof(value);
                step = 1;
                char log_msg[100];
                snprintf(log_msg, sizeof(log_msg), "Nombre 1 recu: %.2f", num1);
                log_message("INFO", log_msg, client_addr);
            } else if (step == 1) {
                num2 = atof(value);
                step = 2;
                char log_msg[100];
                snprintf(log_msg, sizeof(log_msg), "Nombre 2 recu: %.2f", num2);
                log_message("INFO", log_msg, client_addr);
            } else {
                snprintf(response, sizeof(response), "ERROR:Sequence invalide - deja recu deux nombres");
                send(client_socket, response, strlen(response), 0);
                send(client_socket, "\n", 1, 0);
                log_message("ERREUR", "Sequence invalide", client_addr);
                step = 0;
                continue;
            }
            
        } else if (strncmp(buffer, "OPERATOR:", 9) == 0) {
            strcpy(value, buffer + 9);
            
            if (step != 2) {
                snprintf(response, sizeof(response), "ERROR:Sequence invalide - attendu deux nombres avant operateur");
                send(client_socket, response, strlen(response), 0);
                send(client_socket, "\n", 1, 0);
                log_message("ERREUR", "Sequence invalide", client_addr);
                step = 0;
                continue;
            }
            
            if (strlen(value) != 1 || 
               (value[0] != '+' && value[0] != '-' && 
                value[0] != '*' && value[0] != '/')) {
                snprintf(response, sizeof(response), "ERROR:Operateur invalide: %s", value);
                send(client_socket, response, strlen(response), 0);
                send(client_socket, "\n", 1, 0);
                log_message("ERREUR", response, client_addr);
                step = 0;
                continue;
            }
            
            operator = value[0];
            char log_msg[100];
            snprintf(log_msg, sizeof(log_msg), "Operateur recu: %c", operator);
            log_message("INFO", log_msg, client_addr);
            
            char result_str[100];
            calculate(num1, num2, operator, result_str, sizeof(result_str));
            
            if (strstr(result_str, "ERREUR") == NULL) {
                snprintf(response, sizeof(response), "RESULT:%s", result_str);
                
                char calc_log[100];
                snprintf(calc_log, sizeof(calc_log), "%.2f %c %.2f = %s", 
                        num1, operator, num2, result_str);
                log_message("CALCUL", calc_log, client_addr);
            } else {
                snprintf(response, sizeof(response), "ERROR:%s", result_str);
                log_message("ERREUR", result_str, client_addr);
            }
            
            send(client_socket, response, strlen(response), 0);
            send(client_socket, "\n", 1, 0);
            
            step = 0;
            num1 = num2 = 0;
            operator = 0;
            
        } else if (strcmp(buffer, "quit") == 0) {
            log_message("INFO", "Commande quit recue", client_addr);
            snprintf(response, sizeof(response), "BYE:Deconnexion du serveur");
            send(client_socket, response, strlen(response), 0);
            send(client_socket, "\n", 1, 0);
            break;
        } else {
            snprintf(response, sizeof(response), "ERROR:Commande non reconnue: %s", buffer);
            send(client_socket, response, strlen(response), 0);
            send(client_socket, "\n", 1, 0);
            log_message("ERREUR", response, client_addr);
            step = 0;
        }
    }
    
    closesocket(client_socket);
    free(client_data);
    
    _endthreadex(0);
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET server_socket = INVALID_SOCKET;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);
    int iResult;
    
    printf("TCP SERVER\n");
    printf("Port: %d\n", PORT);
    printf("Log file: %s\n", LOG_FILE);
    printf("Waiting for clients...\n");
    
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("Erreur WSAStartup: %d\n", iResult);
        return 1;
    }
    
    log_mutex = CreateMutex(NULL, FALSE, NULL);
    if (log_mutex == NULL) {
        printf("Erreur creation mutex: %ld\n", GetLastError());
        WSACleanup();
        return 1;
    }
    
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        printf("Erreur creation socket: %ld\n", WSAGetLastError());
        CloseHandle(log_mutex);
        WSACleanup();
        return 1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    iResult = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (iResult == SOCKET_ERROR) {
        printf("bind error: %ld\n", WSAGetLastError());
        closesocket(server_socket);
        CloseHandle(log_mutex);
        WSACleanup();
        return 1;
    }
    
    iResult = listen(server_socket, MAX_CLIENTS);
    if (iResult == SOCKET_ERROR) {
        printf("Erreur listen: %ld\n", WSAGetLastError());
        closesocket(server_socket);
        CloseHandle(log_mutex);
        WSACleanup();
        return 1;
    }
    
    printf("Server waiting for client connexion...\n");
    printf("Ctrl+C to stop the server\n\n");
    
    while (1) {
        SOCKET client_socket = accept(server_socket, 
                                      (struct sockaddr*)&client_addr, 
                                      &client_addr_len);
        
        if (client_socket == INVALID_SOCKET) {
            printf("Erreur accept: %ld\n", WSAGetLastError());
            continue;
        }
        
        client_data_t* client_data = (client_data_t*)malloc(sizeof(client_data_t));
        if (client_data == NULL) {
            printf("Erreur allocation memoire pour client_data\n");
            closesocket(client_socket);
            continue;
        }
        
        client_data->client_socket = client_socket;
        client_data->client_addr = client_addr;
        
        HANDLE thread_handle = (HANDLE)_beginthreadex(
            NULL,
            0,
            handle_client,
            client_data,
            0,
            NULL
        );
        
        if (thread_handle == NULL) {
            printf("Erreur creation thread: %d\n", errno);
            closesocket(client_socket);
            free(client_data);
        } else {
            CloseHandle(thread_handle);
        }
    }
    
    closesocket(server_socket);
    CloseHandle(log_mutex);
    WSACleanup();
    
    return 0;
}