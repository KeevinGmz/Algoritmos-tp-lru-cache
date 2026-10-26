#ifndef LRU_CACHE_LIST_H
#define LRU_CACHE_LIST_H
#include <lru_types.h>
typedef struct lru_cache_node_s {
	struct lru_cache_node_s* next;
	void* data;
	unsigned tam;
} lru_cache_node_t;
typedef struct lru_cache_t {
	lru_cache_node_t* head;
	unsigned int max_cache_size;
	unsigned int current_size;
	lru_cmp_t cmp;
} lru_cache_t;
lru_status_t lru_cache_init(lru_cache_t* cache, unsigned int max_cache_size,
														lru_cmp_t cmp);
lru_status_t lru_cache_insert(lru_cache_t* cache, const void* data,
															unsigned int tam);
lru_status_t lru_cache_remove(lru_cache_t* cache, void* data,
															const unsigned tam);
lru_status_t lru_cache_get(lru_cache_t* cache, void* data, const unsigned tam);
void lru_clear(lru_cache_t* cache);

#endif

