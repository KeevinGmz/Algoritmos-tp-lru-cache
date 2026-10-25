
#ifndef LRU_TYPES_H
#define LRU_TYPES_H

#define MIN(x,y)( (x) > (y)? (y) : (x) )
typedef enum {
	LRU_OK = 0,
	LRU_ERR_FULL = -1,    /* capacidad maxima invalida (cero) */
	LRU_ERR_MEM = -2,      /* no se pudo reservar memoria */
	LRU_ERR_NOT_FOUND = -3,/* la clave no existe en la cache */
	LRU_ERR_PARAM = -4,    /* puntero nulo o tamano nulo */
	LRU_ERR_TAM = -5,      /* el buffer no puede contener el dato */
	LRU_ERR_INVAL = -6,     /* primitiva que la version no tiene */
	LRU_ERR_EMPTY = -7     /*lista vacia*/
} lru_status_t;


typedef int (*lru_cmp_t)(const void* claveA, const void* claveB);

#endif
