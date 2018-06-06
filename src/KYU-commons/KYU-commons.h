#ifndef KYU_COMMONS_
#define KYU_COMMONS_

#include "../commons/log.h"
#include "../commons/string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int BUFFER_CAPACITY = 100;

void show(t_log_level level, const char* message, ...);

char *readLine(FILE *file);


#endif /* KYU_COMMONS_ */
