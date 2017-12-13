#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "queue.h"
#include "util.h"

// From writeup
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6 ADDRSTRLEN


// From lookup.c
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

pthread_mutex_t mutex_queue; // queue lock
pthread_mutex_t mutex_result; // result lock
pthread_cond_t cond_signal; // condition signal for waiting threads
queue theQ; // the queue
int cntr; // the counter

void *requester(void *filename);
void *resolver(void *filename);

#endif