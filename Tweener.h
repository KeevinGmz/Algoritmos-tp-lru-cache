#ifndef TWEENER_H_INCLUDED
#define TWEENER_H_INCLUDED
#include <lru_cache_list.h>
typedef struct
{
    int id;
    char texto[141];
}sPosteos;
typedef struct
{
    char usuario[15];
    int feed[20];
}sUsuarios;
typedef int (*cmpTweener) (const void * , const void*);
int cmp(const void*,const void*);
/* tipoLote: 1 concentrado, 2 disperso, 3 ordenado. Devuelve 1 si pudo crear ambos archivos. */
int generarLotePruebas(unsigned int semilla, int tipoLote);
int leerArchivoPosteos();
int leerArchivoPosteosParte1(lru_cache_t *cache);
#endif /* TWEENER_H_INCLUDED */
