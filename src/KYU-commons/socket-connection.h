#ifndef SOCKET_CONNECTION_H_
#define SOCKET_CONNECTION_H_

#include "../commons/collections/list.h"
#include "../commons/error.h"
#include "protocol.h"
#include "KYU-commons.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>


#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>



typedef enum {
	SERVER,
	CLIENT,
	REMOTE
} t_SCB_type;


typedef struct socket_control_block {
	process_id_t pid;
	int socket_fd;
	char * host;
	int port;
	bool connected;
	t_SCB_type type;
} t_SCB;


typedef struct connection_mannager {
	int max_fd;
	int max_data_size;
	fd_set master_set;
	int back_log;
	t_SCB * server;
	// Clientes conectados al server
	t_list * clients;
	// Conexiones a servidores remotos
	t_list * remotes;
} t_connection_mannager;




// Tamanio del buffer de lectura
static const int MAX_DATA_SIZE = 255;
// Cantidad de conexiones pendientes que ouede mantener en espera
static const int BACKLOG = 20;

// Maximo file descriptor para sockets
//int max_fd;
// ID del socket server
//int server_fd;
// Lista de sockets, servidor y clientes
//fd_set master_set;

//bool initialized = false;




/* METODOS PUBLICOS */

/*
 * Crea una nueva instancia del manejador de conecciones
 * */
t_connection_mannager *create_conn_mngr(int max_data_size);


/*
 * Destruye y libera memoria.
 * Cierra todas las conecciones
 */
void destroy_conn_mngr(t_connection_mannager *self);


/*
 * Metodos para busqueda de sockets
 */
t_SCB *find_by_socket_fd(t_connection_mannager *self,  int socket_fd);

t_SCB *find_by_pid(t_connection_mannager *self,  process_id_t pid);



/*
 * Conecta un cliente a un socket servidor
 * Devuelve :
 	 -1 : Si hubo algun error
 	 >0 : Si hubo conexion y la respuesta es el descriptor del socket-cliente
 */
t_SCB *connect_remote(t_connection_mannager *self, char *host, int port);


/*
 * Abre el puerto de conexion
 * Devuelve :
 	 -1 : Si hubo algun error
 	 >0 : Si hubo conexion y la respuesta es el descriptor del socket-servidor
 */
t_SCB *open_server(t_connection_mannager *self, int port, int back_log, process_id_t pserver_id);


/*
 * Pone en escucha al servidor creado anteriormente.
 * server_fd debe ser > 0
 * Devuelve :
 	 -1 : Si hubo error
 	  0 : Si pudo conectar
 */
t_SCB *start_server(t_connection_mannager *self);

//void stop_server(t_connection_mannager *self);


/*
 * Cierra la conexion de un socket segun su descriptor
 */
void disconnect(t_connection_mannager *self, t_SCB *connection);


/*
 * cierra todos los sockets abiertos, clientes y servidor
 */
int disconnect_all(t_connection_mannager *self);

/*
 * Monitorea los sockets abiertos
 * Resuelve las peticiones de conexion al socket servidor (si hubiere un server activo) y las agrega a la lista de descriptores como
 	 nuevas conexiones
 * Entrega una lista con los descriptores de sockets que hayan tenido actividad durante el time_out dado
 * Devuelve :
 	 	  0 : Si se cumple el tiempo de time_out y no hubo movimentos en ninguna conexion
 	 	 -1 : Si ocurrio algun error
 	 	 >0 : Cantidad de conexiones con movimiento
 * @time_out : tiempo de espera maximo en segundos. Devuelve el tiempo restante si algun socket necesita ser atendido o cero y se
 	 	 	 	 	 	 	 cumple todo el tiempo dado
 * @list : lista de descriptores de sockets que necesiten ser atendidos
 * @list_lenght : cantdad de descriptores en la lista
 */
t_list *handle_incomming(t_connection_mannager *self, int time_out);


/*
 * Envia toda la cadena sin importar su tamaño. Envia tantos paquetes de red como sean necesarios hasta completar la longitud
 	 total del mensaje
 * Devuelve :
 	 	  0 : Si el socket esta cerrado
 	 	 -1 : Si ocurrio algun error
 	 	 >0 : Cantidad de bytes efectivamente enviados
 * @socket_fd : descriptor de socket
 * @data : mensaje
 * @data_length : longitud del mensaje a enviar
 */
int send_all(t_SCB *connection, char *msg, int bytes);

/*
 * Recibe la cantidad de bytes solicitados del socket seleccionado y devuelve el mensaje en data
 * Devuelve :
 	 	  0 : Si el socket esta cerrado
 	 	 -1 : Si ocurrio algun error
 	 	 >0 : Cantidad de bytes efectivamente recibidos
 * @socket_fd : Descriptor del socket
 * @data_length : Cantidad de bytes a recibir
 * @data : Mensaje recibido
 */
int recieve_all(t_SCB *connection, char *msg, int bytes);


#endif /* SOCKET_CONNECTION_H_ */
