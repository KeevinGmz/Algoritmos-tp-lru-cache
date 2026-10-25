#include <lru_cache_index.h>
lru_status_t lru_cache_init(lru_cache_t* cache, unsigned int max_cache_size, lru_cmp_t cmp);
lru_status_t lru_cache_insert(lru_cache_t* cache, const void* data, unsigned int tam);
lru_status_t lru_cache_remove(lru_cache_t* cache, void* data, const unsigned tam);
lru_status_t lru_cache_get(lru_cache_t* cache, void* data);
void lru_clear(lru_cache_t* cache);
unsigned int lru_cache_height(lru_cache_t* c);