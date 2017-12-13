// Evan Kerns
// CSCI:3753
// PA3
// Collaborated with Sami Meharzi

#include "multi-lookup.h"

int main(int argc, char* argv[]){

    // Local variables (from lookup.c) (do they even do anything?)
    char errorstr[SBUFSIZE];
    int i;

    // Check MINARGS ( = 3) (from lookup.c)
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	queue_cleanup(&theQ);
	return EXIT_FAILURE;
    }

	// Check MAX_INPUT_FILES ( = 10)
    if(argc > MAX_INPUT_FILES){
	fprintf(stderr, "Too many arguments: %d is the limit: %d\n", MAX_INPUT_FILES, (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	queue_cleanup(&theQ);
	return EXIT_FAILURE;
    }
	
	// Initialize locks. NULL means default attr
	pthread_mutex_init(&mutex_queue, NULL);
	pthread_mutex_init(&mutex_result, NULL);

	// Get remaing argument space. argc-1 is the output, so argc-2 is the last
	int argspace = argc - 2;
	// cntr is will be decreased later to show number of inputs left
	// When it's 0, resolver can start
	cntr = argc - 2;
	// From pthread-hello.c, unused as of now
	int rc;
	long cpyt[argc];
	// Make requester threads (derived from pthread-hello.c)
	pthread_t req_threads[argspace];
	for (long t = 0; t < argspace; t++)
	{
		printf("In main: creating requester thread %ld\n", t);
		printf("argv[t+1] = %d\n", argv[t+1]);
		cpyt[t] = t;
		// t + 1 is because argv[0] is program name
		rc = pthread_create(&(req_threads[t]), NULL, requester, (void *)&argv[t+1]);
		if (rc){
	    		printf("ERROR; return code from pthread_create() is %d\n", rc);
	   		exit(EXIT_FAILURE);
		}
	}

	// Make resolver threads
	pthread_t res_threads[MAX_RESOLVER_THREADS];
	for (int t = 0; t < MAX_RESOLVER_THREADS; t++)
	{
		printf("In main: creating resolver thread %d\n", t);
		printf("argv[argc-1] = %d\n", argv[argc-1]);
		// argc -1 is for the output file
		pthread_create(&(res_threads[t]), NULL, resolver, (void *)&argv[argc-1]);
	}

	// Wait for all threads to finish (from pthread-hello.c)
    for(int t=0;t<argspace;t++){
	pthread_join(req_threads[t],NULL);
    }
    printf("All of the threads were completed!\n");

    for(int t=0;t<MAX_RESOLVER_THREADS;t++){
	pthread_join(res_threads[t],NULL);
    }
    printf("All of the threads were completed!\n");

	// Cleanup and Exit
	queue_cleanup(&theQ);
	return EXIT_SUCCESS;
}

void *requester(void *arg)
{
	printf("^^ Made it to requester ^^\n");
	// Variables, some from lookup.c
   FILE* inputfp = NULL;
	char* filename = (char* )arg;
	inputfp = fopen(filename, "r");
	if (!inputfp)
	{
		printf("*** Error with input file ***\n");
	}
	char* hostname = (char*) malloc(SBUFSIZE);
	printf("^^ Before fscanf loop ^^\n");
	// while loop from lookup.c
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		printf("^^ We are inside fscanf loop ^^\n");
		// If the queue is full, sleep for 0 to 100 microseconds
		if (queue_is_full(&theQ))
		{
			usleep((rand()%100)*1+100);
		}

		// Lock the mutex_queue
		pthread_mutex_lock(&mutex_queue);

		// Check if queue is full when pushing the hostname into theQ
		if(queue_push(&theQ, (void*) hostname) == QUEUE_FAILURE)
		{
			printf("THE QUEUE IS FULL");
		}
		
		// Signal waiting threads
		pthread_cond_signal(&cond_signal);
		// Unlock mutex_queue
		pthread_mutex_unlock(&mutex_queue);
		// Allocate memory for next loop
		hostname = (char* ) malloc(SBUFSIZE);
	}
	printf("^^ Post fscanf loop ^^\n");
	// Decrease cntr until 0, to start resolver
	cntr--;
	
	fclose(inputfp);
	return NULL;
}

void *resolver(void *arg)
{
	printf("-- Made it to resolver --\n");
	// Variables from lookup.c
    FILE* outputfp = NULL;
    char firstipstr[INET6_ADDRSTRLEN];
	char* hostname = (char*) malloc(SBUFSIZE);
	char* popit;

	printf("-- Before cntr > 0 loop --\n");
	while (cntr > 0 || !queue_is_empty(&theQ))
	{
		printf("-- We are in cntr > 0 loop --\n");
		pthread_mutex_lock(&mutex_queue);
		printf("-- After locking --\n");
		// In case queue_is_empty and cntr > 0, waits for !queue_is_empty
		while (queue_is_empty(&theQ))
		{
			printf("-- Queue is empty --\n");
			pthread_cond_wait(&cond_signal, &mutex_queue);
		}

		printf("-- At the pop spot --\n");
		// new hostname equals popped hostname from theQ
		popit = queue_pop(&theQ);
		strcpy(hostname, popit);

		// Unlock the lock
		pthread_mutex_unlock(&mutex_queue);

		printf("-- At the DNS lookup spot --\n");
	    // Lookup hostname and get IP string (from lookup.c)
	    if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
	       == UTIL_FAILURE){
		fprintf(stderr, "dnslookup error: %s\n", hostname);
		strncpy(firstipstr, "", sizeof(firstipstr));
	    }
		
		// Lock the result lock
		pthread_mutex_lock(&mutex_result);

		printf("-- Before opening output --\n");
   		 // Open Output File (from lookup.c)
   	 	outputfp = fopen(arg, "w");
   	 	if(!outputfp){
			perror("Error Opening Output File");
    		}
		printf("-- After cntr > 0 loop --\n");

	    // Write to Output File (from lookup.c)
	    fprintf(outputfp, "%s,%s\n", hostname, firstipstr);


   		 // Close Output File (from lookup.c)
   	 	fclose(outputfp);
		// Set to NULL because it loops again, possibly
		outputfp = NULL;

		// Unlock the result lock
		pthread_mutex_unlock(&mutex_result);
	}
	return NULL;
}
