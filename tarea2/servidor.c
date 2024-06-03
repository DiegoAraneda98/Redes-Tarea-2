void manejar_cliente(int client_socket) {
    char buffer[1024];
    int opcion, consulta_id, hora_index;
    Reserva nueva_reserva;

    while (1) {
        bzero(buffer, 1024);
        read(client_socket, &opcion, sizeof(opcion));  // Cambiado para recibir entero directamente

        switch (opcion) {
            case 1: // Reservar hora
                mostrar_horarios(client_socket);
                
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                hora_index = atoi(buffer) - 1;

                if (hora_index < 0 || hora_index >= sizeof(horarios) / sizeof(horarios[0]) || !horarios[hora_index].disponible) {
                    strcpy(buffer, "Hora no válida o no disponible.\n");
                    write(client_socket, buffer, strlen(buffer));
                    break;
                }

                write(client_socket, "Ingrese nombre: ", strlen("Ingrese nombre: "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                strcpy(nueva_reserva.nombre, buffer);

                write(client_socket, "Ingrese fecha (dd-mm-aaaa): ", strlen("Ingrese fecha (dd-mm-aaaa): "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                strcpy(nueva_reserva.fecha, buffer);

                strcpy(nueva_reserva.hora, horarios[hora_index].hora);
                write(client_socket, "Ingrese categoria: ", strlen("Ingrese categoria: "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                strcpy(nueva_reserva.categoria, buffer);

                agregar_reserva(nueva_reserva, hora_index);

                sprintf(buffer, "Su hora ha sido reservada con éxito para:\nID: %d, Nombre: %s, Fecha: %s, Hora: %s, Categoria: %s\n",
                        nueva_reserva.consulta_id, nueva_reserva.nombre, nueva_reserva.fecha, nueva_reserva.hora, nueva_reserva.categoria);
                write(client_socket, buffer, strlen(buffer));
                break;
            case 2: // Consultar reserva
                write(client_socket, "Ingrese ID de consulta: ", strlen("Ingrese ID de consulta: "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
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
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                consulta_id = atoi(buffer);

                cancelar_reserva(consulta_id);
                strcpy(buffer, "Reserva cancelada.\n");
                write(client_socket, buffer, strlen(buffer));
                break;
            case 4: // Consultar horario
                write(client_socket, "Ingrese mes (1-12): ", strlen("Ingrese mes (1-12): "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
                int mes = atoi(buffer);

                write(client_socket, "Ingrese día: ", strlen("Ingrese día: "));
                bzero(buffer, 1024);
                read(client_socket, buffer, 1024);
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
    int server_socket, client_socket, addr_size;
    struct sockaddr_in server_addr, client_addr;

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
