/*
 * 
 * Author: Jiaolin Luo
 * refer: nimble_page_management_asplos_2019/mm/memory_manage.c
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
//static int thp_migration=1;

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

	//{"thp-migration", required_argument, 0, 't'},

	{"concur-migration", no_argument, 0, 'C'},
	{"opt-migration", no_argument, 0, 'o'},
	{"basic-exchange-pages", no_argument, 0, 'b'},
	{"concur-only-exchange-pages", no_argument, 0, 'c'},
	{"exchange-pages", no_argument, 0, 'e'},
	{"move-hot-and-cold-pages", no_argument, 0, 'm'},
	{"shrink-page-lists", no_argument, 0, 's'},

	{0,0,0,0}
};

//
//
// nimble_control 	--pid=xxx --fast-mem-node=1 --slow-mem-node=0 --managed-pages=max
// 					--thp-migration
// 					--move-hot-and-cold-pages
//
//
//
static void usage(const char *appname)
{
	printf("%s 		--pid=<PID> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node> [--max-pages=<max|number-of-pages>] \\\n", appname);
	//printf("		[--thp-migration=<1|0>] \\\n");
	printf("		[[--concur-migration|--opt-migration|--basic-exchange-pages|--concur-only-exchange-pages|--exchange-pages]|--nomigration] \\\n");
	printf("		[--move-hot-and-cold-pages] \\\n");
	printf("		[--shrink-page-lists]\n");

	printf("Example 1: enable all nimble migration features and move as maximum pages as possible.\n");
	printf("%s 		--pid=<PID> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node>\n", appname);

	printf("Example 2: enable all nimble migration features and move pages up to 10000.\n");
	printf("%s 		--pid=<PID> --fast-mem-node=<fast-mem-node> --slow-mem-node=<slow-mem-node> --max-pages=10000\\\n", appname);

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

	while ((c = getopt_long(argc, argv, "p:F:S:P:NCobcems", long_options, &option_index)) != -1) {
		switch (c) {
			case 'p':
				pid = atoi(optarg);

				if(pid == 0) {
					perror("Invalid --pid\n");
					exit(-1);
				}

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
				if(strcmp(optarg, "max") == 0)
					managed_pages = ULONG_MAX;
				else
					managed_pages = atol(optarg);
				break;
			case 'N':
				migration = 0;
				break;
#if 0
			case 't':
				thp_migration= atoi(optarg);

				if(thp_migration != 0 && thp_migration != 1)
					thp_migration = 1;

				break;
#endif
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
				//break;
				abort();
		}
	}


     

	if (	concur_migration + opt_migration + basic_exchange_pages + concur_only_exchange_pages + exchange_pages > 1) {
		perror("--concur-migration|--opt-migration|--basic-exchange-pages|--concur-only-exchange-pages|--exchange-pages can be used one of them at one time\n");
		exit(-1);
	}


	if (migration == 0) {
		mm_manage_flags &= ~MPOL_MF_MOVE;
		printf("Clear MPOL_MF_MOVE\n");
	} else {
		mm_manage_flags |= MPOL_MF_MOVE;

		printf("Set MPOL_MF_MOVE\n");

		if(exchange_pages + concur_only_exchange_pages + basic_exchange_pages + opt_migration + concur_migration == 0) {
			exchange_pages = 1;
		} 

		if (concur_migration) {
			mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR;
			printf("Add MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR\n");
		}

		if (opt_migration) {
			mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR;
			printf("Add MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR\n");
		}

		if (basic_exchange_pages) {
			mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_EXCHANGE;
			printf("Add MPOL_MF_MOVE|MPOL_MF_EXCHANGE\n");
		}

		if (concur_only_exchange_pages) {
			mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE;
			printf("Add MPOL_MF_MOVE|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE\n");
		}

		if (exchange_pages) {
			mm_manage_flags |= MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE;
			printf("Add MPOL_MF_MOVE|MPOL_MF_MOVE_MT|MPOL_MF_MOVE_CONCUR|MPOL_MF_EXCHANGE\n");
		}
	}

	if(move_hot_and_cold_pages) {
		mm_manage_flags |= MPOL_MF_MOVE_ALL;
		printf("Add MPOL_MF_MOVE_ALL\n");
	}

	if (shrink_page_lists) {
		mm_manage_flags |= MPOL_MF_SHRINK_LISTS;
		printf("Add MPOL_MF_SHRINK_LISTS\n");
	}

	if (mm_manage_flags & ~(
                   MPOL_MF_MOVE|
                  MPOL_MF_MOVE_MT|
                   MPOL_MF_MOVE_DMA|
                   MPOL_MF_MOVE_CONCUR|
                   MPOL_MF_EXCHANGE|
                   MPOL_MF_SHRINK_LISTS|
                   MPOL_MF_MOVE_ALL)) {
		usage(argv[0]);
		exit(0);
	}



	max_nodes = numa_num_possible_nodes();

	printf("Trigger page migration pid = %d slow #%d -> fast #%d managed_pages = %lu ...\n", pid, slowmem_node, fastmem_node, managed_pages);

	ret = syscall(syscall_mm_manage, pid, managed_pages, max_nodes + 1, slowmem_mask->maskp, fastmem_mask->maskp, mm_manage_flags);

	if(ret)
		fprintf(stderr, "Page migration failed pid = %d, ret = %d\n", pid, ret);
	else
		printf("Page migration finished for pid = %d with ret = %d\n", pid, ret);

	exit(ret);
}



