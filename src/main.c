#ifdef WIN32
#include "../wingetopt.h"
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <assert.h>
#include "cmph.h"
#include "hash.h"

#ifdef WIN32
#define VERSION "0.2"
#else
#include "config.h"
#endif


void usage(const char *prg)
{
	fprintf(stderr, "usage: %s [-v] [-h] [-V] [-k nkeys] [-f hash_function] [-g [-c value][-s seed] ] [-m file.mph] [-a algorithm] keysfile\n", prg);   
}
void usage_long(const char *prg)
{
	cmph_uint32 i;
	fprintf(stderr, "usage: %s [-v] [-h] [-V] [-k nkeys] [-f hash_function] [-g [-c value][-s seed] ] [-m file.mph] [-a algorithm] keysfile\n", prg);   
	fprintf(stderr, "Minimum perfect hashing tool\n\n"); 
	fprintf(stderr, "  -h\t print this help message\n");
	fprintf(stderr, "  -c\t c value that determines the number of vertices in the graph\n");
	fprintf(stderr, "  -a\t algorithm - valid values are\n");
	for (i = 0; i < CMPH_COUNT; ++i) fprintf(stderr, "    \t  * %s\n", cmph_names[i]);
	fprintf(stderr, "  -f\t hash function (may be used multiple times) - valid values are\n");
	for (i = 0; i < CMPH_HASH_COUNT; ++i) fprintf(stderr, "    \t  * %s\n", cmph_hash_names[i]);
	fprintf(stderr, "  -V\t print version number and exit\n");
	fprintf(stderr, "  -v\t increase verbosity (may be used multiple times)\n");
	fprintf(stderr, "  -k\t number of keys\n");
	fprintf(stderr, "  -g\t generation mode\n");
	fprintf(stderr, "  -s\t random seed\n");
	fprintf(stderr, "  -m\t minimum perfect hash function file \n");
	fprintf(stderr, "  keysfile\t line separated file with keys\n");
}


int main(int argc, char **argv)
{
	char verbosity = 0;		
	char generate = 0;
	char *mphf_file = NULL;
	FILE *mphf_fd = stdout;
	const char *keys_file = NULL;
	FILE *keys_fd;
	cmph_uint32 nkeys = UINT_MAX;
	cmph_uint32 seed = UINT_MAX;
	CMPH_HASH *hashes = NULL;
	cmph_uint32 nhashes = 0;
	cmph_uint32 i;
	CMPH_ALGO mph_algo = CMPH_CHM;
	float c = 2.09;
	cmph_config_t *config = NULL;
	cmph_t *mphf = NULL;

	cmph_io_adapter_t *source;

	while (1)
	{
		char ch = getopt(argc, argv, "hVvgc:k:a:f:m:s:");
		if (ch == -1) break;
		switch (ch)
		{
			case 's':
				{
					char *cptr;
					seed = strtoul(optarg, &cptr, 10);
					if(*cptr != 0) {
						fprintf(stderr, "Invalid seed %s\n", optarg);
						exit(1);
					}
				}
				break;
			case 'c':
				{
					char *endptr;
					c = strtod(optarg, &endptr);
					if(*endptr != 0) {
						fprintf(stderr, "Invalid c value %s\n", optarg);
						exit(1);
					}
				}
				break;
			case 'g':
				generate = 1;
				break;
			case 'k':
			        {
					char *endptr;
					nkeys = strtoul(optarg, &endptr, 10);
					if(*endptr != 0) {
						fprintf(stderr, "Invalid number of keys %s\n", optarg);
						exit(1);
					}
				}
				break;
			case 'm':
				mphf_file = strdup(optarg);
				break;
			case 'v':
				++verbosity;
				break;
			case 'V':
				printf("%s\n", VERSION);
				return 0;
			case 'h':
				usage_long(argv[0]);
				return 0;
			case 'a':
				{
				char valid = 0;
				for (i = 0; i < CMPH_COUNT; ++i)
				{
					if (strcmp(cmph_names[i], optarg) == 0)
					{
						mph_algo = i;
						valid = 1;
						break;
					}
				}
				if (!valid) 
				{
					fprintf(stderr, "Invalid mph algorithm: %s\n", optarg);
					return -1;
				}
				}
				break;
			case 'f':
				{
				char valid = 0;
				for (i = 0; i < CMPH_HASH_COUNT; ++i)
				{
					if (strcmp(cmph_hash_names[i], optarg) == 0)
					{
						hashes = (CMPH_HASH *)realloc(hashes, sizeof(CMPH_HASH) * ( nhashes + 2 ));
						hashes[nhashes] = i;
						hashes[nhashes + 1] = CMPH_HASH_COUNT;
						++nhashes;
						valid = 1;
						break;
					}
				}
				if (!valid) 
				{
					fprintf(stderr, "Invalid hash function: %s\n", optarg);
					return -1;
				}
				}
				break;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	if (optind != argc - 1)
	{
		usage(argv[0]);
		return 1;
	}
	keys_file = argv[optind];
	if (seed == UINT_MAX) seed = (cmph_uint32)time(NULL);
	srand(seed);

	if (mphf_file == NULL)
	{
		mphf_file = (char *)malloc(strlen(keys_file) + 5);
		memcpy(mphf_file, keys_file, strlen(keys_file));
		memcpy(mphf_file + strlen(keys_file), ".mph\0", 5);
	}	

	keys_fd = fopen(keys_file, "r");
	if (keys_fd == NULL)
	{
		fprintf(stderr, "Unable to open file %s: %s\n", keys_file, strerror(errno));
		return -1;
	}

	if (seed == UINT_MAX) seed = (cmph_uint32)time(NULL);
	if(nkeys == UINT_MAX) source = cmph_io_nlfile_adapter(keys_fd);
	else source = cmph_io_nlnkfile_adapter(keys_fd, nkeys);

	if (generate)
	{
		//Create mphf
		config = cmph_config_new(source);
		cmph_config_set_algo(config, mph_algo);
		if (nhashes) cmph_config_set_hashfuncs(config, hashes);
		cmph_config_set_verbosity(config, verbosity);
		if(mph_algo == CMPH_BMZ && c >= 2.0) c=1.15;
		if (c != 0) cmph_config_set_graphsize(config, c);
		mphf = cmph_new(config);

		if (mphf == NULL)
		{
			fprintf(stderr, "Unable to create minimum perfect hashing function\n");
			cmph_config_destroy(config);
			free(mphf_file);
			return -1;
		}

		mphf_fd = fopen(mphf_file, "w");
		if (mphf_fd == NULL)
		{
			fprintf(stderr, "Unable to open output file %s: %s\n", mphf_file, strerror(errno));
			free(mphf_file);
			return -1;
		}
		cmph_dump(mphf, mphf_fd);
		cmph_destroy(mphf);
		fclose(mphf_fd);
	}
	else
	{
		cmph_uint8 * hashtable = NULL;
		mphf_fd = fopen(mphf_file, "r");
		if (mphf_fd == NULL)
		{
			fprintf(stderr, "Unable to open input file %s: %s\n", mphf_file, strerror(errno));
			free(mphf_file);
			return -1;
		}
		mphf = cmph_load(mphf_fd);
		fclose(mphf_fd);
		if (!mphf)
		{
			fprintf(stderr, "Unable to parser input file %s\n", mphf_file);
			free(mphf_file);
			return -1;
		}
		hashtable = (cmph_uint8*)malloc(source->nkeys*sizeof(cmph_uint8));
		memset(hashtable, 0, source->nkeys);
		//check all keys
		for (i = 0; i < source->nkeys; ++i)
		{
			cmph_uint32 h;
			char *buf;
			cmph_uint32 buflen = 0;
			source->read(source->data, &buf, &buflen);
			h = cmph_search(mphf, buf, buflen);
			if(hashtable[h])fprintf(stderr, "collision: %u\n",h);
			assert(hashtable[h]==0);
			hashtable[h] = 1;
			if (verbosity)
			{
				printf("%s -> %u\n", buf, h);
			}
			source->dispose(source->data, buf, buflen);
		}
		cmph_destroy(mphf);
		free(hashtable);
	}
	fclose(keys_fd);
	free(mphf_file);
	free(source);
	return 0;
}
