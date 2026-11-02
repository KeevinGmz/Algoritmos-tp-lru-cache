#include <lru_cache_list.h>
#include <string.h>
#include <stdlib.h>

/* Deja la cache vacia y guarda su capacidad y su comparador.
 * La capacidad cuenta elementos; cmp indica como comparar sus claves.
 */
lru_status_t lru_cache_init(lru_cache_t *cache, unsigned int max_cache_size,
			    lru_cmp_t cmp)
{
	if (!cmp || max_cache_size <= 0)
		return LRU_ERR_NO_CREADO;

	cache->head = NULL;
	cache->max_cache_size = max_cache_size;
	cache->current_size = 0;
	cache->cmp = cmp;

	return LRU_OK;
}

/* Guarda una copia del dato y lo deja al principio de la lista.
 * Si la clave ya estaba, reemplaza su dato. Si la cache esta llena,
 * saca el ultimo elemento, que es el menos usado recientemente.
 */
lru_status_t lru_cache_insert(lru_cache_t *cache, const void *data,
			      unsigned int tam)
{
	lru_cache_node_t *act = cache->head, *prev = NULL;
	while (act != NULL && cache->cmp(act->data, data) != 0) {
		prev = act;
		act = act->next;
	}
	/* La clave ya existe: se actualiza sin crear otro elemento. */
	if (act != NULL) {

		 if (tam > act->tam)
        {
            void *nuevo = malloc(tam);

            if (!nuevo)
                return LRU_ERR_MEM;

            memcpy(nuevo, data, tam);
            free(act->data);
            act->data = nuevo;
        }

        else
        {
            memcpy(act->data, data, tam);
        }

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
		/* Si esta llena, se busca y se quita el ultimo nodo. */
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
		/* El nuevo nodo queda primero; los demas conservan su orden. */
		new_node->next = cache->head;
		new_node->data = new_data;
		new_node->tam = tam;
		cache->head = new_node;
	}
	return LRU_OK;
}
/* Busca la clave, desconecta el nodo y libera su dato y su memoria.
 * Antes de borrarlo, copia en data los bytes que entran en tam.
 */
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

/* Usa la clave que viene en data para buscar el elemento.
 * Si lo encuentra, devuelve hasta tam bytes en data y lo mueve al frente.
 * No cambia el dato guardado ni su tamano; devuelve HIT o MISS.
 */
lru_status_t lru_cache_get(lru_cache_t *cache, void *data, const unsigned tam)
{
	lru_cache_node_t *act = cache->head;
	lru_cache_node_t *prev = NULL;
	if (!act)
		return LRU_MISS;

	while (act && cache->cmp(act->data, data) != 0) {
		prev = act;
		act = act->next;
	}

	if (!act)
		return LRU_MISS;

	memcpy(data, act->data, MIN(tam, act->tam));
	if (prev) {
		prev->next = act->next;
		act->next = cache->head;
		cache->head = act;
	}
	return LRU_HIT;
}

/* Recorre la lista y libera cada dato y cada nodo.
 * Deja la cache vacia para poder volver a usarla.
 */
int lru_clear(lru_cache_t *cache)
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
	return LRU_OK;
}
