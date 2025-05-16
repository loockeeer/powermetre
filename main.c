#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>

int spawn_child(char *program,char *argv[]){
	pid_t child_pid = fork();

	if(child_pid > 0){
		return child_pid;
	}else{
		setpriority(PRIO_PROCESS, 0,-20);
		execvp(argv[0], argv);
		exit(0); // edge-case
	}
}

char **make_null_terminated_argv(int argc, char **argv)
{
	char **ret = malloc(sizeof(char*)*(argc+1));
	memcpy(ret, argv, argc*sizeof(char*));
	ret[argc] = NULL;
	return ret;
}

long double spent_children_time() {
	pid_t pid = getpid();
	char *buffer = malloc(100);
	sprintf(buffer, "/proc/%d/stat", pid);
	FILE *f = fopen(buffer, "r");
	int field = 0;
	int rel = 0;
	for(int c = fgetc(f);c!=EOF;c=fgetc(f))
	{
		if(c == ' ') {
			field++;
			continue;
		}
		if(field == 15) { // cutime
			buffer[rel] = (char)c;
			rel++;
		}
		if(field > 15) {
			buffer[rel] = '\0';
			break;
		}
	}
	fclose(f);
	long long int d = atoll(buffer);
	free(buffer);
	return (long double)d / (long double)sysconf(_SC_CLK_TCK);
}

int main(int argc, char *argv[]) {
	if(argc < 2)
	{
		printf("pas assez d'args\n");
		return 1;
	}
	int retval;
	int event_set = PAPI_NULL;
	long long start_time, end_time;
	long long values[2]; // Array to store energy values

	// Initialize the PAPI library
	if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		fprintf(stderr, "PAPI_library_init failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	// Create a new event set
	if ((retval = PAPI_create_eventset(&event_set)) != PAPI_OK) {
		fprintf(stderr, "PAPI_create_eventset failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	// Add RAPL energy events to the event set
	if ((retval = PAPI_add_named_event(event_set, "rapl:::PP0_ENERGY:PACKAGE0")) != PAPI_OK) {
		fprintf(stderr, "PAPI_add_event (PAPI_ENERGY_PKG) failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	if ((retval = PAPI_add_named_event(event_set, "rapl:::DRAM_ENERGY:PACKAGE0")) != PAPI_OK) {
		fprintf(stderr, "PAPI_add_event (PAPI_ENERGY_DRAM) failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	// Start counting events
	if ((retval = PAPI_start(event_set)) != PAPI_OK) {
		fprintf(stderr, "PAPI_start failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	// Perform some workload or sleep
	printf("Measuring energy consumption...\n");
	start_time = PAPI_get_real_nsec();
	spawn_child(argv[1], make_null_terminated_argv(argc-1, argv+1));
	int status;
	wait(&status);
	end_time = PAPI_get_real_nsec();
	// Stop counting and read values
	if ((retval = PAPI_stop(event_set, values)) != PAPI_OK) {
		fprintf(stderr, "PAPI_stop failed: %s\n", PAPI_strerror(retval));
		return 1;
	}
	long long diff = end_time - start_time;
	long double real_child_time = spent_children_time();
	long double prop = (real_child_time)/((long double)diff);
	// Print the measured energy values
	printf("Energy consumption:\n");
	printf("  Package power: %Lf J\n", ((long double)values[0])*prop);
	printf("  DRAM power: %Lf J\n",((long double)values[1])*prop);
	printf("  Time (real): %Lf (%Lf) s\n", diff/((long double)1e9), real_child_time);
	printf("  Overshoot : %Lf s\n",diff/((long double)1e9) -real_child_time );
	// Cleanup
	if ((retval = PAPI_cleanup_eventset(event_set)) != PAPI_OK) {
		fprintf(stderr, "PAPI_cleanup_eventset failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	if ((retval = PAPI_destroy_eventset(&event_set)) != PAPI_OK) {
		fprintf(stderr, "PAPI_destroy_eventset failed: %s\n", PAPI_strerror(retval));
		return 1;
	}

	PAPI_shutdown();
	return 0;
}