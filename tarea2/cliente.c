/* CLIENTE - Reserva y consulta de citas médicas con RUT y especialidades */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_TAM_MENSAJE 200

char mensaje[MAX_TAM_MENSAJE];

void catch(int sig) {
    strcpy(mensaje, "terminar();");
}

int main(int argc, char *argv[]) {
    int descriptor_socket_origen;
    struct sockaddr_in socket_origen, socket_destino;
    socklen_t destino_tam;
    char respuesta[MAX_TAM_MENSAJE];
    int recibidos, enviados;

    if (argc != 3) {
        printf("\n\nEl número de parámetros es incorrecto\n");
        printf("Use: %s <IP_servidor> <puerto>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    descriptor_socket_origen = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor_socket_origen == -1) {
        printf("ERROR: El socket del cliente no se ha creado correctamente!\n");
        exit(EXIT_FAILURE);
    }

    socket_origen.sin_family = AF_INET;
    socket_origen.sin_port = htons(0);
    socket_origen.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(descriptor_socket_origen, (struct sockaddr*)&socket_origen, sizeof(socket_origen)) == -1) {
        printf("ERROR al unir el socket a la dirección de la máquina cliente\n");
        close(descriptor_socket_origen);
        exit(EXIT_FAILURE);
    }

    socket_destino.sin_family = AF_INET;
    socket_destino.sin_addr.s_addr = inet_addr(argv[1]);
    socket_destino.sin_port = htons(atoi(argv[2]));

    signal(SIGINT, catch);

    while (1) {
        int opcion;
        printf("Elija una acción:\n1. Reservar una cita\n2. Ver mis citas\n3. Ver horas disponibles\nOpción: ");
        scanf("%d", &opcion);

        if (opcion == 1) {
            int especialidad_opcion;
            char nombre[50], rut[12], fecha[11], hora[6], especialidad[20];

            printf("Elija una especialidad:\n1. Cardiología\n2. Pediatría\n3. Dermatología\n4. Ginecología\nOpción: ");
            scanf("%d", &especialidad_opcion);

            switch (especialidad_opcion) {
                case 1:
                    strcpy(especialidad, "Cardiología");
                    break;
                case 2:
                    strcpy(especialidad, "Pediatría");
                    break;
                case 3:
                    strcpy(especialidad, "Dermatología");
                    break;
                case 4:
                    strcpy(especialidad, "Ginecología");
                    break;
                default:
                    printf("Especialidad no válida.\n");
                    continue;
            }

            printf("Ingrese su nombre: ");
            scanf("%s", nombre);
            printf("Ingrese su RUT (xxxxxxxx-x): ");
            scanf("%s", rut);
            printf("Ingrese la fecha de la cita (YYYY-MM-DD): ");
            scanf("%s", fecha);
            printf("Ingrese la hora de la cita (HH:MM): ");
            scanf("%s", hora);

            snprintf(mensaje, sizeof(mensaje), "RESERVAR %s %s %s %s %s", especialidad, nombre, rut, fecha, hora);

        } else if (opcion == 2) {
            char rut[12];

            printf("Ingrese su RUT (xxxxxxxx-x): ");
            scanf("%s", rut);

            snprintf(mensaje, sizeof(mensaje), "CONSULTAR %s", rut);

        } else if (opcion == 3) {
            int especialidad_opcion;
            char especialidad[20], fecha[11];

            printf("Elija una especialidad:\n1. Cardiología\n2. Pediatría\n3. Dermatología\n4. Ginecología\nOpción: ");
            scanf("%d", &especialidad_opcion);

            switch (especialidad_opcion) {
                case 1:
                    strcpy(especialidad, "Cardiología");
                    break;
                case 2:
                    strcpy(especialidad, "Pediatría");
                    break;
                case 3:
                    strcpy(especialidad, "Dermatología");
                    break;
                case 4:
                    strcpy(especialidad, "Ginecología");
                    break;
                default:
                    printf("Especialidad no válida.\n");
                    continue;
            }

            printf("Ingrese la fecha (YYYY-MM-DD): ");
            scanf("%s", fecha);

            snprintf(mensaje, sizeof(mensaje), "HORAS_DISPONIBLES %s %s", especialidad, fecha);

        } else {
            printf("Opción no válida.\n");
            continue;
        }

        enviados = sendto(descriptor_socket_origen, mensaje, strlen(mensaje), 0, (struct sockaddr*)&socket_destino, sizeof(socket_destino));
        if (enviados < 0) {
            printf("ERROR en sendto()\n");
            close(descriptor_socket_origen);
            exit(EXIT_FAILURE);
        }

        destino_tam = sizeof(socket_destino);
        recibidos = recvfrom(descriptor_socket_origen, respuesta, sizeof(respuesta) - 1, 0, (struct sockaddr*)&socket_destino, &destino_tam);
        if (recibidos < 0) {
            printf("ERROR en recvfrom()\n");
            close(descriptor_socket_origen);
            exit(EXIT_FAILURE);
        }

        respuesta[recibidos] = '\0';
        printf("Respuesta del servidor:\n%s\n", respuesta);

        if (strcmp(mensaje, "terminar();") == 0) {
            printf("***Cliente terminó conexión con servidor.\n");
            close(descriptor_socket_origen);
            exit(EXIT_SUCCESS);
        }
    }

    close(descriptor_socket_origen);
    return 0;
}
