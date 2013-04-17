#include <cmph.h>

int test(cmph_uint32* items_to_hash, cmph_uint32 items_len, CMPH_ALGO alg_n)
{
    cmph_t *hash;
    cmph_config_t *config;
    cmph_io_adapter_t *source;
    cmph_uint32 i;

    printf("%s (%u)\n", cmph_names[alg_n], alg_n);

    source = cmph_io_struct_vector_adapter(items_to_hash,
                                           (cmph_uint32)sizeof(cmph_uint32),
                                           0,
                                           (cmph_uint32)sizeof(cmph_uint32),
                                           items_len);
    config = cmph_config_new(source);
    cmph_config_set_algo(config, alg_n);
    hash = cmph_new(config);
    cmph_config_destroy(config);

    printf("packed_size %u\n",cmph_packed_size(hash));

    for (i=0; i<items_len; ++i)
        printf("%d -> %u\n",
               items_to_hash[i],
               cmph_search(hash,
                           (char*)(items_to_hash+i), 
                           (cmph_uint32)sizeof(cmph_uint32)));
    printf("\n");

    cmph_io_vector_adapter_destroy(source);   
    cmph_destroy(hash);	
    return 0;
}

int main (void)
{
    cmph_uint32 vec1[] = {1,2,3,4,5};
    cmph_uint32 vec1_len = 5;

    cmph_uint32 vec2[] = {7576423, 7554496}; //CMPH_FCH, CMPH_BDZ, CMPH_BDZ_PH (4,5,6)
    cmph_uint32 vec2_len = 2;
    cmph_uint32 vec3[] = {2184764, 1882984, 1170551}; // CMPH_CHD_PH, CMPH_CHD (7,8)
    cmph_uint32 vec3_len = 3;
    cmph_uint32 i;

    // Testing with vec1
    cmph_uint32* values = (cmph_uint32*)vec1;
    cmph_uint32 length = vec1_len;
    printf("TESTING VECTOR WITH %u INTEGERS\n", length);
    for (i = 0; i < CMPH_COUNT; i++)
    {
        test(values, length, i);
    }

    // Testing with vec2
    values = (cmph_uint32*)vec2;
    length = vec2_len;
    printf("TESTING VECTOR WITH %u INTEGERS\n", length);
    for (i = 0; i < CMPH_COUNT; i++)
    {
        test(values, length, i);
    }

    // Testing with vec2
    values = (cmph_uint32*)vec3;
    length = vec3_len;
    printf("TESTING VECTOR WITH %u INTEGERS\n", length);
    for (i = 0; i < CMPH_COUNT; i++)
    {
        test(values, length, i);
    }

    return 0;
}
