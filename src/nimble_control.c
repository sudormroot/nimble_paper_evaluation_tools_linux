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


#define MPOL_MF_MOVE		(1<<1) /* Trigger page migration operations. */
#define MPOL_MF_MOVE_ALL	(1<<2)
#define MPOL_MF_MOVE_DMA	(1<<5)
#define MPOL_MF_MOVE_MT		(1<<6)
#define MPOL_MF_MOVE_CONCUR	(1<<7)
#define MPOL_MF_EXCHANGE	(1<<8)
#define MPOL_MF_SHRINK_LISTS	(1<<9)

#define MPOL_F_MEMCG	(1<<13)

static int syscall_mm_manage = 334;
static int mm_manage_flags = 0;

static int fastmem_node = -1;
static int slowmem_node = -1;

struct bitmask *fastmem_mask = NULL;
struct bitmask *slowmem_mask = NULL;

static unsigned long managed_pages = ULONG_MAX;

static int migration=1;
static int thp_migration=0;

static int concur_migration=0;
static int opt_migration=0;
static int basic_exchange_pages=0;
static int concur_only_exchange_pages=0;
static int exchange_pages=0;
static int hot_and_cold_pages=0;
static int shrink_page_lists=0;
static int move_hot_and_cold_pages=0;


static int pid = 0;

static struct option long_options [] = 
{
	{"pid", required_argument, 0, 'p'},

	{"fast-mem-node", required_argument, 0, 'F'},
	{"slow-mem-node", required_argument, 0, 'S'},

	{"managed-pages", required_argument, 0, 'P'},

	{"nomigration", no_argument, 0, 'N'},

	{"non-thp-migration", no_argument, 0, 't'},

	{"thp-migration", no_argument, 0, 'T'},

	{"concur-migration", no_argument, 0, 'C'},
	{"opt-migration", no_argument, 0, 'o'},
	{"basic-exchange-pages", no_argument, 0, 'b'},
	{"concur-only-exchange-pages", no_argument, 0, 'c'},
	{"exchange-pages", no_argument, 0, 'e'},
	{"move-hot-and-cold-pages", no_argument, 0, 'm'},
	{"shrink-page-lists", no_argument, 0, 's'},

	{0,0,0,0}
};


static void usage(const char *appname)
{
	printf("%s 		--pid=<PID> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node> [--max-pages] \\\n", appname);
	printf("		--non-thp-migration|--thp-migration \\\n");
	printf("		[--nomigration|--concur-migration|--opt-migration|--basic-exchange-pages|--concur-only-exchange-pages|--exchange-pages|--move-hot-and-cold-pages|--shrink-page-lists]\n");
}

int main(int argc, char **argv)
{

	int max_nodes;
	int ret;

	char c;
	int option_index = 0;

	setbuf(stdout, NULL);

	if(argc < 5) {
		usage(argv[0]);
		exit(0);
	}

	while ((c = getopt_long(argc, argv, "F:S:P:NtTCobcems", long_options, &option_index)) != -1) {
		switch (c) {
			case 'p':
				pid = atoi(optarg);
				break;
			case 'F':
				fastmem_mask = numa_parse_nodestring(optarg);
				fastmem_node = atoi(optarg);
				break;
			case 'S':
				slowmem_mask = numa_parse_nodestring(optarg);
				slowmem_node = atoi(optarg);
				break;
			case 'P':
				managed_pages = atol(optarg);
				break;
			case 'N':
				migration = 0;
				break;
			case 't':
				thp_migration=0;
				break;
			case 'T':
				thp_migration=1;
				break;
			case 'C':
				concur_migration=1;
				break;
			case 'o':
				opt_migration=1;
				break;
			case 'b':
				basic_exchange_pages=1;
				break;
			case 'c':
				concur_only_exchange_pages=1;
				break;
			case 'e':
				exchange_pages=1;
				break;
			case 'm':
				move_hot_and_cold_pages=1;
				break;
			case 's':
				shrink_page_lists=1;
				break;
			default:
				abort();
		}
	}



	if (	thp_migration + concur_migration + opt_migration + \
			basic_exchange_pages + concur_only_exchange_pages + exchange_pages > 1) {
		perror("--thp-migration|--concur-migration|--opt-migration|--basic-exchange-pages|--concur-only-exchange-pages|--exchange-pages can be used one of them at one time\n");
		exit(-1);
	}

	if (migration)
		mm_manage_flags |= MPOL_MF_MOVE;

	if (concur_migration)
		mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR;

	if (opt_migration)
		mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR;

	if (basic_exchange_pages)
		mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_EXCHANGE;

	if (concur_only_exchange_pages)
		mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE;

	if (exchange_pages)
		mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE;

	if (shrink_page_lists)
		mm_manage_flags |= MPOL_MF_SHRINK_LISTS;

	if (move_hot_and_cold_pages)
		mm_manage_flags |= MPOL_MF_MOVE_ALL;

	if (no_migration)
		mm_manage_flags &= ~MPOL_MF_MOVE;


	max_nodes = numa_num_possible_nodes();

	printf("Trigger page migration pid = %d %d -> %d...\n", pid, slowmem_node, fastmem_node);

	ret = syscall(syscall_mm_manage, pid, managed_pages, max_nodes + 1, slowmem_mask->maskp, fastmem_mask->maskp, mm_manage_flags);

	printf("Page migration done pid = %d, ret = %d\n", pid, ret);

	return ret;
}



