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
#include <regex.h>

#define MAX_TAM_MENSAJE 200

char mensaje[MAX_TAM_MENSAJE];

void catch(int sig) {
    strcpy(mensaje, "terminar();");
}

int validar_rut(const char *rut) {
    regex_t regex;
    int reti;
    // Expresión regular para el formato del RUT: 8 dígitos, un guion, un dígito o la letra K
    reti = regcomp(&regex, "^[0-9]{8}-[0-9Kk]$", REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "No se pudo compilar la expresión regular\n");
        exit(EXIT_FAILURE);
    }

    // Comprobar el RUT contra la expresión regular
    reti = regexec(&regex, rut, 0, NULL, 0);
    regfree(&regex);

    if (!reti) {
        return 1; // RUT válido
    } else {
        return 0; // RUT no válido
    }
}

int validar_fecha(const char *fecha) {
    regex_t regex;
    int reti;
    // Expresión regular para el formato de fecha: YYYY-MM-DD
    reti = regcomp(&regex, "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$", REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "No se pudo compilar la expresión regular\n");
        exit(EXIT_FAILURE);
    }

    // Comprobar la fecha contra la expresión regular
    reti = regexec(&regex, fecha, 0, NULL, 0);
    regfree(&regex);

    if (!reti) {
        return 1; // Fecha válida
    } else {
        return 0; // Fecha no válida
    }
}

int validar_hora(const char *hora) {
    regex_t regex;
    int reti;
    // Expresión regular para el formato de hora: HH:MM (24 horas)
    reti = regcomp(&regex, "^([01]?[0-9]|2[0-3]):[0-5][0-9]$", REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "No se pudo compilar la expresión regular\n");
        exit(EXIT_FAILURE);
    }

    // Comprobar la hora contra la expresión regular
    reti = regexec(&regex, hora, 0, NULL, 0);
    regfree(&regex);

    if (!reti) {
        return 1; // Hora válida
    } else {
        return 0; // Hora no válida
    }
}

void reservar_cita() {
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
            return;
    }

    printf("Ingrese su nombre: ");
    scanf("%s", nombre);

    while (1) {
        printf("Ingrese su RUT (xxxxxxxx-x): ");
        scanf("%s", rut);
        if (validar_rut(rut)) {
            break;
        } else {
            printf("RUT no válido. Asegúrese de que el formato sea xxxxxxxx-x y que no contenga puntos.\n");
        }
    }

    while (1) {
        printf("Ingrese la fecha de la cita (YYYY-MM-DD): ");
        scanf("%s", fecha);
        if (validar_fecha(fecha)) {
            break;
        } else {
            printf("Fecha no válida. Asegúrese de que el formato sea YYYY-MM-DD.\n");
        }
    }

    while (1) {
        printf("Ingrese la hora de la cita (HH:MM): ");
        scanf("%s", hora);
        if (validar_hora(hora)) {
            break;
        } else {
            printf("Hora no válida. Asegúrese de que el formato sea HH:MM (24 horas).\n");
        }
    }

    snprintf(mensaje, sizeof(mensaje), "RESERVAR %s %s %s %s %s", especialidad, nombre, rut, fecha, hora);
}

void consultar_citas() {
    char rut[12];

    while (1) {
        printf("Ingrese su RUT (xxxxxxxx-x): ");
        scanf("%s", rut);
        if (validar_rut(rut)) {
            break;
        } else {
            printf("RUT no válido. Asegúrese de que el formato sea xxxxxxxx-x y que no contenga puntos.\n");
        }
    }

    snprintf(mensaje, sizeof(mensaje), "CONSULTAR %s", rut);
}

void consultar_horas_disponibles() {
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
            return;
    }

    while (1) {
        printf("Ingrese la fecha (YYYY-MM-DD): ");
        scanf("%s", fecha);
        if (validar_fecha(fecha)) {
            break;
        } else {
            printf("Fecha no válida. Asegúrese de que el formato sea YYYY-MM-DD.\n");
        }
    }

    snprintf(mensaje, sizeof(mensaje), "HORAS_DISPONIBLES %s %s", especialidad, fecha);
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
            reservar_cita();
        } else if (opcion == 2) {
            consultar_citas();
        } else if (opcion == 3) {
            consultar_horas_disponibles();
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