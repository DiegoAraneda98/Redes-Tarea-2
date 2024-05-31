/* SERVIDOR */
/***********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CONN 100 // Nº máximo de conexiones en espera
#define MAX_TAM_MENSAJE 512 // Número de caracteres máximo del mensaje
#define MAX_TURNOS 100 // Número máximo de turnos

int descriptor_socket_servidor, descriptor_socket_cliente;

typedef struct {
    int id;
    char nombre_paciente[100];
    char fecha[20];
    char hora[10];
    int activo;
} Turno;

Turno turnos[MAX_TURNOS];
int contador_turnos = 0;

/**********************************************************/
/* función catch que captura una interrupción             */
/**********************************************************/
void catch(int sig) {
    printf("***Señal: %d atrapada!\n", sig);
    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_cliente);
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    exit(EXIT_SUCCESS);
}

/**********************************************************/
/* función para inicializar los turnos                    */
/**********************************************************/
void inicializarTurnos() {
    for (int i = 0; i < MAX_TURNOS; i++) {
        turnos[i].id = i;
        turnos[i].activo = 0;
    }
}

/**********************************************************/
/* función para reservar un turno                         */
/**********************************************************/
void reservarTurno(char *mensaje_entrada, char *respuesta) {
    char nombre_paciente[100], fecha[20], hora[10];
    sscanf(mensaje_entrada, "%s %s %s reservar", nombre_paciente, fecha, hora);

    for (int i = 0; i < MAX_TURNOS; i++) {
        if (!turnos[i].activo) {
            turnos[i].activo = 1;
            strcpy(turnos[i].nombre_paciente, nombre_paciente);
            strcpy(turnos[i].fecha, fecha);
            strcpy(turnos[i].hora, hora);
            sprintf(respuesta, "Turno reservado con éxito. ID del turno: %d", turnos[i].id);
            return;
        }
    }

    strcpy(respuesta, "No hay turnos disponibles.");
}

/**********************************************************/
/* función para cancelar un turno                         */
/**********************************************************/
void cancelarTurno(char *mensaje_entrada, char *respuesta) {
    int id_turno;
    sscanf(mensaje_entrada, "%d cancelar", &id_turno);

    if (id_turno >= 0 && id_turno < MAX_TURNOS && turnos[id_turno].activo) {
        turnos[id_turno].activo = 0;
        sprintf(respuesta, "Turno con ID %d cancelado con éxito.", id_turno);
    } else {
        strcpy(respuesta, "Turno no encontrado o ya cancelado.");
    }
}

/**********************************************************/
/* función para consultar turnos                          */
/**********************************************************/
void consultarTurnos(char *mensaje_entrada, char *respuesta) {
    char nombre_paciente[100];
    sscanf(mensaje_entrada, "%s consultar", nombre_paciente);

    strcpy(respuesta, "Turnos reservados:\n");
    for (int i = 0; i < MAX_TURNOS; i++) {
        if (turnos[i].activo && strcmp(turnos[i].nombre_paciente, nombre_paciente) == 0) {
            char turno_info[200];
            sprintf(turno_info, "ID: %d, Fecha: %s, Hora: %s\n", turnos[i].id, turnos[i].fecha, turnos[i].hora);
            strcat(respuesta, turno_info);
        }
    }
}

/**********************************************************/
/* función MAIN                                           */
/* Orden Parametros: Puerto                               */
/**********************************************************/
int main(int argc, char *argv[]) {
    socklen_t destino_tam;
    struct sockaddr_in socket_servidor, socket_cliente;
    char mensaje_entrada[MAX_TAM_MENSAJE], mensaje_salida[MAX_TAM_MENSAJE];

    if (argc != 2) {
        printf("\n\nEl número de parámetros es incorrecto\n");
        printf("Use: %s <puerto>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializar turnos
    inicializarTurnos();

    // Creamos el socket del servidor
    descriptor_socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor_socket_servidor == -1) {
        printf("ERROR: El socket del servidor no se ha creado correctamente!\n");
        exit(EXIT_FAILURE);
    }

    // Se prepara la dirección de la máquina servidora
    socket_servidor.sin_family = AF_INET;
    socket_servidor.sin_port = htons(atoi(argv[1]));
    socket_servidor.sin_addr.s_addr = htonl(INADDR_ANY);

    // Asigna una dirección local al socket
    if (bind(descriptor_socket_servidor, (struct sockaddr*)&socket_servidor, sizeof(socket_servidor)) == -1) {
        printf("ERROR al configurar el socket del servidor\n");
        close(descriptor_socket_servidor);
        exit(EXIT_FAILURE);
    }

    // Espera al establecimiento de alguna conexión de múltiples usuarios
    if (listen(descriptor_socket_servidor, MAX_CONN) == -1) {
        printf("ERROR al establecer la escucha de N conexiones en el servidor\n");
        close(descriptor_socket_servidor);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, &catch);
    while (1) {
        printf("\n***Servidor ACTIVO escuchando en el puerto: %s ...\n", argv[1]);
        // Establece una conexión
        destino_tam = sizeof(socket_cliente);
        descriptor_socket_cliente = accept(descriptor_socket_servidor, (struct sockaddr*)&socket_cliente, &destino_tam);
        if (descriptor_socket_cliente == -1) {
            printf("ERROR al establecer la conexión del servidor con el cliente\n");
            close(descriptor_socket_servidor);
            exit(EXIT_FAILURE);
        }
        printf("***Servidor se conectó con el cliente: %d.\n", socket_cliente.sin_addr.s_addr);
        do {
            // Recibe el mensaje del cliente
            if (recv(descriptor_socket_cliente, mensaje_entrada, sizeof(mensaje_entrada), 0) == -1) {
                perror("Error en recv");
                close(descriptor_socket_cliente);
                close(descriptor_socket_servidor);
                exit(EXIT_SUCCESS);
            } else {
                printf("<<Cliente envía >>: %s\n", mensaje_entrada);
            }

            // Procesar el mensaje del cliente
            if (strstr(mensaje_entrada, "reservar")) {
                reservarTurno(mensaje_entrada, mensaje_salida);
            } else if (strstr(mensaje_entrada, "cancelar")) {
                cancelarTurno(mensaje_entrada, mensaje_salida);
            } else if (strstr(mensaje_entrada, "consultar")) {
                consultarTurnos(mensaje_entrada, mensaje_salida);
            } else if (strcmp(mensaje_entrada, "terminar();") == 0) {
                strcpy(mensaje_salida, "Conexión terminada por el cliente.");
            } else {
                strcpy(mensaje_salida, "Comando no reconocido.");
            }

            // Envía el mensaje al cliente
            if (send(descriptor_socket_cliente, mensaje_salida, strlen(mensaje_salida) + 1, 0) == -1) {
                perror("Error en send");
                close(descriptor_socket_cliente);
                close(descriptor_socket_servidor);
                exit(EXIT_SUCCESS);
            } else {
                printf("<<Servidor replica>>: %s\n", mensaje_salida);
            }
        } while (strcmp(mensaje_entrada, "terminar();") != 0);

        // Cierra la conexión con el cliente actual
        printf("***Cerrando conexión con cliente ...\n");
        close(descriptor_socket_cliente);
        printf("***Conexión cerrada.\n");
    }
    // Cierra el servidor
    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    exit(EXIT_SUCCESS);
}
