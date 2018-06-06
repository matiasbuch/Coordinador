#include "KYU-commons.h"

/*
 * Utiles linux
 *
 * Para buscar texto en archivos:
 	 grep -rnw '/path/to/somewhere/' -e 'pattern'

 	 ruta archvos .h
 	 /usr/include
 */

static t_log * log;


void show(t_log_level level, const char* message, ...) {

	char * strLevel = log_level_as_string(level);

	va_list arguments;
	va_start(arguments, message);
	char * msg = string_from_format("[%s]=>%s", strLevel, message);
	char * print = string_from_format(msg, arguments);
	printf(print);
	va_end(arguments);


	if (log != NULL) {
		switch(level) {
		case LOG_LEVEL_TRACE:
			log_trace(message, arguments);
			break;
		case LOG_LEVEL_DEBUG:
			log_debug(message, arguments);
			break;
		case LOG_LEVEL_INFO:
			log_info(message, arguments);
			break;
		case LOG_LEVEL_WARNING:
			log_warning(message, arguments);
			break;
		case LOG_LEVEL_ERROR:
			log_error(message, arguments);
			break;
		}
	}

	free(arguments);
	free(print);
	free(msg);
	free(strLevel);

}



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

