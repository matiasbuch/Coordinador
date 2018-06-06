#ifndef PROTOCOL_H_
#define PROTOCOL_H_


#include <sys/time.h>

/* tipo de dato para representar el ID de mensaje */
typedef unsigned int mid_t;
typedef char * entry_key_t;
typedef unsigned int process_id_t;

/*
 * Tipo de instruccion
 */
typedef enum {
	GET,
	SET,
	STORAGE
} sys_intruction_t;


typedef enum {
	EXECUTED,
	NOT_EXECUTED,
	ERROR
} sys_sentence_result_t;

/*
 * Tipos de mensaje
 */
typedef enum {
	ACK = 0,
	CONNECTION_REQUEST = 1,
	CONNECTION_RESPONSE = 2,
	EXECUTE_INSTRUCTION_REQUEST = 3,
	EXECUTE_INSTRUCTION_RESPONSE = 4
} message_type_t;

/*
 * Estado recibido luego de la solicitu de conexion
 */
typedef enum {
	ACCEPTED = 0,
	REFUSED = 1
} connection_result_t;


typedef struct sys_sentence {
	sys_intruction_t instruction;
	entry_key_t key;
	char *value;
} sys_sentence_t;


/*
 * Estructura que gusrada informacion del mensaje enviado
 */
typedef struct message_sent {
	mid_t message_id;
	struct timeval date;
	char* message;
	int socket_fd;
} message_sent_t;


/*
 * ESTRUCTURAS PARA ENVIO DE MENSAJES
 * */

/*
 * Header de mensaje
 */
typedef struct msg_header {
	process_id_t process_id;
	mid_t message_id;
	message_type_t message_type;
	int message_length;
} message_header_t;


/*
 * Solicitud de conexion
 */
typedef struct msg_connect_request {
	char *message;
} msg_connect_request_t;


/*
 * Respuesta de solicitud de conexion
 * @connected : 0:No conectado, 1:Conectado
 * @message_length : tamaño del mensaje
 * @message : mensaje vario. Puede ser la razon de conexion no permitida o lo que sea.
  				Puede estar vacio y en ese caso message_length debe ser 0
 */
typedef struct msg_connect_response {
	connection_result_t connected;
	int message_lenght;
	char *message;
} msg_connect_response_t;


/*
 * Solicitud de ejecucion de sentencia
 */
typedef struct msg_execute_sentence_request {
	sys_sentence_t sentence;
} t_msg_execute_instruction_request;


/*
 * Repuesta de ejecucion de sentencia
 */
typedef struct msg_execute_sentence_response {
	sys_sentence_result_t result;
} msg_execute_instruction_response_t;


/*
 * SERIALIZACION Y DESEREALIZACION DE MENSAJES
 */

/*
 * parsea el string y devuelve la estructura del header
 */
message_header_t deserialize_msg_header(char *buffer);

/*
 * devuelve un string formado por los datos de la estructura
 */
char *serialize_msg_header(message_header_t header);


#endif /* PROTOCOL_H_ */


