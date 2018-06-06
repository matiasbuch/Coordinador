#include "protocol.h"
#include <time.h>
#include <stdlib.h>

/*
 * Genera un id unico de mensaje
 */
int init_mid_seed = 0;

mid_t get_message_id() {

	if (init_mid_seed == 0) {
		time_t t;
		srand((unsigned) time(&t));
		init_mid_seed++;
	}

	 // returns a pseudo-random integer between 0 and RAND_MAX
	return rand();
}


message_header_t *deserialize_message_header(char *buffer) {
	message_header_t *header = malloc(sizeof(message_header_t));
	// TODO
	return header;
}

char *serialize_message_header(message_header_t *header) {
	//char* buffer;
	//TODO
	return NULL;
}


