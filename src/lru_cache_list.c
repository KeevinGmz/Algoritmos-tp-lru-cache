#include <lru_cache_list.h>
#include <string.h>
#include <stdlib.h>

lru_status_t lru_cache_init(lru_cache_t *cache, unsigned int max_cache_size,
			    lru_cmp_t cmp)
{
	if (!cmp || max_cache_size <= 0)
		return ;

	cache->head = NULL;
	cache->max_cache_size = max_cache_size;
	cache->current_size = 0;
	cache->cmp = cmp;

	return LRU_OK;
}

lru_status_t lru_cache_insert(lru_cache_t *cache, const void *data,
			      unsigned int tam)
{
	lru_cache_node_t *act = cache->head, *prev = NULL;
	while (act != NULL && cache->cmp(act->data, data) != 0) {
		prev = act;
		act = act->next;
	}
	if (act != NULL) {
		/*!TODO: PREGUNTAR SI HAY QUE HACER REALLOC*/
		memcpy(act->data, data, tam);
		act->tam = tam;
		if (prev) {
			prev->next = act->next;
			act->next = cache->head;
			cache->head = act;
		}
	} else {
		lru_cache_node_t *new_node =
			(lru_cache_node_t *)malloc(sizeof(lru_cache_node_t));
		void *new_data = NULL;
		if (!new_node) {
			return LRU_ERR_FULL;
		}
		new_data = malloc(tam);
		if (!new_data) {
			free(new_node);
			return LRU_ERR_FULL;
		}
		memcpy(new_data, data, tam);
		if (cache->current_size + 1 > cache->max_cache_size) {
			lru_cache_node_t *elim = cache->head;
			prev = NULL;
			while (elim->next != NULL) {
				prev = elim;
				elim = elim->next;
			}
			free(elim->data);
			free(elim);
			if (prev != NULL) {
				prev->next = NULL;
			} else {
				cache->head = NULL;
			}
		} else {
			cache->current_size++;
		}
		new_node->next = cache->head;
		new_node->data = new_data;
		new_node->tam = tam;
		cache->head = new_node;
	}
	return LRU_OK;
}
lru_status_t lru_cache_remove(lru_cache_t *cache, void *data,
			      const unsigned tam)
{
	lru_cache_node_t *act = cache->head, *prev = NULL;
	if (cache->head == NULL) {
		return LRU_ERR_EMPTY;
	}
	while (act != NULL && cache->cmp(act->data, data) != 0) {
		prev = act;
		act = act->next;
	}
	if (act == NULL) {
		return LRU_ERR_NOT_FOUND;
	}
	memcpy(data, act->data, MIN(tam, act->tam));
	if (prev) {
		prev->next = act->next;
	} else {
		cache->head = act->next;
	}
	cache->current_size--;
	free(act->data);
	free(act);
	return LRU_OK;
}

lru_status_t lru_cache_get(lru_cache_t *cache, void *data, const unsigned tam)
{
	lru_cache_node_t *act = cache->head;
	lru_cache_node_t *prev = NULL;
	if (!act)
		return LRU_ERR_EMPTY;

	while (act && cache->cmp(act->data, data) != 0) {
		prev = act;
		act = act->next;
	}

	if (!act)
		return LRU_ERR_NOT_FOUND;

	memcpy(data, act->data, MIN(tam, act->tam));
	if (prev) {
		act->tam = tam;
		prev->next = act->next;
		act->next = cache->head;
		cache->head = act;
	}
	return LRU_OK;
}

void lru_clear(lru_cache_t *cache)
{
	if (cache) {
		lru_cache_node_t *act = cache->head;
		lru_cache_node_t *elim;
		while (act) {
			elim = act;
			act = elim->next;
			free(elim->data);
			free(elim);
		}
		cache->head = NULL;
		cache->current_size = 0;
	}
}
