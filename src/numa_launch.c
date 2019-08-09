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

static int fastmem_node = 0;
static int slowmem_node = 0;
static int cpu_node = 0;


static char cgroup_procs[256];

static struct sigaction child_exit_sigact = {0};


static int child_quit = 0;

static pid_t child_pid = 0;

static struct option long_options [] = 
{

	{"fast-mem-node", required_argument, 0, 'F'},
	{"slow-mem-node", required_argument, 0, 'S'},

	{"cpu-node", required_argument, 0, 'C'},

	{"cgroup", required_argument, 0, 'c'},

	{0,0,0,0}
};


static void usage(const char *appname)
{
	printf("%s --cgroup=<cgroup> --cpu-node=<cpu-node> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node>\n", appname);
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

	if(argc != 5) {
		usage(argv[0]);
		exit(0);
	}

	while ((c = getopt_long(argc, argv, "F:S:C:c:", long_options, &option_index)) != -1) {
		switch (c) {
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
				memset(cgroup_procs, 0, sizeof(cgroup_procs));
				(void)snprintf(cgroup_procs, sizeof(cgroup_procs), "%s/cgroup.procs", optarg);
				break;
			default:
				abort();
		}
	}

	child_exit_sigact.sa_sigaction = child_exit;

	child_exit_sigact.sa_flags = SA_SIGINFO;

	if (sigaction(SIGCHLD, &child_exit_sigact, NULL) < 0) {
		perror("failed sigaction SIGCHLD\n");
		exit(-1);
	}

	child_pid = fork();

	if(child_pid == 0) {

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

		if (numa_run_on_node_mask_all(cpu_mask) < 0) {
			fprintf(stderr, "failed to bind on numa node #%d\n", cpu_node);
			exit(-1);
		}


		if (set_mempolicy(MPOL_PREFERRED|MPOL_F_MEMCG, fastmem_mask->maskp, fastmem_mask->size + 1) < 0) {
			fprintf(stderr, "failed set_mempolicy\n");
			exit(-1);
		}

		printf("Launch %s ...\n",argv[0]);

		child_status = execvp(argv[0], argv);

		printf("child exited with child_status = %d\n", child_status);

		exit(child_status);
	}

	printf("Child pid is %d\n", child_pid);

	while(child_quit == 0)
		sleep(1);

	return 0;
}
