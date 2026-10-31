#include <lru_cache_index.h>
#include <string.h>
#include <stdlib.h>

static bst_node_t **buscar_nodo_arbol(bst_node_t **p, const void *data,
				      lru_cmp_t cmp)
{
	int resultado;

	while (*p) {
		resultado = cmp(data, (*p)->node->data);

		if (resultado == 0)
			return p;

		if (resultado < 0)
			p = &(*p)->left;
		else
			p = &(*p)->right;
	}

	return NULL;
}

static int alturaArbolBin(bst_node_t **p)
{
	unsigned hl, hr;

	if (!*p)
		return 0;

	hl = alturaArbolBin(&(*p)->left);
	hr = alturaArbolBin(&(*p)->right);

	return (hl > hr ? hl : hr) + 1;
}

static bst_node_t **mayorNodoArbolBinBusq(bst_node_t **p)
{
	if (!*p)
		return NULL;

	while ((*p)->right)
		p = &(*p)->right;

	return p;
}

static bst_node_t **menorNodoArbolBinBusq(bst_node_t **p)
{
	if (!*p)
		return NULL;

	while ((*p)->left)
		p = &(*p)->left;

	return p;
}

static int eliminarRaizArbolBinBusq(bst_node_t **p)
{
	bst_node_t **remp;
	bst_node_t *elim;

	if (!*p)
		return 0;

	/*Si es hoja,se elimina directamente*/
	if (!(*p)->left && !(*p)->right) {
		free(*p);
		*p = NULL;
		return 1;
	}

	/* Elegir el reemplazante segun altura*/
	remp = alturaArbolBin(&(*p)->left) > alturaArbolBin(&(*p)->right) ?
		       mayorNodoArbolBinBusq(&(*p)->left) :
		       menorNodoArbolBinBusq(&(*p)->right);

	elim = *remp;

	(*p)->node = elim->node;

	/* Reconectar el posible hijo del reemplazante*/
	*remp = elim->left ? elim->left : elim->right;

	free(elim);

	return 1;
}

static int eliminarElemArbolBinBusq(bst_node_t **p, const void *data,
				    lru_cmp_t cmp)
{
	p = buscar_nodo_arbol(p, data, cmp);

	if (!p)
		return 0;

	return eliminarRaizArbolBinBusq(p);
}
static int insertarArbolBinBusq(bst_node_t **p,
				lru_cache_index_node_t *nodo_lista,
				lru_cmp_t cmp)
{
	bst_node_t *nuevo;
	int resultado;

	/* busco donde se insertara el nodo */
	while (*p) {
		resultado = cmp(nodo_lista->data, (*p)->node->data);

		if (resultado < 0)
			p = &(*p)->left;
		else if (resultado > 0)
			p = &(*p)->right;
		else
			return 0; /* clave duplicada */
	}

	nuevo = malloc(sizeof(bst_node_t));

	if (!nuevo)
		return 0;

	nuevo->node = nodo_lista;
	nuevo->left = NULL;
	nuevo->right = NULL;

	*p = nuevo;

	return 1;
}

lru_status_t lru_cache_init(lru_cache_t *cache, unsigned int max_cache_size,
			    lru_cmp_t cmp)
{
	if (!cmp || max_cache_size <= 0)
		return LRU_ERR_PARAM;

	cache->head = NULL;
	cache->last = NULL;
	cache->root = NULL;
	cache->max_cache_size = max_cache_size;
	cache->current_size = 0;
	cache->cmp = cmp;

	return LRU_OK;
}
lru_status_t lru_cache_insert(lru_cache_t *cache, const void *data,
			      unsigned int tam)
{
	bst_node_t **found;
	lru_cache_index_node_t *node, *aux;

	found = buscar_nodo_arbol(&cache->root, data, cache->cmp);
	if (found) {
		node = (*found)->node;
		if (tam >
		    node->tam) /* si el tamaño del nuevo dato es superior a la actual del nodo, se hace un realloc con la memoria necesaria. */
		{
			void *aux = realloc(node->data, tam);

			if (!aux)
				return LRU_ERR_MEM;

			node->data = aux;
		}

		memcpy(node->data, data, tam);
		node->tam = tam;
		if (node != cache->head) {
			node->prev->next = node->next;

			if (node->next) /* si tiene siguiente, significa q no es el ultimo de la cache */
				node->next->prev = node->prev;
			else /* es el ultimo elemento de la cache. */
				cache->last = node->prev;

			node->next = cache->head; /* siempre pasa al frente */
			node->prev =
				NULL; /* siempre su anterior va a ser null */
			cache->head->prev =
				node; /* el anterior de la cabeza vieja va a ser el nuevo primero. */
			cache->head =
				node; /* se actualiza la cabeza del nodo */
		}

		return LRU_OK;
	}

	node = malloc(sizeof(lru_cache_index_node_t));
	if (!node || (node->data = malloc(tam)) == NULL) {
		free(node);
		return LRU_ERR_MEM;
	}
	memcpy(node->data, data, tam);
	node->tam = tam;
	if (!insertarArbolBinBusq(&cache->root, node, cache->cmp)) {
		free(node->data);
		free(node);
		return LRU_ERR_MEM;
	}

	if (cache->current_size == cache->max_cache_size) {
		aux = cache->last;
		eliminarElemArbolBinBusq(&cache->root, aux->data, cache->cmp);

		cache->last = aux->prev;
		if (aux->prev != NULL) /* si tiene anterior */
			aux->prev->next = NULL;

		else /* si no tiene anterior,vacio la cache momentaneamente (caso de 1 elemento maximo) */
			cache->head = NULL;

		free(aux->data);
		free(aux);
	}

	node->prev = NULL;
	node->next = cache->head;
	if (cache->head)
		cache->head->prev = node;
	else
		cache->last = node;
	cache->head = node;

	/* insercion */

	if (cache->current_size < cache->max_cache_size)
		cache->current_size++;
	return LRU_OK;
}
lru_status_t lru_cache_get(lru_cache_t *cache, void *data, const unsigned tam)
{
	bst_node_t **found;
	lru_cache_index_node_t *node;
	if (cache->current_size == 0)
		return LRU_MISS;
	found = buscar_nodo_arbol(&cache->root, data, cache->cmp);

	if (found) {
		node = (*found)->node;
		memcpy(data, node->data, MIN(node->tam, tam));

		if (node != cache->head) {
			if (node->next)
				node->next->prev = node->prev;
			else
				cache->last = node->prev;

			node->prev->next = node->next;

			node->next = cache->head;
			node->prev = NULL;
			cache->head->prev = node;
			cache->head = node;
		}

		return LRU_HIT;
	} else
		return LRU_MISS;
}
lru_status_t lru_cache_remove(lru_cache_t *cache, void *data,
			      const unsigned tam)
{
	bst_node_t **found;
	lru_cache_index_node_t *aux;
	if (cache->current_size == 0)
		return LRU_ERR_EMPTY;
	found = buscar_nodo_arbol(&cache->root, data, cache->cmp);

	if (!found) {
		return LRU_ERR_NOT_FOUND;
	}

	aux = (*found)->node;
	memcpy(data, (*found)->node->data, MIN(tam, (*found)->node->tam));
	eliminarRaizArbolBinBusq(found);

	if (aux->prev != NULL) /* si tiene anterior */
		aux->prev->next = aux->next;

	else
		cache->head = aux->next;
	if (aux->next)
		aux->next->prev = aux->prev;
	else
		cache->last = aux->prev;

	free(aux->data);
	free(aux);
	cache->current_size--;
	return LRU_OK;
}

static void bst_destruir(bst_node_t *root)
{
	if (!root)
		return;
	bst_destruir(root->left);
	bst_destruir(root->right);
	free(root);
}
int lru_clear(lru_cache_t *cache)
{
	int cant = 0;
	lru_cache_index_node_t *act;

	if (!cache)
		return 0;

	act = cache->head;

	while (act != NULL) {
		lru_cache_index_node_t *aux = act->next;

		free(act->data);
		free(act);
		act = aux;
		cant++;
	}

	bst_destruir(cache->root);

	cache->root = NULL;
	cache->head = NULL;
	cache->last = NULL;
	cache->current_size = 0;

	return cant;
}
unsigned int lru_cache_height(lru_cache_t *c)
{
	return alturaArbolBin(&c->root);
}
