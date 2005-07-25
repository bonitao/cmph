#include <cmph.h>

// Create minimal perfect hash function from in-memory vector
int main(int argc, char **argv)
{   
	// Creating a filled vector
	const char *vector[] = {"aaaaaaaaaa", "bbbbbbbbbb", "cccccccccc", "dddddddddd", "eeeeeeeeee", 
				"ffffffffff", "gggggggggg", "hhhhhhhhhh", "iiiiiiiiii", "jjjjjjjjjj"};
	unsigned int nkeys = 10;
	// Source of keys
	cmph_io_adapter_t *source = cmph_io_vector_adapter(vector, nkeys);

	//Create minimal perfect hash function using the default (chm) algorithm.
	cmph_config_t *config = cmph_config_new(source);
	cmph_t *hash = cmph_new(config);
	cmph_config_destroy(config);
   
	//Find key
	const char *key = "jjjjjjjjjj";
	unsigned int id = cmph_search(hash, key, strlen(key));
	fprintf(stderr, "Id:%u\n", id);
	//Destroy hash
	cmph_destroy(hash);
	free(source);   
	return 0;
}
