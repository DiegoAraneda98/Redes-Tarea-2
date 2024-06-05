/* SERVIDOR - Reserva y consulta de citas médicas con RUT y especialidades */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>  // Biblioteca para trabajar con expresiones regulares

#define MAX_TAM_MENSAJE 200
#define MAX_CITAS 100

typedef struct {
    char especialidad[50];
    char nombre_paciente[50];
    char rut[12];
    char fecha[11];
    char hora[6];
} Cita;

Cita calendario[MAX_CITAS];
int total_citas = 0;

int descriptor_socket_servidor;

void catch(int sig) {
    printf("***Señal: %d atrapada!\n", sig);
    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    exit(EXIT_SUCCESS);
}

void validar_rut(char *rut, char *respuesta, struct sockaddr_in socket_cliente, socklen_t destino_tam) {
    regex_t regex;
    int reti;

    // Compilar la expresión regular para el formato del RUT
    reti = regcomp(&regex, "[0-9]{7,8}-[0-9Kk]", REG_EXTENDED);
    if (reti) {
        printf("ERROR: No se pudo compilar la expresión regular para el RUT\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Validar el RUT ingresado
        reti = regexec(&regex, rut, 0, NULL, 0);
        if (!reti) {
            // Si el RUT es válido, romper el bucle
            regfree(&regex);
            break;
        } else {
            // Si el RUT es inválido, pedir nuevamente el RUT
            sprintf(respuesta, "ERROR: Formato de RUT inválido. El RUT debe tener el formato xxxxxxxx-x. Ingrese nuevamente el RUT:");
            sendto(descriptor_socket_servidor, respuesta, strlen(respuesta), 0, (struct sockaddr*)&socket_cliente, destino_tam);

            // Recibir nuevo RUT del cliente
            int recibidos = recvfrom(descriptor_socket_servidor, rut, 12, 0, (struct sockaddr*)&socket_cliente, &destino_tam);
            rut[recibidos] = '\0';
        }
    }
}

void reservar_cita(char *mensaje, char *respuesta, struct sockaddr_in socket_cliente, socklen_t destino_tam) {
    char especialidad[50], nombre[50], rut[12], fecha[11], hora[6];
    sscanf(mensaje, "RESERVAR %s %s %s %s %s", especialidad, nombre, rut, fecha, hora);

    // Validar el formato del RUT
    validar_rut(rut, respuesta, socket_cliente, destino_tam);

    for (int i = 0; i < total_citas; i++) {
        if (strcmp(calendario[i].especialidad, especialidad) == 0 &&
            strcmp(calendario[i].fecha, fecha) == 0 &&
            strcmp(calendario[i].hora, hora) == 0) {
            sprintf(respuesta, "ERROR: No se pudo reservar la cita. Puede que ya esté ocupada.");
            return;
        }
    }

    if (total_citas < MAX_CITAS) {
        strcpy(calendario[total_citas].especialidad, especialidad);
        strcpy(calendario[total_citas].nombre_paciente, nombre);
        strcpy(calendario[total_citas].rut, rut);
        strcpy(calendario[total_citas].fecha, fecha);
        strcpy(calendario[total_citas].hora, hora);
        total_citas++;
        sprintf(respuesta, "Cita reservada para %s (%s) en %s el %s a las %s.", nombre, rut, especialidad, fecha, hora);
    } else {
        sprintf(respuesta, "ERROR: No se pudo reservar la cita. Agenda llena.");
    }
}

void consultar_citas(char *mensaje, char *respuesta) {
    char rut[12];
    sscanf(mensaje, "CONSULTAR %s", rut);
    char citas[1000] = "Citas agendadas:\n";

    int found = 0;
    for (int i = 0; i < total_citas; i++) {
        if (strcmp(calendario[i].rut, rut) == 0) {
            char cita[200];
            sprintf(cita, "Especialidad: %s, Nombre: %s, Fecha: %s, Hora: %s\n",
                    calendario[i].especialidad, calendario[i].nombre_paciente,
                    calendario[i].fecha, calendario[i].hora);
            strcat(citas, cita);
            found = 1;
        }
    }

    if (!found) {
        strcpy(citas, "No tiene citas agendadas.");
    }

    strcpy(respuesta, citas);
}

void formatear_horas_disponibles(char *horas) {
    int primer_minuto = 8 * 60; // Primer minuto a las 08:00 a.m.
    int ultimo_minuto = 18 * 60; // Último minuto a las 18:00 p.m.

    char formato_horas[1000] = ""; // Buffer para el formato de las horas
    char hora[6]; // Buffer para la hora actual

    // Iterar sobre los minutos disponibles
    for (int minuto = primer_minuto; minuto <= ultimo_minuto; minuto += 30) {
        int h = minuto / 60; // Obtener la hora
        int m = minuto % 60; // Obtener los minutos

        // Formatear la hora actual
        sprintf(hora, "%02d:%02d", h, m);

        // Concatenar la hora formateada al formato de horas
        strcat(formato_horas, hora);

        // Agregar un guion si no es la última hora del día
        if (minuto != ultimo_minuto) {
            strcat(formato_horas, " - ");
        }
    }

    // Copiar el formato de horas al argumento "horas"
    strcpy(horas, formato_horas);
}

void consultar_horas_disponibles(char *mensaje, char *respuesta) {
    char especialidad[50], fecha[11];
    sscanf(mensaje, "HORAS_DISPONIBLES %s %s", especialidad, fecha);
    char horas[1000] = "Horas disponibles:\n";

    // Array para marcar las horas ocupadas
    int horas_ocupadas[24 * 60] = {0};

    // Marcar las horas ocupadas
    for (int i = 0; i < total_citas; i++) {
        if (strcmp(calendario[i].especialidad, especialidad) == 0 &&
            strcmp(calendario[i].fecha, fecha) == 0) {
            int h, m;
            sscanf(calendario[i].hora, "%d:%d", &h, &m);
            horas_ocupadas[h * 60 + m] = 1; // Marcar el tiempo ocupado
        }
    }

    // Formatear las horas disponibles
    char formato_horas[1000] = "";
    char hora[6];

    for (int minuto = 8 * 60; minuto <= 18 * 60; minuto += 30) {
        int h = minuto / 60;
        int m = minuto % 60;

        // Verificar si la hora está disponible
        if (!horas_ocupadas[minuto]) {
            sprintf(hora, "%02d:%02d", h, m);
            strcat(formato_horas, hora);
            strcat(formato_horas, " - ");
        }
    }

    // Eliminar el último guión y espacio
    formato_horas[strlen(formato_horas) - 3] = '\0';

    // Copiar las horas disponibles al mensaje de respuesta
    strcat(horas, formato_horas);
    strcat(horas, "\n");

    // Copiar el resultado al mensaje de respuesta
    strcpy(respuesta, horas);
}


int main(int argc, char *argv[]) {
    socklen_t destino_tam;
    struct sockaddr_in socket_servidor, socket_cliente;
    char mensaje_entrada[MAX_TAM_MENSAJE], mensaje_salida[MAX_TAM_MENSAJE];
    int recibidos, enviados;

    if (argc != 2) {
        printf("\n\nEl número de parámetros es incorrecto\n");
        printf("Use: %s <puerto>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    descriptor_socket_servidor = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor_socket_servidor == -1) {
        printf("ERROR: El socket del servidor no se ha creado correctamente!\n");
        exit(EXIT_FAILURE);
    }

    socket_servidor.sin_family = AF_INET;
    socket_servidor.sin_port = htons(atoi(argv[1]));
    socket_servidor.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(descriptor_socket_servidor, (struct sockaddr*)&socket_servidor, sizeof(socket_servidor)) == -1) {
        printf("ERROR al unir el socket a la dirección de la máquina servidora\n");
        close(descriptor_socket_servidor);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, catch);

    printf("\n***Servidor ACTIVO escuchando en el puerto: %s ...\n", argv[1]);

    while (1) {
        destino_tam = sizeof(socket_cliente);
        recibidos = recvfrom(descriptor_socket_servidor, mensaje_entrada, MAX_TAM_MENSAJE, 0, (struct sockaddr*)&socket_cliente, &destino_tam);
        if (recibidos < 0) {
            printf("ERROR en recvfrom()\n");
            exit(EXIT_FAILURE);
        }

        mensaje_entrada[recibidos] = '\0';

        if (strncmp(mensaje_entrada, "RESERVAR", 8) == 0) {
            reservar_cita(mensaje_entrada, mensaje_salida, socket_cliente, destino_tam);
        } else if (strncmp(mensaje_entrada, "CONSULTAR", 9) == 0) {
            consultar_citas(mensaje_entrada, mensaje_salida);
        } else if (strncmp(mensaje_entrada, "HORAS_DISPONIBLES", 17) == 0) {
            consultar_horas_disponibles(mensaje_entrada, mensaje_salida);
        } else if (strcmp(mensaje_entrada, "terminar();") == 0) {
            strcpy(mensaje_salida, "Conexión terminada por el cliente.");
        } else {
            strcpy(mensaje_salida, "Comando no reconocido.");
        }

        enviados = sendto(descriptor_socket_servidor, mensaje_salida, strlen(mensaje_salida), 0, (struct sockaddr*)&socket_cliente, destino_tam);
        if (enviados < 0) {
            printf("ERROR en sendto()\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("***Cerrando servicio ...\n");
    close(descriptor_socket_servidor);
    printf("***Servicio cerrado.\n");
    return 0;
}

