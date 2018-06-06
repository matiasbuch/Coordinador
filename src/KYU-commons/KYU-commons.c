#include "KYU-commons.h"

/*
 * Utiles linux
 *
 * Para buscar texto en archivos:
 	 grep -rnw '/path/to/somewhere/' -e 'pattern'

 	 ruta archvos .h
 	 /usr/include
 */



char *read_line(FILE *file, int *readed) {

	int _buffer_capacity = BUFFER_CAPACITY;

	if (file == NULL) {
			printf("Error de lectura de archivo");
			*readed = 0;
			return NULL;
	}

	char *buffer = malloc(sizeof(char) * _buffer_capacity);

	if (buffer == NULL) {
			printf("Error iniciando buffer");
			*readed = 0;
			return NULL;
	}

	char ch = getc(file);
	int count = 0;

	while ((ch != '\n') && (ch != EOF)) {
			if (count == _buffer_capacity) {
				_buffer_capacity = _buffer_capacity * 2;
				buffer = realloc(buffer, _buffer_capacity);

				if (buffer == NULL) {
						printf("No hay memoria suficiente para buffer de lectura");
						*readed = 0;
						return NULL;
				}
			}

			buffer[count++] = ch;
			ch = getc(file);
	}

	buffer[count] = '\0';

	char *line = malloc(count + 1);
	strncpy(line, buffer, (count + 1));
	free(buffer);

	*readed = count;
	return line;
}

