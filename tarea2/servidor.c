/* SERVIDOR - Sistema de Reservas de Horas Médicas */
/**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_TAM_MENSAJE 100 // Aumentamos el tamaño del mensaje para manejar las solicitudes del cliente

#define NUM_ESPECIALIDADES 4
#define NUM_HORAS 21 // Horas desde las 8:00 am hasta las 18:00 pm, con intervalos de 30 minutos

typedef struct {
    char nombre[50];
    char fecha[11]; // dd/mm/yyyy
    char hora[6];   // hh:mm
    char especialidad[30];
} Reserva;

int descriptor_socket_servidor;
Reserva reservas[NUM_HORAS][NUM_ESPECIALIDADES]; // Matriz para almacenar las reservas

/**********************************************************/
/* Función catch que captura una interrupción             */
/**********************************************************/
void catch(int sig) {
    printf("***Señal: %d atrapada!\n", sig);
    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    exit(EXIT_SUCCESS);
}

/**********************************************************/
/* Función para inicializar las reservas                  */
/**********************************************************/
void inicializar_reservas() {
    for (int i = 0; i < NUM_HORAS; i++) {
        for (int j = 0; j < NUM_ESPECIALIDADES; j++) {
            strcpy(reservas[i][j].nombre, "");
            strcpy(reservas[i][j].fecha, "");
            strcpy(reservas[i][j].hora, "");
            strcpy(reservas[i][j].especialidad, "");
        }
    }
}

/**********************************************************/
/* Función para verificar si una hora está ocupada        */
/**********************************************************/
int hora_ocupada(int hora_index, int especialidad_index) {
    if (strcmp(reservas[hora_index][especialidad_index].nombre, "") == 0)
        return 0; // Hora no ocupada
    else
        return 1; // Hora ocupada
}

/**********************************************************/
/* Función para manejar la solicitud del cliente          */
/**********************************************************/
void manejar_solicitud(char *mensaje_entrada, char *mensaje_salida) {
    // Lógica para manejar la solicitud del cliente
    // Implementa la lógica según los requerimientos del sistema de reservas de horas médicas
}

/**********************************************************/
/* Función MAIN                                           */
/**********************************************************/
int main(int argc, char *argv[]) {
    socklen_t destino_tam;
    struct sockaddr_in socket_servidor, socket_cliente;
    char mensaje_entrada[MAX_TAM_MENSAJE], mensaje_salida[MAX_TAM_MENSAJE];
    int recibidos, enviados; // bytes recibidos

    if (argc != 2) {
        printf("\n\nEl número de parámetros es incorrecto\n");
        printf("Use: %s <puerto>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializar reservas
    inicializar_reservas();

    // Crear el socket del servidor
    descriptor_socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor_socket_servidor == -1) {
        printf("ERROR: El socket del servidor no se ha creado correctamente!\n");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    socket_servidor.sin_family = AF_INET;
    socket_servidor.sin_port = htons(atoi(argv[1]));
    socket_servidor.sin_addr.s_addr = htonl(INADDR_ANY);

    // Asignar una dirección local al socket
    if (bind(descriptor_socket_servidor, (struct sockaddr*)&socket_servidor, sizeof(socket_servidor)) == -1) {
        printf("ERROR al unir el socket a la dirección del servidor\n");
        close(descriptor_socket_servidor);
        exit(EXIT_FAILURE);
    }

    // Manejar las señales de interrupción
    signal(SIGINT, &catch);

    printf("\n***Servidor ACTIVO escuchando en el puerto: %s ...\n", argv[1]);

    do {
        // Quedar a la espera de alguna solicitud del cliente
        destino_tam = sizeof(socket_cliente);
        recibidos = recvfrom(descriptor_socket_servidor, mensaje_entrada, MAX_TAM_MENSAJE, 0, (struct sockaddr*)&socket_cliente, &destino_tam);
        if (recibidos < 0) {
            printf("ERROR de recvfrom() \n");
            exit(EXIT_FAILURE);
        } else {
            printf("***Servidor recibe dato del cliente: %d.\n", socket_cliente.sin_addr.s_addr);
            printf("<<Cliente envía >>: %s\n", mensaje_entrada);

            // Manejar la solicitud del cliente
            manejar_solicitud(mensaje_entrada, mensaje_salida);

            // Enviar respuesta al cliente
            enviados = sendto(descriptor_socket_servidor, mensaje_salida, strlen(mensaje_salida), 0, (struct sockaddr*)&socket_cliente, destino_tam);
            if (enviados < 0) {
                printf("Error en sendto() \n");
                exit(EXIT_SUCCESS);
            } else
                printf("<<Server replica>>: %s\n", mensaje_salida);
        }
    } while (1);

    // Cerrar el servidor
    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    exit(EXIT_SUCCESS);
}
