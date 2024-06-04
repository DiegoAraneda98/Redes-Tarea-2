#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

typedef struct {
    char nombre[100];
    char fecha[11];  // formato dd-mm-aaaa
    char hora[6];    // formato hh:mm
    char categoria[50];
    int consulta_id;
} Reserva;

typedef struct {
    char hora[6];
    bool disponible;
} Horario;

Horario horarios[] = {
    {"08:00", true}, {"08:30", true}, {"09:00", true}, {"09:30", true}, {"10:00", true},
    {"10:30", true}, {"11:00", true}, {"11:30", true}, {"12:00", true}, {"12:30", true},
    {"13:00", true}, {"13:30", true}, {"14:00", true}, {"14:30", true}, {"15:00", true},
    {"15:30", true}, {"16:00", true}, {"16:30", true}, {"17:00", true}, {"17:30", true},
    {"18:00", true}, {"18:30", true}, {"19:00", true}, {"19:30", true}, {"20:00", true}
};

Reserva reservas[100];
int reserva_count = 0;
int consulta_id_counter = 1;

void mostrar_horarios(int client_socket) {
    char buffer[1024];
    strcpy(buffer, "Horario de la categoria:\nHoras disponibles:\n");
    write(client_socket, buffer, strlen(buffer));

    for (int i = 0; i < sizeof(horarios) / sizeof(horarios[0]); i++) {
        if (horarios[i].disponible) {
            sprintf(buffer, "%d. %s\n", i + 1, horarios[i].hora);
            write(client_socket, buffer, strlen(buffer));
        }
    }
    write(client_socket, "\n", 1);
}

void agregar_reserva(Reserva r, int hora_index) {
    r.consulta_id = consulta_id_counter++;
    reservas[reserva_count++] = r;
    horarios[hora_index].disponible = false;
}

Reserva* consultar_reserva(int consulta_id) {
    for (int i = 0; i < reserva_count; i++) {
        if (reservas[i].consulta_id == consulta_id) {
            return &reservas[i];
        }
    }
    return NULL;
}

void cancelar_reserva(int consulta_id) {
    for (int i = 0; i < reserva_count; i++) {
        if (reservas[i].consulta_id == consulta_id) {
            for (int j = 0; j < sizeof(horarios) / sizeof(horarios[0]); j++) {
                if (strcmp(horarios[j].hora, reservas[i].hora) == 0) {
                    horarios[j].disponible = true;
                    break;
                }
            }
            reservas[i] = reservas[--reserva_count];
            return;
        }
    }
}

void mostrar_horarios_disponibles(int client_socket, int dia, int mes) {
    char buffer[1024];
    sprintf(buffer, "Horas disponibles para el dia %02d/%02d:\n", dia, mes);
    write(client_socket, buffer, strlen(buffer));

    for (int i = 0; i < sizeof(horarios) / sizeof(horarios[0]); i++) {
        if (horarios[i].disponible) {
            sprintf(buffer, "%d. %s\n", i + 1, horarios[i].hora);
            write(client_socket, buffer, strlen(buffer));
        }
    }
    write(client_socket, "\n", 1);
}

void manejar_cliente(int client_socket) {
    char buffer[1024];
    int opcion, consulta_id, hora_index;
    Reserva nueva_reserva;

    while (1) {
        bzero(buffer, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));
        opcion = atoi(buffer);

        switch (opcion) {
            case 1: // Reservar hora
                mostrar_horarios(client_socket);

                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                hora_index = atoi(buffer) - 1;

                // Leer datos de la reserva
                write(client_socket, "Ingrese nombre: ", strlen("Ingrese nombre: "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                buffer[strcspn(buffer, "\n")] = 0;  // Eliminar nueva línea
                strcpy(nueva_reserva.nombre, buffer);

                write(client_socket, "Ingrese fecha (dd-mm-aaaa): ", strlen("Ingrese fecha (dd-mm-aaaa): "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                buffer[strcspn(buffer, "\n")] = 0;  // Eliminar nueva línea
                strcpy(nueva_reserva.fecha, buffer);

                strcpy(nueva_reserva.hora, horarios[hora_index].hora);
                write(client_socket, "Ingrese categoria: ", strlen("Ingrese categoria: "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                buffer[strcspn(buffer, "\n")] = 0;  // Eliminar nueva línea
                strcpy(nueva_reserva.categoria, buffer);

                agregar_reserva(nueva_reserva, hora_index);

                sprintf(buffer, "Su hora ha sido reservada con éxito para:\nID: %d, Nombre: %s, Fecha: %s, Hora: %s, Categoria: %s\n",
                        nueva_reserva.consulta_id, nueva_reserva.nombre, nueva_reserva.fecha, nueva_reserva.hora, nueva_reserva.categoria);
                write(client_socket, buffer, strlen(buffer));
                break;
            case 2: // Consultar reserva
                write(client_socket, "Ingrese ID de consulta: ", strlen("Ingrese ID de consulta: "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                consulta_id = atoi(buffer);

                Reserva* res = consultar_reserva(consulta_id);
                if (res) {
                    sprintf(buffer, "ID: %d, Nombre: %s, Fecha: %s, Hora: %s, Categoria: %s\n",
                            res->consulta_id, res->nombre, res->fecha, res->hora, res->categoria);
                } else {
                    strcpy(buffer, "Reserva no encontrada.\n");
                }
                write(client_socket, buffer, strlen(buffer));
                break;
            case 3: // Cancelar reserva
                write(client_socket, "Ingrese ID de consulta: ", strlen("Ingrese ID de consulta: "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                consulta_id = atoi(buffer);

                cancelar_reserva(consulta_id);
                strcpy(buffer, "Reserva cancelada.\n");
                write(client_socket, buffer, strlen(buffer));
                break;
            case 4: // Consultar horario
                write(client_socket, "Ingrese mes (1-12): ", strlen("Ingrese mes (1-12): "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                int mes = atoi(buffer);

                write(client_socket, "Ingrese día: ", strlen("Ingrese día: "));
                bzero(buffer, sizeof(buffer));
                read(client_socket, buffer, sizeof(buffer));
                int dia = atoi(buffer);

                mostrar_horarios_disponibles(client_socket, dia, mes);
                break;
            case 5:
                close(client_socket);
                return;
            default:
                strcpy(buffer, "Opción no válida.\n");
                write(client_socket, buffer, strlen(buffer));
        }
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("Servidor escuchando en el puerto 8080...\n");

    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        printf("Cliente conectado...\n");

        if (fork() == 0) {
            close(server_socket);
            manejar_cliente(client_socket);
            exit(0);
        } else {
            close(client_socket);
        }
    }

    return 0;
}
