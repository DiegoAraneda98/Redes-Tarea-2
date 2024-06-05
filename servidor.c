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

void reservar_cita(char *mensaje, char *respuesta) {
    char especialidad[50], nombre[50], rut[12], fecha[11], hora[6];
    sscanf(mensaje, "RESERVAR %s %s %s %s %s", especialidad, nombre, rut, fecha, hora);

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

void consultar_horas_disponibles(char *mensaje, char *respuesta) {
    char especialidad[50], fecha[11];
    sscanf(mensaje, "HORAS_DISPONIBLES %s %s", especialidad, fecha);
    char horas[1000] = "Horas disponibles:\n";

    int horas_ocupadas[24 * 60] = {0};  // Matriz para las 1440 posibles minutos del día

    // Calcular el índice del primer minuto a partir de las 08:00 a.m. (480 minutos)
    int primer_minuto = 8 * 60;

    // Calcular el índice del último minuto hasta las 18:00 p.m. (1080 minutos)
    int ultimo_minuto = 18 * 60;

    // Marcar como ocupadas las horas que ya tienen citas agendadas
    for (int i = 0; i < total_citas; i++) {
        if (strcmp(calendario[i].especialidad, especialidad) == 0 &&
            strcmp(calendario[i].fecha, fecha) == 0) {
            int h, m;
            sscanf(calendario[i].hora, "%d:%d", &h, &m);
            horas_ocupadas[h * 60 + m] = 1;  // Marcar el tiempo ocupado
        }
    }

    // Generar las horas disponibles en intervalos de media hora
    for (int minuto = primer_minuto; minuto <= ultimo_minuto; minuto += 30) {
        int h = minuto / 60;  // Obtener la hora correspondiente al índice del minuto
        int m = minuto % 60;  // Obtener los minutos correspondientes al índice del minuto

        // Verificar si el minuto está disponible
        if (!horas_ocupadas[minuto]) {
            char hora[6];
            sprintf(hora, "%02d:%02d\n", h, m);
            strcat(horas, hora);
        }
    }

    // Copiar la cadena de horas disponibles en la respuesta
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
            reservar_cita(mensaje_entrada, mensaje_salida);
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
