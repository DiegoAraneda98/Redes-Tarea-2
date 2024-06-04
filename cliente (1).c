#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void mostrar_menu_principal() {
    clear_screen();
    printf("Bienvenido a Clinica de Redes.\n");
    printf("Seleccione el área correspondiente a consultar:\n");
    printf("1. Oncología.\n");
    printf("2. Pediatría.\n");
    printf("3. Oftalmología.\n");
    printf("4. Salir.\n");
}

void mostrar_submenu(char *categoria) {
    clear_screen();
    printf("Seleccione una opción para %s:\n", categoria);
    printf("1. Reservar hora.\n");
    printf("2. Consultar reserva.\n");
    printf("3. Cancelar hora.\n");
    printf("4. Consultar horario.\n");
    printf("5. Retroceder.\n");
}

void manejar_reserva(int client_socket) {
    char buffer[1024];
    int hora_index;

    // Solicitar y mostrar horarios disponibles
    read(client_socket, buffer, sizeof(buffer));
    printf("%s", buffer);

    // Recibir y mostrar todas las líneas de horarios
    while (read(client_socket, buffer, sizeof(buffer)) > 0) {
        printf("%s", buffer);
        if (strchr(buffer, '\n') != NULL) break;
        bzero(buffer, sizeof(buffer));
    }

    // Enviar la selección de hora
    scanf("%d", &hora_index);
    sprintf(buffer, "%d", hora_index);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    // Enviar datos de la reserva
    printf("Ingrese nombre: ");
    bzero(buffer, sizeof(buffer));
    scanf(" %[^\n]s", buffer);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    printf("Ingrese fecha (dd-mm-aaaa): ");
    bzero(buffer, sizeof(buffer));
    scanf(" %[^\n]s", buffer);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    printf("Ingrese categoria: ");
    bzero(buffer, sizeof(buffer));
    scanf(" %[^\n]s", buffer);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    // Leer la confirmación de la reserva
    bzero(buffer, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void manejar_consulta(int client_socket) {
    char buffer[1024];

    printf("Ingrese ID de consulta: ");
    bzero(buffer, sizeof(buffer));
    scanf(" %[^\n]s", buffer);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    bzero(buffer, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void manejar_cancelacion(int client_socket) {
    char buffer[1024];

    printf("Ingrese ID de consulta: ");
    bzero(buffer, sizeof(buffer));
    scanf(" %[^\n]s", buffer);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    bzero(buffer, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    printf("%s", buffer);
}

void consultar_horario(int client_socket) {
    char buffer[1024];
    int mes, dia;

    printf("Ingrese mes (1-12): ");
    scanf("%d", &mes);
    sprintf(buffer, "%d", mes);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    printf("Ingrese día: ");
    scanf("%d", &dia);
    sprintf(buffer, "%d", dia);
    write(client_socket, buffer, strlen(buffer));
    write(client_socket, "\n", 1);  // Enviar nueva línea

    bzero(buffer, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    printf("%s", buffer);

    while (read(client_socket, buffer, sizeof(buffer)) > 0) {
        printf("%s", buffer);
        if (strchr(buffer, '\n') != NULL) break;
        bzero(buffer, sizeof(buffer));
    }
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    int opcion, categoria_opcion;
    char buffer[1024]; // Declarar la variable buffer

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    while (1) {
        mostrar_menu_principal();
        scanf("%d", &categoria_opcion);

        switch (categoria_opcion) {
            case 1:
                mostrar_submenu("Oncología");
                break;
            case 2:
                mostrar_submenu("Pediatría");
                break;
            case 3:
                mostrar_submenu("Oftalmología");
                break;
            case 4:
                close(client_socket);
                exit(0);
            default:
                printf("Opción no válida.\n");
                continue;
        }

        scanf("%d", &opcion);
        sprintf(buffer, "%d", opcion);
        write(client_socket, buffer, strlen(buffer));
        write(client_socket, "\n", 1);  // Enviar nueva línea

        switch (opcion) {
            case 1:
                manejar_reserva(client_socket);
                break;
            case 2:
                manejar_consulta(client_socket);
                break;
            case 3:
                manejar_cancelacion(client_socket);
                break;
            case 4:
                consultar_horario(client_socket);
                break;
            case 5:
                continue;
            default:
                printf("Opción no válida.\n");
        }
    }

    return 0;
}
