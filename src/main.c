#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <assert.h>
#include "cmph.h"
#include "hash.h"
#include "../wingetopt.h"

#ifdef WIN32
#define VERSION "0.2"
#else
#include "config.h"
#endif


void usage(const char *prg)
{
	fprintf(stderr, "usage: %s [-v] [-h] [-V] [-k nkeys] [-g [-c value][-s seed] ] [-m file.mph] [-a algorithm] keysfile\n", prg);   
}
void usage_long(const char *prg)
{
	uint32 i;
	fprintf(stderr, "usage: %s [-v] [-h] [-V] [-k] [-g [-s seed] ] [-m file.mph] [-a algorithm] keysfile\n", prg);   
	fprintf(stderr, "Minimum perfect hashing tool\n\n"); 
	fprintf(stderr, "  -h\t print this help message\n");
	fprintf(stderr, "  -c\t c value that determines the number of vertices in the graph\n");
	fprintf(stderr, "  -a\t algorithm - valid values are\n");
	for (i = 0; i < MPH_COUNT; ++i) fprintf(stderr, "    \t  * %s\n", mph_names[i]);
	fprintf(stderr, "  -f\t hash function (may be used multiple times) - valid values are\n");
	for (i = 0; i < HASH_COUNT; ++i) fprintf(stderr, "    \t  * %s\n", hash_names[i]);
	fprintf(stderr, "  -V\t print version number and exit\n");
	fprintf(stderr, "  -v\t increase verbosity (may be used multiple times)\n");
	fprintf(stderr, "  -k\t number of keys\n");
	fprintf(stderr, "  -g\t generation mode\n");
	fprintf(stderr, "  -s\t random seed\n");
	fprintf(stderr, "  -m\t minimum perfect hash function file \n");
	fprintf(stderr, "  keysfile\t line separated file with keys\n");
}

static int key_read(void *data, char **key, uint32 *keylen)
{
	FILE *fd = (FILE *)data;
	*key = NULL;
	*keylen = 0;
	while(1)
	{
		char buf[BUFSIZ];
		char *c = fgets(buf, BUFSIZ, fd); 
		if (c == NULL) return -1;
		if (feof(fd)) return -1;
		*key = (char *)realloc(*key, *keylen + strlen(buf) + 1);
		memcpy(*key + *keylen, buf, strlen(buf));
		*keylen += (uint32)strlen(buf);
		if (buf[strlen(buf) - 1] != '\n') continue;
		break;
	}
	if ((*keylen) && (*key)[*keylen - 1] == '\n')
	{
		(*key)[(*keylen) - 1] = 0;
		--(*keylen);
	}
	return *keylen;
}

static void key_dispose(void *data, char *key, uint32 keylen)
{
	free(key);
}
static void key_rewind(void *data)
{
	FILE *fd = (FILE *)data;
	rewind(fd);
}

static uint32 count_keys(FILE *fd)
{
	uint32 count = 0;
	rewind(fd);
	while(1)
	{
		char buf[BUFSIZ];
		fgets(buf, BUFSIZ, fd); 
		if (feof(fd)) break;
		if (buf[strlen(buf) - 1] != '\n') continue;
		++count;
	}
	rewind(fd);
	return count;
}

int main(int argc, char **argv)
{
	char verbosity = 0;		
	char generate = 0;
	char *mphf_file = NULL;
	FILE *mphf_fd = stdout;
	const char *keys_file = NULL;
	FILE *keys_fd;
	uint32 nkeys = UINT_MAX;
	uint32 seed = UINT_MAX;
	CMPH_HASH *hashes = NULL;
	uint32 nhashes = 0;
	uint32 i;
	MPH_ALGO mph_algo = MPH_CZECH;
	float c = 2.09;
	mph_t *mph = NULL;
	mphf_t *mphf = NULL;

	key_source_t source;

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
				for (i = 0; i < MPH_COUNT; ++i)
				{
					if (strcmp(mph_names[i], optarg) == 0)
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
				for (i = 0; i < HASH_COUNT; ++i)
				{
					if (strcmp(hash_names[i], optarg) == 0)
					{
						hashes = (CMPH_HASH *)realloc(hashes, sizeof(CMPH_HASH) * ( nhashes + 2 ));
						hashes[nhashes] = i;
						hashes[nhashes + 1] = HASH_COUNT;
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
	if (seed == UINT_MAX) seed = (uint32)time(NULL);
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

	source.data = (void *)keys_fd;
	if (seed == UINT_MAX) seed = (uint32)time(NULL);
	if(nkeys == UINT_MAX) source.nkeys = count_keys(keys_fd);
	else source.nkeys = nkeys;
	source.read = key_read;
	source.dispose = key_dispose;
	source.rewind = key_rewind;
	
	if (generate)
	{
		//Create mphf
	
		mph = mph_new(mph_algo, &source);
		if (nhashes) mph_set_hashfuncs(mph, hashes);
		mph_set_verbosity(mph, verbosity);
		if(mph_algo == MPH_BMZ && c >= 2.0) c=1.15;
		if (c != 0) mph_set_graphsize(mph, c);
		mphf = mph_create(mph);

		if (mphf == NULL)
		{
			fprintf(stderr, "Unable to create minimum perfect hashing function\n");
			mph_destroy(mph);
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
		mphf_dump(mphf, mphf_fd);
		mphf_destroy(mphf);
		fclose(mphf_fd);
	}
	else
	{
	    uint8 * hashtable = NULL;
		mphf_fd = fopen(mphf_file, "r");
		if (mphf_fd == NULL)
		{
			fprintf(stderr, "Unable to open input file %s: %s\n", mphf_file, strerror(errno));
			free(mphf_file);
			return -1;
		}
		mphf = mphf_load(mphf_fd);
		fclose(mphf_fd);
		if (!mphf)
		{
			fprintf(stderr, "Unable to parser input file %s\n", mphf_file);
			free(mphf_file);
			return -1;
		}
		hashtable = (uint8*)malloc(source.nkeys*sizeof(uint8));
		memset(hashtable, 0, source.nkeys);
		//check all keys
		for (i = 0; i < source.nkeys; ++i)
		{
			uint32 h;
			char *buf;
			uint32 buflen = 0;
			source.read(source.data, &buf, &buflen);
			h = mphf_search(mphf, buf, buflen);
			if(hashtable[h])fprintf(stderr, "collision: %u\n",h);
			assert(hashtable[h]==0);
			hashtable[h] = 1;
			if (verbosity)
			{
				printf("%s -> %u\n", buf, h);
			}
			source.dispose(source.data, buf, buflen);
		}
		mphf_destroy(mphf);
		free(hashtable);
	}
	fclose(keys_fd);
	free(mphf_file);
	return 0;
}
