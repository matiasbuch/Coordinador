#include "socket-connection.h"


/*
		FD_SET(int fd, fd_set *set); Add fd to the set.
		FD_CLR(int fd, fd_set *set); Remove fd from the set.
		FD_ISSET(int fd, fd_set *set); Return true if fd is in the set.
		FD_ZERO(fd_set *set); Clear all entries from the set.
	 */

/*
 * 	METODOS PRIVADOS
 *
 * 	Estos metodos no deben ser utilizados por el programador. Son solo de uso interno de la biblioteca
 *
 * */

/*
 * No debe ser utilizado por el programador. Es un metodo privado
 */
static void _sigchld_handler(int s);


static void _destroy_socket(t_SCB *socket);


static bool _find_by_socket_fd(void* socket);
static bool _find_by_pid(void* socket);

/*
 * Administra la solicitud de conexion de un cliente a un socket servidor.
 * Establece esa conexion y agrega al descriptor del socket cliente a la lista de conexiones activas
 */
static t_SCB *_handle_connection_request(t_connection_mannager *self);

/*
 * Carga la esructura de direcciones IPv4 o IPv6 para la direccion IP dada
 */
void *get_in_addr(struct sockaddr *sa);

// Devuelve la direccion IP para IPv4 e IPv6
static char *get_ip_str(const struct sockaddr *sa);





static void _sigchld_handler(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}


static void _destroy_socket(t_SCB *socket) {
	free(socket->host);
}


static bool _find_by_socket_fd(void* connection) {
	return ((t_SCB*)connection)->socket_fd == (int)global_seach_condition;
}


static bool _find_by_pid(void* connection) {
	return ((t_SCB*)connection)->pid == (process_id_t)global_seach_condition;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


t_connection_mannager *create_conn_mngr(int max_data_size) {

	t_connection_mannager * self = malloc(sizeof(t_connection_mannager));

	self->max_fd = -1;
	self->max_data_size = max_data_size;
	//FD_ZERO(&master_set);s
	FD_ZERO(&(self->master_set));
	self->remotes = list_create();

	self->server = malloc(sizeof(t_SCB));
	self->server->pid = 0;
	self->server->socket_fd = -1;
	self->server->port = 0;
	self->server->connected = false;
	self->server->type = SERVER;
	self->back_log = 0;

	return self;
}


void destroy_conn_mngr(t_connection_mannager *self) {
	disconnect_all(self);

	for (int i = 0; i < ((t_list *)(self->remotes))->elements_count -1; i++) {
		_destroy_socket(list_get(self->remotes, i));
	}
	list_destroy(self->remotes);

	for (int i = 0; i < ((t_list *)self->remotes)->elements_count -1; i++) {
			_destroy_socket(list_get(self->clients, i));
	}
	list_destroy(self->clients);

	_destroy_socket(self->server);
	free(&(self->master_set));
	free(self);
};



t_SCB *connect_remote(t_connection_mannager *self, char *host, int port) {

	int sockfd; //, numbytes;
	//char buf[self->max_data_size];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	t_SCB * client = malloc(sizeof(t_SCB));
	client->connected = false;
	client->host = host;
	client->port = port;
	client->socket_fd = -1;
	client->type = CLIENT;

	/* verificar formato de IP valida
	if (argc != 2) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}
	*/

	// Obtiene los datos del servidor para IPv4 e IPv6
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char strPort[5];
	sprintf(strPort, "%d", client->port);

	if ((rv = getaddrinfo(client->host, strPort, &hints, &servinfo)) != 0) {
	//if ((rv = getaddrinfo(client->host, "1", &hints, &servinfo)) != 0) {
		error_show("getaddrinfo: %s\n", gai_strerror(rv));
		//freeaddrinfo(servinfo); ???
		return client;
	}

	// Intenta conectar con la info obtenida anteriormente del servidor. Conecta con la primera que pueda
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// Crea el socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			error_show("Socket:[%d]: creado para conexion a remoto:[%s:%d]", sockfd, host, port);
			continue;
		}

		// Conecta
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			error_show("Intento de conexion socket:[%d]: a remoto:[%s:%d] fallida", sockfd, host, port);
			continue;
		}
		else {
			error_show("Conexion establecida con socket:[%d]: a remoto:[%s:%d]", sockfd, host, port);
		}

		break;
	}

	// Salgo si no logro conectar con ninguna
	if (p == NULL) {
		error_show("Intento de conexion fallida a remoto:[%s:%d]", host, port);
	}
	else {
		// Si entro aca es porque logro conectar
		client->connected = true;
		client->socket_fd = sockfd;
		self->max_fd = sockfd;
		list_add(self->clients, client);
		FD_SET(sockfd, &(self->master_set));
		//FD_SET(sockfd, &master_set);

		// log
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		error_show("client: connecting to %s\n", s);
	}

	// Libero la estructura del info del server
	freeaddrinfo(servinfo);

	return client;
}



t_SCB *open_server(t_connection_mannager *self, int port, int back_log, process_id_t pserver_id) {

	if (self == NULL) {
		error_show("El controlador de conecciones no fue inicializado\n");
		return NULL;
	}

	if (self->server->connected) {
		error_show("El servidor ya esta conectado\n");
		return NULL;
	}

	int socket_fd = -1;
	//socklen_t addrlen;
	int rv;
	//int yes=1; // for setsockopt() SO_REUSEADDR, below
	struct addrinfo hints, *ai, *p;

	self->server->pid = pserver_id;
	self->server->port = port;
	self->back_log = back_log;

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char strPort[5];
	sprintf(strPort, "%d", self->server->port);

	// Info IP local
	if ((rv = getaddrinfo(NULL, strPort, &hints, &ai)) != 0) {
		error_show("Error[%d] en seteo de datos de conexion para servidor\n", gai_strerror(rv));
		freeaddrinfo(ai);
		return self->server;
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socket_fd < 0) {
			continue;
		}
		else {
			error_show("Puerto:[%d] de conexion establecido para socket:[%d]\n", socket_fd, port);
		}

		// lose the pesky "address already in use" error message
		//setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
			close(socket_fd);
			error_show("Intento de enlace de puerto:[%d] de conexion para socket:[%d] fallido\n", socket_fd, port);
			continue;
		}
		else {
			self->max_fd = socket_fd;
			self->server->socket_fd = socket_fd;
			self->clients = list_create();
			FD_SET(socket_fd, &(self->master_set));
			error_show("Enlace establecido de puerto:[%d] de conexion para socket:[%d]\n", socket_fd, port);
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		error_show("selectserver: failed to bind\n");
	}

	// all done with this
	freeaddrinfo(ai);

	return self->server;
}



t_SCB *start_server(t_connection_mannager *self) {

	if (self == NULL) {
		error_show("El controlador de conexiones no esta inicializado");
		return NULL;
	}

	if (self->server->socket_fd < 0) {
		error_show("El servidor no fue iniciado correctamente. server_fd = %d", self->server->socket_fd);
		return self->server;
	}

	if (self->server->connected) {
		error_show("El servidor ya esta conectado");
		return self->server;
	}

	if (listen(self->server->socket_fd, self->back_log) != 0) {
		error_show("Ocurrio un error al iniciar servidor. Error = %d", errno);
		return self->server;
	}
	else {
		self->server->connected = true;
	}

	return self->server;
}


static char *get_ip_str(const struct sockaddr *sa) {
	size_t maxlen;
	char *s;

	switch(sa->sa_family) {
		case AF_INET:
			s = malloc(sizeof(INET_ADDRSTRLEN));
			maxlen = sizeof(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
			break;
		case AF_INET6:
			s = malloc(sizeof(INET6_ADDRSTRLEN));
			maxlen = sizeof(INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
			break;
		default:
			strncpy(s, "Unknown AF", maxlen);
			return NULL;
	}

	return s;
}



static t_SCB *_handle_connection_request(t_connection_mannager *self) {
	socklen_t addrlen;
	int new_fd;
	// Esta estructura soporta tanto IPv4 e IPv6
	struct sockaddr_storage remoteaddr;
	//char remoteIP[INET6_ADDRSTRLEN];
	//int port;

	// handle new connections
	addrlen = sizeof remoteaddr;
	new_fd = accept(self->server->socket_fd, (struct sockaddr *)&remoteaddr, &addrlen);

	if (new_fd == -1) {
		error_show("Error aceptando conexion");
		return NULL;

	} else {
		// add to master set
		FD_SET(new_fd, &(self->master_set));

		// keep track of the max
		if (new_fd > self->max_fd) {
			self->max_fd = new_fd;
		}

		t_SCB *client = malloc(sizeof(t_SCB));
		client->connected = true;
		client->socket_fd = new_fd;
		client->type = CLIENT;

		client->host = get_ip_str(get_in_addr((struct sockaddr*)&remoteaddr));

		if (remoteaddr.ss_family == AF_INET) {
			client->port = ((struct sockaddr_in6*)&remoteaddr)->sin6_port;
		}
		else {
			client->port = -1;
		}

		list_add(self->clients, client);

		error_show("selectserver: new connection from %s on socket %d\n", client->host, client->socket_fd);
		return client;
	}

}


t_list *handle_incomming(t_connection_mannager *self, int time_out) {
	int i;

	// temp file descriptor list for select()
	fd_set read_fds, except_fds;
	struct timeval tv;
	tv.tv_sec = time_out;
	tv.tv_usec = 500000;

	FD_ZERO(&read_fds);
	FD_ZERO(&except_fds);

	t_list *list = list_create();
	t_SCB * connection;


	 // hago la copia de los sockets que quiero atender
	read_fds = self->master_set;

	if (select(self->max_fd + 1, &read_fds, NULL, &except_fds, &tv) == -1) {
		error_show("select");
		list_destroy(list);
		return NULL;
	}

	// Reviso las conexiones activas
	for(i = 0; i <= self->max_fd; i++) {

		if (FD_ISSET(i, &read_fds)) {
			// Solicitudes de conexion al server
			if (i == self->server->socket_fd) {
				connection = _handle_connection_request(self);

				if (connection->connected)
					list_add(list, connection);
			}
			else {

				if ((connection = find_by_socket_fd(self, i)) != NULL)
					list_add(list, connection);
			}
		}
	}

	return list;
}


int send_all(t_SCB *connection, char *msg, int bytes) {
	int bytes_sent = 0;
	int total_bytes_sent = 0;
	bool stop = false;

	while (!stop) {
		bytes_sent = send(connection->socket_fd, &msg[total_bytes_sent], (total_bytes_sent - bytes_sent), 0);

		if (bytes_sent == -1) {
			stop = true;

			error_show("Error enviando datos al host:[%s] descriptor:[%d]. Bytes enviados:[%d] Bytes totales:[%d]\n",
								connection->host,
								connection->socket_fd,
								total_bytes_sent,
								bytes);
		}
		else {
			total_bytes_sent += bytes_sent;
			stop = (total_bytes_sent >= bytes);
		}
	}

	return total_bytes_sent;
}


int recieve_all(t_SCB *connection, char *msg, int bytes) {

	char buffer[bytes];
	int bytes_received = 0;
	int total_bytes_received = 0;
	bool stop = false;
	msg = malloc(bytes);

	while (!stop) {

		memset(buffer, 0, sizeof(buffer));
		bytes_received = recv(connection->socket_fd, buffer, (bytes - total_bytes_received), 0);

		if ((bytes_received == -1) || (bytes_received = 0)) {
			stop = true;

			error_show("Error recibiendo datos de host:[%s] descriptor:[%d]. Bytes recibidos:[%d] Bytes esperados:[%d]\n",
					connection->host,
					connection->socket_fd,
					(total_bytes_received + bytes_received),
					bytes);
		}
		else {
			memcpy(&msg[total_bytes_received], buffer, bytes_received);
			total_bytes_received += bytes_received;
			stop = (total_bytes_received >= bytes);
		}
	}

	free(buffer);
	return total_bytes_received;
}


// void* list_remove_by_condition(t_list *self, bool(*condition)(void*))

// void func ( void (*f)(int) );
/*
void print ( int x ) {
  printf("%d\n", x);
}
*/

/*
static bool _find_by_socket_fd(void* connection) {
	return ((t_SCB*)connection)->socket_fd == (int)global_seach_condition;
}


static bool _find_by_pid(void* connection) {
	return ((t_SCB*)connection)->pid == (process_id_t)global_seach_condition;
}
*/

void disconnect(t_connection_mannager *self, t_SCB *connection) {
	while (find_mutex)
		sleep(200);

	find_mutex = 1;
	global_seach_condition = &(connection->socket_fd);


	if (connection->type == CLIENT) {
		list_remove_by_condition(self->clients, _find_by_socket_fd);
	}
	else if (connection->type == REMOTE) {
		list_remove_by_condition(self->remotes, _find_by_socket_fd);
	}
	find_mutex = 0;

	close(connection->socket_fd);
	connection->connected = false;
	FD_CLR(connection->socket_fd, &(self->master_set));
}


void stop_server(t_connection_mannager *self) {
	if (self->server->connected) {
		close(self->server->socket_fd);
		FD_CLR(self->server->socket_fd, &(self->master_set));
		self->server->connected = false;
	}
}


int disconnect_all(t_connection_mannager *self) {
	/* TODO
	 * Cerrar todos los sockets de master_set
	 */

	t_SCB *connection;

	for (int i = 0; i < ((t_list*)(self->remotes))->elements_count; i++) {
		connection = (t_SCB*)list_get(self->clients, i);

		if ((connection != NULL) && (connection->connected)) {
			disconnect(self, connection);
		}
	}

	for (int i = 0; i < ((t_list*)(self->remotes))->elements_count; i++) {
		connection = (t_SCB*)list_get(self->remotes, i);

		if ((connection != NULL) && (connection->connected)) {
			disconnect(self, connection);
		}
	}

	stop_server(self);
	FD_ZERO(&(self->master_set));

	return 0;
}


t_SCB *find_by_socket_fd(t_connection_mannager *self,  int socket_fd) {
	while (find_mutex)
		sleep(200);

	find_mutex = 1;
	global_seach_condition = &socket_fd;
	t_SCB * connection = NULL;

	connection = (t_SCB *)list_find(self->clients, _find_by_socket_fd);

	if ((t_SCB*)connection == NULL)
		connection = (t_SCB *)list_find(self->remotes, _find_by_socket_fd);


	find_mutex = 0;
	return connection;
}


t_SCB *find_by_pid(t_connection_mannager *self,  process_id_t pid) {
	while (find_mutex)
			sleep(200);

	find_mutex = 1;
	global_seach_condition = &pid;
	t_SCB * connection = NULL;

	connection = (t_SCB*)list_find(self->clients, _find_by_pid);

	if ((t_SCB*)connection == NULL)
		connection = (t_SCB*)list_find(self->remotes, _find_by_pid);

	find_mutex = 0;
	return connection;
}


