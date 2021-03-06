/*
 *
 * Author: Jiaolin Luo
 *
 * */

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#include <errno.h>
#include <limits.h>

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MPOL_F_MEMCG    (1<<13)

struct bitmask *fastmem_mask = NULL;
struct bitmask *slowmem_mask = NULL;
struct bitmask *cpu_mask = NULL;

static char fastmem_node[128];
static char slowmem_node[128];
static char cpu_node[128];

//static unsigned long fastmem_size = 0;

//static char cgroup[128];
//static char cgroup_procs[256];
//static char cgroup_max_size[256];

static struct sigaction child_exit_sigact = {0};
static struct sigaction term_sigact = {0};


static int child_quit = 0;

static pid_t child_pid = 0;

static struct option long_options [] = 
{

	//{"fast-mem-size", required_argument, 0, 's'},

	{"fast-mem-node", required_argument, 0, 'F'},
	{"slow-mem-node", required_argument, 0, 'S'},
	{"cpu-node", required_argument, 0, 'C'},

	//{"cgroup", required_argument, 0, 'c'},

	//{"help", required_argument, 0, 'H'},
	{0,0,0,0}
};


static void usage(const char *appname)
{
	//printf("%s --cgroup=<cgroup> --cpu-node=<cpu-node> [--fast-mem-size=<fast-mem-size-in-mb>] --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node>\n", appname);
	printf("%s --cpu-node=<cpu-node> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node> -- <cmd> <arg1> ...\n", appname);
	printf("Ex #1: %s --cpu-node=1 --fast-mem-node=0 --slow-mem-node=1 -- ls\n", appname);
	printf("Ex #2: %s --cpu-node=0-1 --fast-mem-node=0-1 --slow-mem-node=2-3 -- ls\n", appname);
	printf("Ex #3: %s --cpu-node=any --fast-mem-node=0-1 --slow-mem-node=2-3 -- ls\n", appname);
}

void term_signal(int sig, siginfo_t *siginfo, void *context)
{
	int status;

	sleep(1);
	kill(child_pid, SIGTERM);
	sleep(1);
	kill(child_pid, SIGKILL);

	waitpid(child_pid, &status, WNOHANG);

	child_quit = 1;
}


void child_exit(int sig, siginfo_t *siginfo, void *context)
{
	int status;

	printf("Child %d exited\n", siginfo->si_pid);

	if (waitpid(siginfo->si_pid, &status, WNOHANG) != child_pid)
		return;

	child_quit = 1;
}

int main(int argc, char **argv)
{
	char c;
	int option_index = 0;

	int fd;

	char str[32];

	int ret;

	int child_status;


	setbuf(stdout, NULL);

	char *cmdline;
	int i;
	int len = 0;
	int found = 0;

   // printf("argc=%d\n", argc);

	if(argc < 6) {
		usage(argv[0]);
		exit(0);
	}



	for(i = 0; i < argc; i++) {
		if(strcmp(argv[i], "--") == 0 && strlen(argv[i]) == strlen("--")) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
        printf("can't find the separator '--'\n");
		usage(argv[0]);
		exit(0);
	}

	//memset(cgroup, 0, sizeof(cgroup));

    memset(cpu_node, 0, sizeof(cpu_node));
    memset(fastmem_node, 0, sizeof(fastmem_node));
    memset(slowmem_node, 0, sizeof(slowmem_node));
    
	while ((c = getopt_long(argc, argv, "F:S:C:", long_options, &option_index)) != -1) {

        option_index=0;

		switch (c) {
			case 'F':
                strcpy(fastmem_node, optarg);
				fastmem_mask = numa_parse_nodestring(fastmem_node);
            //    printf("fastmem_node=%s\n", optarg);
				break;
			case 'S':
                strcpy(slowmem_node, optarg);
				slowmem_mask = numa_parse_nodestring(slowmem_node);
              //  printf("slowmem_node=%s\n", optarg);
				break;
			case 'C':
                //printf("cpu_node=%s\n", optarg);
                strcpy(cpu_node, optarg);
                if(strcmp(optarg, "any") != 0) {
				    cpu_mask = numa_parse_nodestring(cpu_node);
				    //cpu_node = atoi(optarg);
                }
				break;
			default:
		        usage(argv[0]);
		        exit(0);
				//abort();
                //break;
		}
	}


	if(fastmem_node[0] == 0) {
        printf("Option '--fast-mem-node' must be set\n");
		usage(argv[0]);
		exit(0);
	}

	if(slowmem_node[0] == 0) {
        printf("Option '--slow-mem-node' must be set\n");
		usage(argv[0]);
		exit(0);
	}

	//skip
	argv += optind;
	argc -= optind;


	child_exit_sigact.sa_sigaction = child_exit;
	child_exit_sigact.sa_flags = SA_SIGINFO;

	if (sigaction(SIGCHLD, &child_exit_sigact, NULL) < 0) {
		perror("failed sigaction SIGCHLD\n");
		exit(-1);
	}

	term_sigact.sa_sigaction = term_signal;
	term_sigact.sa_flags = SA_SIGINFO;

	if (sigaction(SIGTERM, &term_sigact, NULL) < 0) {
		perror("failed sigaction SIGTERM\n");
		exit(-1);
	}

	child_pid = fork();

	if(child_pid == 0) {
#if 0
		//set cgroup.procs
		
		(void)snprintf(cgroup_procs, sizeof(cgroup_procs), "%s/cgroup.procs", cgroup);

		fd = open(cgroup_procs, O_RDWR);

		if(fd < 0) {
			fprintf(stdout, "failed to open %s\n", cgroup_procs);
			exit(-1);
		}

		memset(str, 0, sizeof(str));

		(void) snprintf(str, sizeof(str), "%d\n", getpid());

		if ((ret = write(fd, str, sizeof(str))) <= 0) {
			
			fprintf(stderr, "failed to write pid %d to %s\n", getpid(), cgroup_procs);

			exit(-1);
		}

		close(fd);


		//set memory.max_at_node:1
		
		(void)snprintf(cgroup_max_size, sizeof(cgroup_max_size), "%s/memory.max_at_node:%d", cgroup, fastmem_node);

		fd = open(cgroup_max_size, O_RDWR);

		if(fd < 0) {
			fprintf(stdout, "failed to open %s\n", cgroup_max_size);
			exit(-1);
		}

		memset(str, 0, sizeof(str));

		if(fastmem_size == 0) {
			sprintf(str, "max\n");
		} else {
			(void) snprintf(str, sizeof(str), "%lu\n", fastmem_size  << 20);
		}

		if ((ret = write(fd, str, sizeof(str))) <= 0) {
			
			fprintf(stderr, "failed to write pid %d to %s\n", getpid(), cgroup_max_size);

			exit(-1);
		}

		close(fd);
#endif

        if(cpu_mask) {
            if (numa_run_on_node_mask_all(cpu_mask) < 0) {
                fprintf(stderr, "failed to bind on numa node #%s\n", cpu_node);
                exit(-1);
            }
        }

		if (set_mempolicy(MPOL_PREFERRED|MPOL_F_MEMCG, slowmem_mask->maskp, slowmem_mask->size + 1) < 0) {
		//if (set_mempolicy(MPOL_PREFERRED|MPOL_F_MEMCG, fastmem_mask->maskp, fastmem_mask->size + 1) < 0) {
			fprintf(stderr, "failed set_mempolicy\n");
			exit(-1);
		}

#if 0
		len = 0;

		for(i = 0; i < argc; i++) {
			len += strlen(argv[i]);
			len++;
		}

		len++;

		cmdline = malloc(len);

		if(cmdline == NULL) {
			perror("failed malloc()\n");
			exit(-1);
		}

		memset(cmdline, 0, len);

		len = 0;
		for(i = 0; i < argc; i++) {
			len += snprintf(cmdline + len, "%s ", argv[i]);
			len++;
		}
#endif

		//printf("Launch application %s on cpu node #%d fastmem_size %lu MB fastmem_node #%d slowmem_node #%d ...\n", cmdline, cpu_node, fastmem_size, fastmem_node, slowmem_node);
		printf("Launch application %s on cpu node #%s fastmem_node #%s  slowmem_node #%s ...\n", argv[0], cpu_node, fastmem_node, slowmem_node);

		//free(cmdline);

		child_status = execvp(argv[0], argv);

		printf("child exited with child_status = %d\n", child_status);

		exit(child_status);
	}

	printf("Child %s pid is %d\n", argv[0], child_pid);

	while(child_quit == 0)
		sleep(1);

	printf("Child %s pid %d exited\n", argv[0], child_pid);

	exit(0);
}
