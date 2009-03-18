#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<assert.h>
#include<limits.h>

#include "cmph_structs.h"
#include "chd_structs_ph.h"
#include "chd_ph.h"
#include"miller_rabin.h"

#define DEBUG
#include "debug.h"

// NO_ELEMENT is equivalent to null pointer
#ifndef NO_ELEMENT
#define NO_ELEMENT UINT_MAX
#endif

// struct to represents the buckets items
struct _chd_ph_item_t
{
	cmph_uint32 f;
	cmph_uint32 h;
	struct _chd_ph_item_t * next;
};
typedef struct _chd_ph_item_t chd_ph_item_t;


// struct to represent a bucket
struct _chd_ph_bucket_t
{
	cmph_uint32 size;
	chd_ph_item_t * items_list;
	cmph_uint32 next_in_list;
};
typedef struct _chd_ph_bucket_t chd_ph_bucket_t;

static inline chd_ph_bucket_t * chd_ph_bucket_new(cmph_uint32 nbuckets);
static inline void chd_ph_bucket_clean(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets);
static inline cmph_uint8 chd_ph_bucket_insert(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets, cmph_uint32 g, chd_ph_item_t * item);
static inline void chd_ph_bucket_destroy(chd_ph_bucket_t * buckets);

chd_ph_bucket_t * chd_ph_bucket_new(cmph_uint32 nbuckets)
{
    chd_ph_bucket_t * buckets = (chd_ph_bucket_t *) calloc(nbuckets, sizeof(chd_ph_bucket_t));
    return buckets;
}

void chd_ph_bucket_clean(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets)
{
	register cmph_uint32 i = 0;
	assert(buckets);
	for(i = 0; i < nbuckets; i++)
	{
		buckets[i].size = 0;
		buckets[i].items_list = 0;
		buckets[i].next_in_list = NO_ELEMENT;
	};
}

cmph_uint8 chd_ph_bucket_insert(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets, cmph_uint32 g, chd_ph_item_t * item)
{
	chd_ph_item_t * item1, * prior_item1;
	item1 = buckets[g].items_list;
	prior_item1 = 0;
	while(item1 != 0 && (item1->f < item->f || (item1->f == item->f && item1->h < item->h)) )
	{
		prior_item1 = item1;
		item1 = item1->next;
	};
	
	if(item1 != 0 && item1->f == item->f && item1->h == item->h)
	{
		DEBUGP("Item not added\n");
		return 0;
	};
	item->next = item1;
	if(prior_item1 == 0)
		buckets[g].items_list = item;
	else
		prior_item1->next = item;
	
	buckets[g].size++;
	
	return 1;
}

void chd_ph_bucket_destroy(chd_ph_bucket_t * buckets)
{
    free(buckets);
}

static inline cmph_uint8 chd_ph_mapping(cmph_config_t *mph, chd_ph_bucket_t * buckets, chd_ph_item_t * items, 
					cmph_uint32 *max_bucket_size);

static inline cmph_uint32 * chd_ph_ordering(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets, cmph_uint32 max_bucket_size);

static inline cmph_uint8 chd_ph_searching(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint32 max_bucket_size,
                                          cmph_uint32 *sorted_lists, cmph_uint32 max_probes, cmph_uint32 * disp_table, 
					  cmph_uint8 * occup_table);

static inline double chd_ph_space_lower_bound(cmph_uint32 _n, cmph_uint32 _r)
{
	double r = _r, n = _n;
	return (1 + (r/n - 1.0 + 1.0/(2.0*n))*log(1 - n/r))/log(2);
};

/* computes the entropy of non empty buckets.*/
static inline double chd_ph_get_entropy(cmph_uint32 * disp_table, cmph_uint32 n, cmph_uint32 max_probes)
{
	register cmph_uint32 * probe_counts = (cmph_uint32 *) calloc(max_probes, sizeof(cmph_uint32));
	register cmph_uint32 i;
	register double entropy = 0;

	for(i = 0; i < n; i++)
	{
		probe_counts[disp_table[i]]++;
	};
	
	for(i = 0; i < max_probes; i++)
	{
		if(probe_counts[i] > 0)
			entropy -= probe_counts[i]*log((double)probe_counts[i]/(double)n)/log(2);
	};
	free(probe_counts);
	return entropy;
};

chd_ph_config_data_t *chd_ph_config_new()
{
	chd_ph_config_data_t *chd_ph;
	chd_ph = (chd_ph_config_data_t *)malloc(sizeof(chd_ph_config_data_t));
	assert(chd_ph);
	memset(chd_ph, 0, sizeof(chd_ph_config_data_t));
	
	chd_ph->hashfunc = CMPH_HASH_JENKINS;
	chd_ph->cs = NULL;
	chd_ph->nbuckets = 0;
	chd_ph->n = 0;
	chd_ph->hl = NULL;

	chd_ph->m = 0;
	chd_ph->use_h = 1;
	chd_ph->keys_per_bin = 1;
	chd_ph->keys_per_bucket = 4;
	
	//The following fields are used just for statistics
	chd_ph->space_usage = 0;
	chd_ph->entropy = 0.0;
	return chd_ph;
}

void chd_ph_config_destroy(cmph_config_t *mph)
{
	chd_ph_config_data_t *data = (chd_ph_config_data_t *) mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
}


void chd_ph_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	chd_ph_config_data_t *chd_ph = (chd_ph_config_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint32 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 1) break; //chd_ph only uses one linear hash function
		chd_ph->hashfunc = *hashptr;	
		++i, ++hashptr;
	}
}


void chd_ph_config_set_b(cmph_config_t *mph, cmph_uint32 keys_per_bucket)
{
	assert(mph);
	chd_ph_config_data_t *chd_ph = (chd_ph_config_data_t *)mph->data;
	if(keys_per_bucket <= 1 || keys_per_bucket >= 15)
	{
	    keys_per_bucket = 4;
	}
	chd_ph->keys_per_bucket = keys_per_bucket;
}


void chd_ph_config_set_keys_per_bin(cmph_config_t *mph, cmph_uint32 keys_per_bin)
{
	assert(mph);
	chd_ph_config_data_t *chd_ph = (chd_ph_config_data_t *)mph->data;
	if(keys_per_bin <= 1 || keys_per_bin >= 128)
	{
	    keys_per_bin = 1;
	}
	chd_ph->keys_per_bin = keys_per_bin;
}

cmph_uint8 chd_ph_mapping(cmph_config_t *mph, chd_ph_bucket_t * buckets, chd_ph_item_t * items, cmph_uint32 *max_bucket_size)
{
	register cmph_uint32 i = 0, g = 0;
	cmph_uint32 hl[3];
	chd_ph_config_data_t *chd_ph = (chd_ph_config_data_t *)mph->data;
	char * key = NULL;
	cmph_uint32 keylen = 0;
	chd_ph_item_t * item;
	register cmph_uint32 mapping_iterations = 1000;
	*max_bucket_size = 0;
	while(1)
	{
		mapping_iterations--;
		if (chd_ph->hl) hash_state_destroy(chd_ph->hl);
		chd_ph->hl = hash_state_new(chd_ph->hashfunc, chd_ph->m); 

		chd_ph_bucket_clean(buckets, chd_ph->nbuckets);

		mph->key_source->rewind(mph->key_source->data);  

		for(i = 0; i < chd_ph->m; i++)
		{
			mph->key_source->read(mph->key_source->data, &key, &keylen);		
			hash_vector(chd_ph->hl, key, keylen, hl);
			
			item = (items + i);

			g = hl[0] % chd_ph->nbuckets;
			item->f = hl[1] % chd_ph->n;
			item->h = hl[2] % (chd_ph->n - 1) + 1;
			
			mph->key_source->dispose(mph->key_source->data, key, keylen);		
			
// 			if(buckets[g].size == (chd_ph->keys_per_bucket << 2))
// 			{
// 				DEBUGP("BUCKET = %u -- SIZE = %u -- MAXIMUM SIZE = %u\n", g, buckets[g].size, (chd_ph->keys_per_bucket << 2));
// 				goto error;
// 			}
			
			if(!chd_ph_bucket_insert(buckets, chd_ph->nbuckets, g, item))
			{
				break;
			}
			
			if(buckets[g].size > *max_bucket_size)
			{
				  *max_bucket_size = buckets[g].size;
			}
		}
		
		if(i == chd_ph->m)
		{
			return 1; // SUCCESS
		}
		
		if(mapping_iterations == 0)
		{
		      goto error;
		}
	}
error:
	hash_state_destroy(chd_ph->hl);
	chd_ph->hl = NULL;
	return 0; // FAILURE

}

cmph_uint32 * chd_ph_ordering(chd_ph_bucket_t * buckets, cmph_uint32 nbuckets, cmph_uint32 max_bucket_size)
{
	cmph_uint32 * sorted_lists = (cmph_uint32 *) calloc(max_bucket_size + 1, sizeof(cmph_uint32));
	register cmph_uint32 i, size;
	DEBUGP("MAX BUCKET SIZE = %u\n", max_bucket_size);
	for(i = 0; i <= max_bucket_size; i++)
	{
		sorted_lists[i] = NO_ELEMENT;
	}
	for(i = 0; i < nbuckets; i++)
	{
		size = buckets[i].size;
		if(size == 0)
			continue;
		buckets[i].next_in_list = sorted_lists[size];
		sorted_lists[size] = i;
	};

	return sorted_lists;
}

static inline cmph_uint8 place_bucket_probe(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint8 * occup_table, 
                                            cmph_uint32 probe0_num, cmph_uint32 probe1_num, cmph_uint32 bucket_num)
{
	register cmph_uint32 i;
	register cmph_uint32 size = buckets[bucket_num].size;
	register chd_ph_item_t * item;
	register cmph_uint32 position;

	item = buckets[bucket_num].items_list;
	// try place bucket with probe_num
	for(i = 0; i < size; i++) // placement
	{
		position = (item->f + ((cmph_uint64)item->h)*probe0_num + probe1_num) % chd_ph->n;
		
		if(occup_table[position] >= chd_ph->keys_per_bin)
		{
			break;
		}
		occup_table[position]++;
		
		item = item->next;
	};
	
	if(i != size) // Undo the placement
	{
		item = buckets[bucket_num].items_list;
		while(1)
		{
			if(i == 0)
			{
				break;
			}
			position = (item->f + ((cmph_uint64 )item->h) * probe0_num + probe1_num) % chd_ph->n;
			occup_table[position]--;
			item = item->next;
			i--;
		};
		return 0;
	};	
	return 1;
};

static inline cmph_uint8 place_bucket(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint32 max_probes, 
                                      cmph_uint32 * disp_table, cmph_uint8 * occup_table, cmph_uint32 bucket_num)
				      
{
	register cmph_uint32 probe0_num, probe1_num, probe_num;
	probe0_num = 0;
	probe1_num = 0;
	probe_num = 0;
	
	while(1)
	{
		if(place_bucket_probe(chd_ph, buckets, occup_table, probe0_num, probe1_num, bucket_num))
		{
			disp_table[bucket_num] = probe0_num + probe1_num * chd_ph->n;
			return 1;
		}
		probe0_num++;
		if(probe0_num >= chd_ph->n)
		{
			probe0_num -= chd_ph->n;
			probe1_num++;
		};
		probe_num++;
		if(probe_num >= max_probes || probe1_num >= chd_ph->n)
		{
			return 0;
		};
	};
	return 0;
};

static inline cmph_uint8 place_buckets1(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint32 max_bucket_size,
                                        cmph_uint32 *sorted_lists, cmph_uint32 max_probes, cmph_uint32 * disp_table, 
					cmph_uint8 * occup_table)
{
	register cmph_uint32 i = 0;
	register cmph_uint32 curr_bucket = 0;

	for(i = max_bucket_size; i > 0; i--)
	{
		curr_bucket = sorted_lists[i];
		while(curr_bucket != NO_ELEMENT)
		{
			if(!place_bucket(chd_ph, buckets, max_probes, disp_table, occup_table, curr_bucket))
			{
				return 0;
			}
			curr_bucket = buckets[curr_bucket].next_in_list;
		};
	
	};
	return 1;
};

static inline cmph_uint8 place_buckets2(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint32 max_bucket_size,
                                        cmph_uint32 *sorted_lists, cmph_uint32 max_probes, cmph_uint32 * disp_table, 
					cmph_uint8 * occup_table)
{
	register cmph_uint32 i;
	register cmph_uint32 curr_bucket, prev_bucket;
	register cmph_uint32 probe_num, probe0_num, probe1_num;
	DEBUGP("USING HEURISTIC TO PLACE BUCKETS\n");
	for(i = max_bucket_size; i > 0; i--)
	{
		probe_num = 0;
		probe0_num = 0;
		probe1_num = 0;
		while(sorted_lists[i] != NO_ELEMENT)
		{
			prev_bucket = NO_ELEMENT;
			curr_bucket = sorted_lists[i];
			while(curr_bucket != NO_ELEMENT)
			{
				// if bucket is successfully placed remove it from list
				if(place_bucket_probe(chd_ph, buckets, occup_table, probe0_num, probe1_num, curr_bucket))
				{	
					disp_table[curr_bucket] = probe0_num + probe1_num * chd_ph->n;
// 					DEBUGP("BUCKET %u PLACED --- DISPLACEMENT = %u\n", curr_bucket, disp_table[curr_bucket]);
					if(prev_bucket == NO_ELEMENT)
					{
						sorted_lists[i] = buckets[curr_bucket].next_in_list;
					}
					else
					{
						buckets[prev_bucket].next_in_list = buckets[curr_bucket].next_in_list;
					}
						
				} 
				else
				{
// 					DEBUGP("BUCKET %u NOT PLACED\n", curr_bucket);
					prev_bucket = curr_bucket;
				}
				curr_bucket = buckets[curr_bucket].next_in_list;
			};
			probe0_num++;
			if(probe0_num >= chd_ph->n)
			{
				probe0_num -= chd_ph->n;
				probe1_num++;
			};
			probe_num++;
			if(probe_num >= max_probes || probe1_num >= chd_ph->n)
			{
				return 0;
			};
		};
	};
	return 1;
};

cmph_uint8 chd_ph_searching(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, cmph_uint32 max_bucket_size,
                            cmph_uint32 *sorted_lists, cmph_uint32 max_probes, cmph_uint32 * disp_table, 
			    cmph_uint8 * occup_table)
{
	if(chd_ph->use_h)
	{
		return place_buckets2(chd_ph, buckets, max_bucket_size, sorted_lists, max_probes, disp_table, occup_table);
	}
	else
	{
		return place_buckets1(chd_ph, buckets, max_bucket_size, sorted_lists, max_probes, disp_table, occup_table);
	}

}

static inline cmph_uint8 chd_ph_check_bin_hashing(chd_ph_config_data_t *chd_ph, chd_ph_bucket_t *buckets, 
                                                  cmph_uint32 * disp_table, cmph_uint8 * occup_table)
{
	register cmph_uint32 i, j;
	register cmph_uint32 position, probe0_num, probe1_num;
	register cmph_uint32 m = 0;
	register chd_ph_item_t * item;

	memset(occup_table, 0, chd_ph->n);
	for(i = 0; i < chd_ph->nbuckets; i++)
	{
		j = buckets[i].size;
		item = buckets[i].items_list;
		probe0_num = disp_table[i] % chd_ph->n;
		probe1_num = disp_table[i] / chd_ph->n;
		for(; j > 0; j--)
		{
			if(item == 0)
			{
				return 0;
			}
			m++;
			position = (item->f + ((cmph_uint64 )item->h) * probe0_num + probe1_num) % chd_ph->n;
			if(occup_table[position] >= chd_ph->keys_per_bin)
			{
				return 0;
			}
			occup_table[position]++;
			item = item->next;
		};
	};
	DEBUGP("We were able to place m = %u keys\n", m);
	return 1;
};


cmph_t *chd_ph_new(cmph_config_t *mph, double c)
{
	cmph_t *mphf = NULL;
	chd_ph_data_t *chd_phf = NULL;
	chd_ph_config_data_t *chd_ph = (chd_ph_config_data_t *)mph->data;
	
	register double load_factor = c;
	register cmph_uint8 searching_success = 0;
	register cmph_uint32 max_probes = 1 << 18; // default value for max_probes
	register cmph_uint32 iterations = 100;
	chd_ph_bucket_t * buckets = NULL;
	chd_ph_item_t * items = NULL;
	register cmph_uint8 failure = 0;
	cmph_uint32 max_bucket_size = 0;
	cmph_uint32 * sorted_lists = NULL;
	cmph_uint32 * disp_table = NULL;
	cmph_uint8 * occup_table;
	
	chd_ph->m = mph->key_source->nkeys;
	DEBUGP("m = %u\n", chd_ph->m);
	
	chd_ph->nbuckets = (cmph_uint32)(chd_ph->m/chd_ph->keys_per_bucket) + 1;
	DEBUGP("nbuckets = %u\n", chd_ph->nbuckets);
	
	if(load_factor < 0.5 )
	{
		load_factor = 0.5;
	}
	
	if(load_factor >= 0.99)
	{
		load_factor = 0.99;
	}
	
	DEBUGP("load_factor = %.3f\n", load_factor);
	
	chd_ph->n = (cmph_uint32)(chd_ph->m/(chd_ph->keys_per_bin * load_factor)) + 1;
	
	//Round the number of bins to the prime immediately above
	if(chd_ph->n % 2 == 0) chd_ph->n++;
	for(;;)
	{
		if(check_primality(chd_ph->n) == 1)
			break;
		chd_ph->n += 2; // just odd numbers can be primes for n > 2
		
	};
	
	DEBUGP("n = %u \n", chd_ph->n);
	
	if(mph->verbosity && chd_ph->keys_per_bin == 1)
	{
		fprintf(stderr, "space lower bound is %.3f bits per key", chd_ph_space_lower_bound(chd_ph->m, chd_ph->n));
	}

       	// We allocate the working tables
	buckets = chd_ph_bucket_new(chd_ph->nbuckets); 
	items   = (chd_ph_item_t *) calloc(chd_ph->m, sizeof(chd_ph_item_t));

	max_probes = (cmph_uint32)(((log(chd_ph->m)/log(2))/20) * max_probes);
	occup_table = (cmph_uint8 *) calloc(chd_ph->n, sizeof(cmph_uint8));
	disp_table = (cmph_uint32 *) calloc(chd_ph->nbuckets, sizeof(cmph_uint32));
// 	
// 	init_genrand(time(0));
	
	while(1)
	{
		iterations --;
		if (mph->verbosity)
		{
			fprintf(stderr, "Starting mapping step for mph creation of %u keys with %u bins\n", chd_ph->m, chd_ph->n);
		}
		
		if(!chd_ph_mapping(mph, buckets, items, &max_bucket_size))
		{
			if (mph->verbosity)
			{
				fprintf(stderr, "Failure in mapping step\n");		
			}
			failure = 1;
			goto cleanup;
		}

		if (mph->verbosity)
		{
			fprintf(stderr, "Starting ordering step\n");
		}
		if(sorted_lists)
		{
			free(sorted_lists);
		}
        	sorted_lists = chd_ph_ordering(buckets, chd_ph->nbuckets, max_bucket_size);
		
		if (mph->verbosity)
		{
			fprintf(stderr, "Starting searching step\n");
		}
		
		searching_success = chd_ph_searching(chd_ph, buckets, max_bucket_size, sorted_lists, max_probes, disp_table, occup_table);
			
		if(searching_success) break;
		
		// reset occup_table
		memset(occup_table, 0, chd_ph->n);
		if(iterations == 0)
		{
			// Cleanup memory
			if (mph->verbosity)
			{
				fprintf(stderr, "Failure because the max trials was exceeded\n");
			}
			failure = 1;
			goto cleanup;
		};
	}

	#ifdef DEBUG	
	chd_ph->entropy = chd_ph_get_entropy(disp_table, chd_ph->nbuckets, max_probes);
	DEBUGP("Entropy = %.4f\n", chd_ph->entropy/chd_ph->m);

	if(!chd_ph_check_bin_hashing(chd_ph, buckets, disp_table, occup_table))
	{
	
		DEBUGP("Error for bin packing generation");
		return NULL;
	};
	#endif
	
	if (mph->verbosity)
	{
		fprintf(stderr, "Starting compressing step\n");
	}
	
	if(chd_ph->cs)
	{
		free(chd_ph->cs);
	}
	chd_ph->cs = (compressed_seq_t *) calloc(1, sizeof(compressed_seq_t));
	compressed_seq_init(chd_ph->cs);
	compressed_seq_generate(chd_ph->cs, disp_table, chd_ph->nbuckets);
	chd_ph->space_usage = compressed_seq_get_space_usage(chd_ph->cs);
	chd_ph->space_usage += 64;
	DEBUGP("space_usage/key = %.4f\n", chd_ph->space_usage/(double)chd_ph->m);
	
cleanup:
	chd_ph_bucket_destroy(buckets); 
	free(items);
	free(sorted_lists);
	free(disp_table);
	free(occup_table);
	if(failure) 
	{
		if(chd_ph->hl)
		{
			hash_state_destroy(chd_ph->hl);
		}
		chd_ph->hl = NULL;
		return NULL;
	}

	mphf = (cmph_t *)malloc(sizeof(cmph_t));
	mphf->algo = mph->algo;
	chd_phf = (chd_ph_data_t *)malloc(sizeof(chd_ph_data_t));
	
	chd_phf->cs = chd_ph->cs;
	chd_ph->cs = NULL; //transfer memory ownership
	chd_phf->hl = chd_ph->hl;
	chd_ph->hl = NULL; //transfer memory ownership
	chd_phf->n = chd_ph->n;
	chd_phf->nbuckets = chd_ph->nbuckets;
	
	mphf->data = chd_phf;
	mphf->size = chd_ph->n;

	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}

	return mphf;
}



void chd_ph_load(FILE *fd, cmph_t *mphf)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	register cmph_uint32 nbytes;
	chd_ph_data_t *chd_ph = (chd_ph_data_t *)malloc(sizeof(chd_ph_data_t));

	DEBUGP("Loading chd_ph mphf\n");
	mphf->data = chd_ph;

	nbytes = fread(&buflen, sizeof(cmph_uint32), (size_t)1, fd);
	DEBUGP("Hash state has %u bytes\n", buflen);
	buf = (char *)malloc((size_t)buflen);
	nbytes = fread(buf, (size_t)buflen, (size_t)1, fd);
	chd_ph->hl = hash_state_load(buf, buflen);
	free(buf);
	
	nbytes = fread(&buflen, sizeof(cmph_uint32), (size_t)1, fd);
	DEBUGP("Compressed sequence structure has %u bytes\n", buflen);
	buf = (char *)malloc((size_t)buflen);
	nbytes = fread(buf, (size_t)buflen, (size_t)1, fd);
	chd_ph->cs = (compressed_seq_t *) calloc(1, sizeof(compressed_seq_t)); 
	compressed_seq_load(chd_ph->cs, buf, buflen);
	free(buf);
	
	// loading n and nbuckets
	DEBUGP("Reading n and nbuckets\n");
	nbytes = fread(&(chd_ph->n), sizeof(cmph_uint32), (size_t)1, fd);	
	nbytes = fread(&(chd_ph->nbuckets), sizeof(cmph_uint32), (size_t)1, fd);	
}

int chd_ph_dump(cmph_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	register cmph_uint32 nbytes;
	chd_ph_data_t *data = (chd_ph_data_t *)mphf->data;
	
	__cmph_dump(mphf, fd);

	hash_state_dump(data->hl, &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	nbytes = fwrite(&buflen, sizeof(cmph_uint32), (size_t)1, fd);
	nbytes = fwrite(buf, (size_t)buflen, (size_t)1, fd);
	free(buf);

	compressed_seq_dump(data->cs, &buf, &buflen);
	DEBUGP("Dumping compressed sequence structure with %u bytes to disk\n", buflen);
	nbytes = fwrite(&buflen, sizeof(cmph_uint32), (size_t)1, fd);
	nbytes = fwrite(buf, (size_t)buflen, (size_t)1, fd);
	free(buf);

	// dumping n and nbuckets
	nbytes = fwrite(&(data->n), sizeof(cmph_uint32), (size_t)1, fd);
	nbytes = fwrite(&(data->nbuckets), sizeof(cmph_uint32), (size_t)1, fd);
	return 1;
}

void chd_ph_destroy(cmph_t *mphf)
{
	chd_ph_data_t *data = (chd_ph_data_t *)mphf->data;
	compressed_seq_destroy(data->cs);
	free(data->cs);
	hash_state_destroy(data->hl);
	free(data);
	free(mphf);

}

cmph_uint32 chd_ph_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	register chd_ph_data_t * chd_ph = mphf->data;
	cmph_uint32 hl[3];
	register cmph_uint32 disp,position;
	register cmph_uint32 probe0_num,probe1_num;
	register cmph_uint32 f,g,h;
	hash_vector(chd_ph->hl, key, keylen, hl);	
	g = hl[0] % chd_ph->nbuckets;
	f = hl[1] % chd_ph->n;
	h = hl[2] % (chd_ph->n-1) + 1;
	
	disp = compressed_seq_query(chd_ph->cs, g);
	probe0_num = disp % chd_ph->n;
	probe1_num = disp/chd_ph->n;
	position = (f + ((cmph_uint64 )h)*probe0_num + probe1_num) % chd_ph->n;
	return position;
}

void chd_ph_pack(cmph_t *mphf, void *packed_mphf)
{
	chd_ph_data_t *data = (chd_ph_data_t *)mphf->data;
	cmph_uint8 * ptr = packed_mphf;

	// packing hl type
	CMPH_HASH hl_type = hash_get_type(data->hl);
	*((cmph_uint32 *) ptr) = hl_type;
	ptr += sizeof(cmph_uint32);

	// packing hl
	hash_state_pack(data->hl, ptr);
	ptr += hash_state_packed_size(hl_type);

	// packing n
	*((cmph_uint32 *) ptr) = data->n;
	ptr += sizeof(data->n);

	// packing nbuckets
	*((cmph_uint32 *) ptr) = data->nbuckets;
	ptr += sizeof(data->nbuckets);

	// packing cs
	compressed_seq_pack(data->cs, ptr);
	//ptr += compressed_seq_packed_size(data->cs);

}

cmph_uint32 chd_ph_packed_size(cmph_t *mphf)
{
	register chd_ph_data_t *data = (chd_ph_data_t *)mphf->data;
	register CMPH_HASH hl_type = hash_get_type(data->hl); 
	register cmph_uint32 hash_state_pack_size =  hash_state_packed_size(hl_type);
	register cmph_uint32 cs_pack_size = compressed_seq_packed_size(data->cs);
	
	return (sizeof(CMPH_ALGO) + hash_state_pack_size + cs_pack_size + 3*sizeof(cmph_uint32));

}

cmph_uint32 chd_ph_search_packed(void *packed_mphf, const char *key, cmph_uint32 keylen)
{
	register CMPH_HASH hl_type  = *(cmph_uint32 *)packed_mphf;
	register cmph_uint8 *hl_ptr = (cmph_uint8 *)(packed_mphf) + 4;
	
	register cmph_uint32 * ptr = (cmph_uint32 *)(hl_ptr + hash_state_packed_size(hl_type));
	register cmph_uint32 n = *ptr++;
	register cmph_uint32 nbuckets = *ptr++;
	cmph_uint32 hl[3];
		
	register cmph_uint32 disp,position;
	register cmph_uint32 probe0_num,probe1_num;
	register cmph_uint32 f,g,h;
	
	hash_vector_packed(hl_ptr, hl_type, key, keylen, hl);

	g = hl[0] % nbuckets;
	f = hl[1] % n;
	h = hl[2] % (n-1) + 1;
	
	disp = compressed_seq_query_packed(ptr, g);
	probe0_num = disp % n;
	probe1_num = disp/n;
	position = (f + ((cmph_uint64 )h)*probe0_num + probe1_num) % n;
	return position;
}



