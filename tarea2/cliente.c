/* CLIENTE - Sistema de Reservas de Horas Médicas */
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

/**********************************************************/
/* Función MAIN                                           */
/**********************************************************/
int main(int argc, char *argv[]) {
    int descriptor_socket_origen;
    struct sockaddr_in socket_origen, socket_destino;
    socklen_t destino_tam;
    char mensaje[MAX_TAM_MENSAJE];
    char respuesta[MAX_TAM_MENSAJE];
    int recibidos, enviados; // bytes recibidos y enviados

    if (argc != 3) {
        printf("\n\nEl número de parámetros es incorrecto\n");
        printf("Use: %s <IP_servidor> <puerto>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Crear el socket del cliente
    descriptor_socket_origen = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor_socket_origen == -1) {
        printf("ERROR: El socket del cliente no se ha creado correctamente!\n");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección de la máquina cliente
    socket_origen.sin_family = AF_INET;
    socket_origen.sin_port = htons(0);                                 // Asigna un puerto disponible de la máquina
    socket_origen.sin_addr.s_addr = htonl(INADDR_ANY);    // Asigna una IP de la máquina

    // Asignar una dirección local al socket
    if (bind(descriptor_socket_origen, (struct sockaddr*)&socket_origen, sizeof(socket_origen)) == -1) {
        printf("ERROR al unir el socket a la dirección de la máquina cliente\n");
        close(descriptor_socket_origen);
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección de la máquina servidora
    socket_destino.sin_family = AF_INET;
    socket_destino.sin_addr.s_addr = inet_addr(argv[1]);
    socket_destino.sin_port = htons(atoi(argv[2]));

    do {
        // Solicitar al cliente que ingrese la solicitud
        printf("\nElija una especialidad:\n");
        printf("1. Especialidad 1\n");
        printf("2. Especialidad 2\n");
        printf("3. Especialidad 3\n");
        printf("4. Especialidad 4\n");
        printf("Ingrese el número correspondiente a la especialidad: ");
        int especialidad;
        scanf("%d", &especialidad);

        // Validar la selección de especialidad
        if (especialidad < 1 || especialidad > 4) {
            printf("Especialidad inválida. Por favor, elija un número entre 1 y 4.\n");
            continue;
        }

        // Solicitar al cliente que ingrese una fecha válida
        char fecha[11];
        printf("Ingrese la fecha en formato dd/mm/yyyy: ");
        scanf("%s", fecha);

        // Solicitar al cliente que ingrese su nombre
        char nombre[50];
        printf("Ingrese su nombre: ");
        scanf("%s", nombre);

        // Solicitar al cliente que ingrese la hora en formato hh:mm
        char hora[6];
        printf("Ingrese la hora en formato hh:mm: ");
        scanf("%s", hora);

        // Construir el mensaje para enviar al servidor
        sprintf(mensaje, "%d,%s,%s,%s,%s", especialidad, fecha, hora, nombre);

        // Enviar la solicitud al servidor
        enviados = sendto(descriptor_socket_origen, mensaje, strlen(mensaje), 0, (struct sockaddr*)&socket_destino, sizeof(socket_destino));
        if (enviados < 0) {
            printf("ERROR en sendto() \n");
            close(descriptor_socket_origen);
            exit(EXIT_FAILURE);
        } else {
            printf("Solicitud enviada al servidor.\n");
        }

        // Recibir la respuesta del servidor
        destino_tam = sizeof(socket_destino);
        recibidos = recvfrom(descriptor_socket_origen, respuesta, sizeof(respuesta), 0, (struct sockaddr*)&socket_destino, &destino_tam);
        if (recibidos < 0) {
            printf("ERROR en recvfrom() \n");
            close(descriptor_socket_origen);
            exit(EXIT_FAILURE);
        } else {
            printf("Respuesta del servidor: %s\n", respuesta);
        }

    } while (1);

    // Cerrar el socket del cliente
    printf("\nCliente termina.\n");
    close(descriptor_socket_origen);
    exit(EXIT_SUCCESS);
}
