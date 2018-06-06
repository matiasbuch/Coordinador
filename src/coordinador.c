#include "coordinador.h"

t_coordinador_config *get_coordinador_config(char *path) {

	char* PUERTO_COORDINADOR = "PUERTO_COORDINADOR";
	char* ALGORITMO_DISTRIBUCION = "ALGORITMO_DISTRIBUCION";
	char* CANTIDAD_ENTRADAS = "CANTIDAD_ENTRADAS";
	char* TAMANIO_ENTRADAS = "TAMANIO_ENTRADAS";
	char* RETARDO = "RETARDO";

	bool key_not_found = false;
	t_coordinador_config *coordinador_config = NULL;


	// Abro el archivo de configuracion
	t_config *config_file = config_create(path);

	if(config_file == NULL) {
		error_show("Error al abrir el archivo de configuracion");
		goto do_exit;
	}

	if (dictionary_is_empty(config_file->properties))
	{
		error_show("El archivo de configuracion no contiene las llaves requeridas");
		goto do_exit;
	}

	// PUERTO_COORDINADOR
	if (!dictionary_has_key(config_file->properties, PUERTO_COORDINADOR))
	{
		error_show("No se encontro la llave:[%s] en el archivo de configuracion", PUERTO_COORDINADOR);
		key_not_found = true;
	}

	// CANTIDAD_ENTRADAS
	if (!dictionary_has_key(config_file->properties, CANTIDAD_ENTRADAS))
	{
		error_show("No se encontro la llave:[%s] en el archivo de configuracion", CANTIDAD_ENTRADAS);
		key_not_found = true;
	}

	// TAMANIO_ENTRADAS
	if (!dictionary_has_key(config_file->properties, TAMANIO_ENTRADAS))
	{
		error_show("No se encontro la llave:[%s] en el archivo de configuracion", TAMANIO_ENTRADAS);
		key_not_found = true;
	}

	// RETARDO
	if (!dictionary_has_key(config_file->properties, RETARDO))
	{
		error_show("No se encontro la llave:[%s] en el archivo de configuracion", RETARDO);
		key_not_found = true;
	}

	// ALGORITMO_DISTRIBUCION
	if (!dictionary_has_key(config_file->properties, ALGORITMO_DISTRIBUCION))
	{
		error_show("No se encontro la llave:[%s] en el archivo de configuracion", ALGORITMO_DISTRIBUCION);
		key_not_found = true;
	}

	if (key_not_found)
		goto do_exit;


	coordinador_config = malloc(sizeof(t_coordinador_config));

	if (coordinador_config == NULL) {
		error_show("Memoria insuficiente para alocar la configuracion del coordinador");
		goto do_exit;
	}

	coordinador_config->server_port = config_get_int_value(config_file, PUERTO_COORDINADOR);
	coordinador_config->entry_count = config_get_int_value(config_file, CANTIDAD_ENTRADAS);
	coordinador_config->entry_size = config_get_int_value(config_file, TAMANIO_ENTRADAS);
	coordinador_config->execution_delay = config_get_int_value(config_file, RETARDO);
	char* algoritmo = config_get_string_value(config_file, ALGORITMO_DISTRIBUCION );

	if (string_equals_ignore_case(algoritmo, "LSU")) {
		coordinador_config->distribution_criteria = LAST_SPACE_USED;
	}
	else if (string_equals_ignore_case(algoritmo, "EL")) {
		coordinador_config->distribution_criteria = EQUITATIVE_LOAD;
	}
	else if (string_equals_ignore_case(algoritmo, "KE")) {
		coordinador_config->distribution_criteria = KEY_EXPLICIT;
	}

do_exit:

	config_destroy(config_file);
	return coordinador_config;
}


void coordinador_config_destroy(t_coordinador_config *config) {
	if (config == NULL)
		return;

	free(config);
}


void coordinador_destroy(t_coordinador *self) {
	if (self == NULL)
		return;

	// Cierro todas las conexiones
	// TODO
	disconnect_all(self->conn_mngr);
	coordinador_config_destroy(self->config);
	free(self);
}


t_coordinador *coordinador_create(char *config_path) {

	t_coordinador *self = malloc(sizeof(t_coordinador));

	self->id = process_getpid();
	self->config = get_coordinador_config(config_path);

	if (self->config == NULL) {
		error_show("No se pudo cargar el archivo de configuracion %s", config_path);
		coordinador_destroy(self);
		return NULL;
	}

	self->conn_mngr = create_conn_mngr(MAX_DATA_SIZE);

	if (self->conn_mngr == NULL) {
		error_show("No se pudo iniciar el controlador de conexiones");
		goto exception;
	}

	// Creo el servidor (socket) y verifico que haya asignado el file descriptor
	open_server(self->conn_mngr, self->config->server_port, BACKLOG, self->id);

	if (self->conn_mngr->server->socket_fd < 0) {
		error_show("No se pudo iniciar el servidor de conexiones. Puerto:[%d]", self->config->server_port);
		goto exception;
	}

	return self;


exception:
	coordinador_destroy(self);
	return NULL;
}



int coordinador_run(t_coordinador *self) {

	return 0;
}


int handle_esi_request(t_coordinador *self, int socket_fd) {
	return 0;
}


int handle_instance_request(t_coordinador *self, int socket_fd) {
	return 0;
}


int handle_planificador_request(t_coordinador *self, int socket_fd) {
	return 0;
}


int process_sentence(t_coordinador *self, sys_sentence_t *sentence) {
	return 0;
}



int main(int argc, char *argv[]) {

	//char* path = "/home/utnso/git/Coordinador/resources/config.cfg";
	show(LOG_LEVEL_INFO, "Inicio Coordinador\n");

	if (argc < 2) {
		error_show("Se esperaba como parametro el path del archivo de configuracion\n");
		exit(1);
	}

	t_coordinador * self = coordinador_create(argv[1]);

	if (self == NULL) {
		goto exception;
	}

	// Pongo a escichar al servidor
	start_server(self->conn_mngr);

	if (!self->conn_mngr->server->connected) {
		error_show("Erro al abrir el puerto servidor de conexiones. Puerto:[%d]", self->conn_mngr->server->port);
		coordinador_destroy(self);
		goto exception;
	}

	//coordinador_run(self);

exception:
	coordinador_destroy(self);
	exit(1);
}


