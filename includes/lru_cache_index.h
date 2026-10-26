#ifndef LRU_CACHE_INDEX_H
#define LRU_CACHE_INDEX_H
#include <lru_types.h>
typedef struct lru_cache_node_s {
	struct lru_cache_node_s* next, * prev;
	void* data;
	unsigned  tam;
}lru_cache_node_t;
typedef struct bst_node_s {
	 lru_cache_node_t* node;
	struct bst_node_s* left, * right;
}bst_node_t;
typedef struct lru_cache_t {
	lru_cache_node_t* head;
	lru_cache_node_t* last;
	bst_node_t *root;
	unsigned int max_cache_size;
	unsigned int current_size;
	lru_cmp_t cmp;
} lru_cache_t;
lru_status_t lru_cache_init(lru_cache_t* cache, unsigned int max_cache_size, lru_cmp_t cmp);
lru_status_t lru_cache_insert(lru_cache_t* cache, const void* data, unsigned int tam);
lru_status_t lru_cache_remove(lru_cache_t* cache, void* data);
lru_status_t lru_cache_get(lru_cache_t* cache, void* data,const unsigned tam);
int lru_clear(lru_cache_t* cache);
unsigned int lru_cache_height(lru_cache_t* c);

#endif
