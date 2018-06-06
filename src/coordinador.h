/*
 * coordinador.h
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#ifndef COORDINADOR_H_
#define COORDINADOR_H_

#include "commons/log.h"
#include "commons/process.h"
#include "commons/collections/queue.h"
#include "commons/collections/dictionary.h"
#include "commons/collections/list.h"
#include "commons/error.h"
#include "commons/config.h"
#include "commons/string.h"

#include "KYU-commons/socket-connection.h"


#include <stdlib.h>
#include <stdio.h>

/*
 * Algoritmos de ditribucion
 */
typedef enum  {
	LAST_SPACE_USED,
	EQUITATIVE_LOAD,
	KEY_EXPLICIT
} distribution_criteria_t;


/*
 * Datos del archivo de configuracion
 * @server_port : Puerto de escucha para conexiones entrantes
 * @distribution_criteria : Algoritmo de distribucion para almacenamiento de datos en instancias
 * @entry_count : Cantidad de entradas que maneja una instancia
 * @entry_size : Tamaño de las entradas
 * @execution_delay : Tiempo en segundos que dura la ejecucion de una instruccion sobre la instancia
 * @config_file : Archivo de configuracion. Da acceso a la lectura/modificacion por clave claves
 */
typedef struct coordinador_config {
	int server_port;
	distribution_criteria_t distribution_criteria;
	int entry_count;
	int entry_size;
	int execution_delay;
} t_coordinador_config;


/*
 * Planificador
 * @id : Id de proceso
 * @key_table : guarda la distribucion de llaves respecto de instancias
 * @instances : Lista de instancias conectadas
 */
typedef struct coordinador {
	process_id_t id;
	t_coordinador_config * config;
	t_dictionary * key_table;
	t_list * instances;
	t_connection_mannager * conn_mngr;
	t_SCB * planificador;
} t_coordinador;


// Log de eventos. Una vez que este todo funcionando debe quitarse porque no es parte del requerimiento para
// este modulo
//t_log log;



/*
 * Lee el archivo de configuracion y devuelve los datos en una estructura
 * @path : ruta del archivo de configuracion del coordinador
 * */
t_coordinador_config *get_coordinador_config(char *path);


/*
 * Libera la memoria alocada
 * */
void coordinador_destroy(t_coordinador *self);

void coordinador_config_destroy(t_coordinador_config *config);


/*
 * Crea al coordinador y carga su archivo de configuracion
 * */
t_coordinador *coordinador_create(char *config_path);


/*
 * Recibe las solicitudes de ejecucion de los ESI, administra las instancias y coopera con
 * el planificador para el control de utilizacion de claves
 *
 * Recibe una solicitud del ESI
 * 	|->	Instruccion GET - Envia aviso al planificacion del requerimiento de la clave
 * 	|-> Instruccion SET - Consulta al planificador si la clave esta bloqueda por otro ESI. Si no esta bloqueada
 * 												envia la instruccion a la instancia que corresponda
 * 	|-> Instruccion STORE - Envia la instruccion a la instancia y da aviso al planificador
 *
 * Recibe una solicitud de la instancia
 * 	|->
 *
 * Devuelve :
 	 -1 : Si hubo error
 	  0 : exito
 */
int coordinador_run(t_coordinador *self);


/*
 * Procesa la solicitud de ejecucion del ESI
 */
int handle_esi_request(t_coordinador *self, int socket_fd);


/*
 * Procesa la solicitud de la instancia
 * @self : El coordinador
 * @socket_fd : Descriptor del socket que debe ser atendido
 * */
int handle_instance_request(t_coordinador *self, int socket_fd);


/*
 * Procesa la solicitud del planificador
 * @self : El coordinador
 * @socket_fd : Descriptor del socket que debe ser atendido
 * */
int handle_planificador_request(t_coordinador *self, int socket_fd);


/*
 * Procesa la sentencia sobre la instancia adecuada. La sentencia se recibe del ESI
 * 1. Para operaciones GET, No realiza ninguna operacion sobre la instancia, solo da aviso al
 * 		planificador
 * 2. Para operaciones SET, selecciona la instancia segun el algorimo de distribucion configurado, opera y
 * 		da aviso al planificador
 * 3. Para operaciones STORE, selecciona la instancia de key_table, opera y da aviso al planificador para liberar
 * 		la clave
 */
int process_sentence(t_coordinador *self, sys_sentence_t *sentence);

/*
 * Inicializa el coordinador, lanza la ejecucion de tareas y se pone en espera de conexion
 * de los ESIs, Instancias y Planificador
 * Recive 1 parametro. El path del archivo de configracion
 */
int main(int argc, char *argv[]);



#endif /* COORDINADOR_H_ */
