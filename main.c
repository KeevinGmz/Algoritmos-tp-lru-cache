#include "Tweener.h"
#include <stdio.h>

#define POSTEOS "posteos.dat"
#define USUARIOS "usuarios.dat"

int main()
{
	lru_cache_t cache;
	int tipoLote, usoCache;
	unsigned int tamanioCache = 1000;
	lru_cache_init(&cache, tamanioCache, cmp);
	do {
		printf("Selecciona tipo de lote: \n");
		printf("Lote Concentrado: 1\n");
		printf("Lote Disperso: 2\n");
		printf("Lote Ordenado: 3\n");
		scanf("%d", &tipoLote);
	} while (tipoLote != LOTE_ORDENADO && tipoLote != LOTE_DISPERSO &&
		 tipoLote != LOTE_CONCENTRADO);

	do {
		printf("Usar Cache? \n");
		printf("Si: 1\n");
		printf("No: 0\n");
		scanf("%d", &usoCache);
	} while (usoCache != 1 && usoCache != 0);

	generarLotePruebas(1, tipoLote);

	procesarLotes(&cache, usoCache, tipoLote, USUARIOS, POSTEOS);
	lru_clear(&cache);
	return 0;
}
