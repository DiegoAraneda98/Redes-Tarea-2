#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

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


int validar_formato_rut(const char *rut) {
    int i = 0;
    // Verificar que los primeros 8 caracteres sean dígitos
    for (i = 0; i < 8; i++) {
        if (!isdigit(rut[i])) {
            return 0;
        }
    }
    // Verificar que el noveno caracter sea un guión
    if (rut[8] != '-') {
        return 0;
    }
    // Verificar que el décimo caracter sea un dígito o una 'X'
    if (!isdigit(rut[9]) && rut[9] != 'X') {
        return 0;
    }
    // Verificar que el largo total sea 10 caracteres
    if (strlen(rut) != 10) {
        return 0;
    }
    return 1;
}


int validar_formato_hora(const char *hora) {
    if (strlen(hora) != 5) return 0; // Debe tener 5 caracteres
    if (hora[2] != ':') return 0; // El tercer carácter debe ser ':'
    if (!isdigit(hora[0]) || !isdigit(hora[1]) || !isdigit(hora[3]) || !isdigit(hora[4])) return 0; // Los demás deben ser dígitos

    int horas = (hora[0] - '0') * 10 + (hora[1] - '0');
    int minutos = (hora[3] - '0') * 10 + (hora[4] - '0');
    
    if (horas < 0 || horas > 23 || minutos < 0 || minutos > 59) return 0; // Rango válido de horas y minutos

    return 1;
}

int validar_formato_fecha(const char *fecha) {
    if (strlen(fecha) != 10) return 0; // Debe tener 10 caracteres
    if (fecha[4] != '-' || fecha[7] != '-') return 0; // El cuarto y séptimo carácter deben ser '-'
    if (!isdigit(fecha[0]) || !isdigit(fecha[1]) ||
        !isdigit(fecha[2]) || !isdigit(fecha[3]) ||
        !isdigit(fecha[5]) || !isdigit(fecha[6]) ||
        !isdigit(fecha[8]) || !isdigit(fecha[9])) return 0; // Los demás deben ser dígitos

    int anio = (fecha[0] - '0') * 1000 + (fecha[1] - '0') * 100 + (fecha[2] - '0') * 10 + (fecha[3] - '0');
    int mes = (fecha[5] - '0') * 10 + (fecha[6] - '0');
    int dia = (fecha[8] - '0') * 10 + (fecha[9] - '0');
    
    if (anio < 0 || mes < 1 || mes > 12 || dia < 1 || dia > 31) return 0; // Rango válido de año, mes y día

    return 1;
}

void reservar_cita(char *mensaje, char *respuesta, struct sockaddr_in socket_cliente, socklen_t destino_tam) {
    char especialidad[50], nombre[50], rut[12], fecha[11], hora[6];
    sscanf(mensaje, "RESERVAR %s %s %s %s %s", especialidad, nombre, rut, fecha, hora);

    // Variable para almacenar los errores
    char errores[200] = "";

    // Validar el formato del rut
    if (!validar_formato_rut(rut)) {
        strcat(errores, "ERROR: Formato de rut incorrecto. Debe ser xxxxxxxx-x o xxxxxxxx-X.\n");
    }

    // Validar el formato de la fecha
    if (!validar_formato_fecha(fecha)) {
        strcat(errores, "ERROR: Formato de fecha incorrecto. Debe ser YYYY-MM-DD.\n");
    }

    // Validar el formato de la hora
    if (!validar_formato_hora(hora)) {
        strcat(errores, "ERROR: Formato de hora incorrecto. Debe ser HH:MM.\n");
    }

    // Verificar si se detectaron errores
    if (strlen(errores) > 0) {
        strcpy(respuesta, errores); // Copiar los errores al mensaje de respuesta
        return;
    }
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

    // Validar el formato del rut
    if (!validar_formato_rut(rut)) {
        // Ignorar el mensaje y continuar esperando
        return;
    }

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

    int h, m; // Declarar las variables aquí

    // Iterar sobre los minutos disponibles
    for (int minuto = primer_minuto; minuto <= ultimo_minuto; minuto += 30) {
        h = minuto / 60; // Obtener la hora
        m = minuto % 60; // Obtener los minutos

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
    if (strlen(formato_horas) > 3) {
        formato_horas[strlen(formato_horas) - 3] = '\0';
    }

    // Copiar las horas disponibles al mensaje de respuesta
    strcat(horas, formato_horas);
    strcat(horas, "\n");

    // Copiar resultado al mensaje de respuesta
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


