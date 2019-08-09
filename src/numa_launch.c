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

static int fastmem_node = -1;
static int slowmem_node = -1;
static int cpu_node = -1;

static unsigned long fastmem_size = 0;

static char cgroup[128];
static char cgroup_procs[256];
static char cgroup_max_size[256];

static struct sigaction child_exit_sigact = {0};


static int child_quit = 0;

static pid_t child_pid = 0;

static struct option long_options [] = 
{

	{"fast-mem-size", required_argument, 0, 's'},

	{"fast-mem-node", required_argument, 0, 'F'},
	{"slow-mem-node", required_argument, 0, 'S'},

	{"cpu-node", required_argument, 0, 'C'},

	{"cgroup", required_argument, 0, 'c'},

	{0,0,0,0}
};


static void usage(const char *appname)
{
	printf("%s --cgroup=<cgroup> --cpu-node=<cpu-node> [--fast-mem-size=<fast-mem-size-in-mb>] --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node>\n", appname);
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


	if(argc < 6) {
		usage(argv[0]);
		exit(0);
	}

	memset(cgroup, 0, sizeof(cgroup));

	while ((c = getopt_long(argc, argv, "s:F:S:C:c:", long_options, &option_index)) != -1) {

		printf("c=%d %c option_index=%d optind=%d\n",c,c,option_index, optind);

		switch (c) {
			case 's':
				fastmem_size = atol(optarg);
				break;
			case 'F':
				fastmem_mask = numa_parse_nodestring(optarg);
				fastmem_node = atoi(optarg);
				break;
			case 'S':
				slowmem_mask = numa_parse_nodestring(optarg);
				slowmem_node = atoi(optarg);
				break;
			case 'C':
				cpu_mask = numa_parse_nodestring(optarg);
				cpu_node = atoi(optarg);
				break;
			case 'c':
				(void)snprintf(cgroup, sizeof(cgroup), "%s", optarg);
				break;
			default:
				abort();
		}
	}


	if(fastmem_node == -1 || slowmem_node == -1 || cpu_node == -1 || strlen(cgroup) == 0) {
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

	child_pid = fork();

	if(child_pid == 0) {

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


		if (numa_run_on_node_mask_all(cpu_mask) < 0) {
			fprintf(stderr, "failed to bind on numa node #%d\n", cpu_node);
			exit(-1);
		}


		if (set_mempolicy(MPOL_PREFERRED|MPOL_F_MEMCG, fastmem_mask->maskp, fastmem_mask->size + 1) < 0) {
			fprintf(stderr, "failed set_mempolicy\n");
			exit(-1);
		}

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

		printf("Launch application %s on cpu node #%d fastmem_size %lu MB fastmem_node #%d slowmem_node #%d ...\n", cmdline, cpu_node, fastmem_size, fastmem_node, slowmem_node);

		free(cmdline);

		child_status = execvp(argv[0], argv);

		printf("child exited with child_status = %d\n", child_status);

		exit(child_status);
	}

	printf("Child pid is %d\n", child_pid);

	while(child_quit == 0)
		sleep(1);

	return 0;
}
