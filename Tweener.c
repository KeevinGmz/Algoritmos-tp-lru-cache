/* C89 estricto: en POSIX glibc solo declara clock_gettime() y CLOCK_MONOTONIC
 * si _POSIX_C_SOURCE >= 199309L. Tiene que ir antes de cualquier include. */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L
#endif

#include "Tweener.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CANT_POSTEOS 50000
#define CANT_USUARIOS 200
#define CANT_FEEDS 20

typedef void(*procesarLote)(void *cache, FILE *archPosteos, sMetricas *metricas,
			    sUsuarios *usuario, sPosteos *post, const int pos);

#ifdef _WIN32
#include <windows.h>
static double ahora(void)
{
	LARGE_INTEGER freq, cont;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cont);
	return (double)cont.QuadPart / (double)freq.QuadPart;
}
#else
#include <sys/stat.h>
static double ahora(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
#endif

void procesarLoteConCache(void *cache, FILE *archPosteos, sMetricas *metricas,
			 sUsuarios *usuario, sPosteos *post, const int pos)
{
	lru_status_t estaEnCache;
	estaEnCache = lru_cache_get(cache, usuario->feed + pos, sizeof(int));

	if (estaEnCache == LRU_MISS) {
		metricas->cantLecturasPosteos++;
		metricas->cantFallos++;
		fseek(archPosteos, *(usuario->feed + pos) * sizeof(sPosteos),
		      SEEK_SET);
		fread(post, sizeof(sPosteos), 1, archPosteos);
		lru_cache_insert(cache, post, sizeof(sPosteos));
	} else {
		metricas->cantAciertos++;
	}

}

void procesarLoteSinCache(void *cache, FILE *archPosteos, sMetricas *metricas,
			 sUsuarios *usuario, sPosteos *post, const int pos)
{
	metricas->cantLecturasPosteos++;
	metricas->cantFallos++;
	fseek(archPosteos, *(usuario->feed + pos) * sizeof(sPosteos), SEEK_SET);
	fread(post, sizeof(sPosteos), 1, archPosteos);

}

static int randRango(int n)
{
	unsigned r = ((unsigned)rand() << 15) | (unsigned)rand();
	return (int)(r % (unsigned)n);
}

int generarLotePruebas(unsigned int semilla, int tipoLote)
{
	FILE *posteos, *usuarios;
	sPosteos posteo;
	sUsuarios usuario;
	int i, j, k, id;
	int cierrePosteos, cierreUsuarios;

	if (tipoLote < 1 || tipoLote > 3)
		return 0;

	posteos = fopen("posteos.dat", "wb");
	if (!posteos)
		return 0;

	usuarios = fopen("usuarios.dat", "wb");
	if (!usuarios) {
		fclose(posteos);
		return 0;
	}

	srand(semilla);

	for (i = 0; i < CANT_POSTEOS; i++) {
		memset(&posteo, 0, sizeof(posteo));
		posteo.id = i;
		sprintf(posteo.texto, "Posteo %d para el lote de pruebas.", i);

		if (fwrite(&posteo, sizeof(posteo), 1, posteos) != 1) {
			fclose(posteos);
			fclose(usuarios);
			return 0;
		}
	}

	for (i = 0; i < CANT_USUARIOS; i++) {
		memset(&usuario, 0, sizeof(usuario));
		sprintf(usuario.usuario, "Usuario%03d", i + 1);

		for (j = 0; j < CANT_FEEDS; j++) {
			if (tipoLote == LOTE_ORDENADO) {
				usuario.feed[j] = i * CANT_FEEDS + j;
			} else {
				/* No repetir un posteo dentro del mismo feed. */
				do {
					if (tipoLote == LOTE_CONCENTRADO)
						id = randRango(
							100); /* concentrado: ids 0..99 */
					else
						id = randRango(
							CANT_POSTEOS); /* disperso: todo el rango */

					for (k = 0;
					     k < j && usuario.feed[k] != id;
					     k++)
						;
				} while (k < j);

				usuario.feed[j] = id;
			}
		}

		if (fwrite(&usuario, sizeof(usuario), 1, usuarios) != 1) {
			fclose(posteos);
			fclose(usuarios);
			return 0;
		}
	}

	cierrePosteos = fclose(posteos);
	cierreUsuarios = fclose(usuarios);

	return cierrePosteos == 0 && cierreUsuarios == 0;
}

int leerArchivoPosteosParte1(lru_cache_t *cache)
{
	FILE *archPosteos, *archUsuarios;
	sPosteos post;
	sUsuarios usuario;
	double inicio, fin, segundos;
	unsigned cantUsuarios = 0;
	unsigned cantLecturasPosteos = 0;
	unsigned cantAciertos = 0;
	unsigned cantFallos = 0;
	lru_status_t estaEnCache;
	int i = 0;

	archPosteos = fopen("posteos.dat", "rb");
	if (!archPosteos)
		return -1;

	archUsuarios = fopen("usuarios.dat", "rb");
	if (!archUsuarios) {
		fclose(archPosteos);
		return -1;
	}

	inicio = ahora();

	while (fread(&usuario, sizeof(sUsuarios), 1, archUsuarios) == 1) {
		cantUsuarios++;

		for (i = 0; i < 20; i++) {
			estaEnCache = lru_cache_get(cache, usuario.feed + i,
						    sizeof(int));

			if (estaEnCache == LRU_MISS) {
				cantLecturasPosteos++;
				cantFallos++;
				fseek(archPosteos,
				      usuario.feed[i] * sizeof(sPosteos),
				      SEEK_SET);
				fread(&post, sizeof(sPosteos), 1, archPosteos);
				lru_cache_insert(cache, &post,
						 sizeof(sPosteos));
			} else {
				cantAciertos++;
			}
		}
	}

	fin = ahora();
	segundos = fin - inicio;

	printf("Usuarios procesados: %u\n", cantUsuarios);
	printf("Posteos consultados: %u\n", cantLecturasPosteos);
	printf("Tiempo total: %.9f segundos\n", segundos);

	if (cantUsuarios > 0)
		printf("Promedio por usuario: %.9f segundos\n",
		       segundos / cantUsuarios);

	if (cantLecturasPosteos > 0)
		printf("Promedio por posteo: %.9f segundos\n",
		       segundos / cantLecturasPosteos);

	printf("Cantidad de aciertos: %u\n", cantAciertos);
	printf("Cantidad de fallos: %u\n", cantFallos);
	printf("Cantidad de lecturas realizasl al archivo posteos: %u\n",
	       cantLecturasPosteos);

	fclose(archPosteos);
	fclose(archUsuarios);
	return 1;
}

int procesarLotes(lru_cache_t *cache, int usoCache, int tipoLote,
		  char *nomArchUsuarios, char *nomArchPosteos)
{
	FILE *archPosteos, *archUsuarios;
	sPosteos post;
	sUsuarios usuario;
	sMetricas metricas = { 0 };
	int i = 0;
	procesarLote p;

	if (usoCache == 1) {
		p = procesarLoteConCache;
	}

	archPosteos = fopen(nomArchPosteos, "rb");
	if (!archPosteos)
		return -1;

	archUsuarios = fopen(nomArchUsuarios, "rb");
	if (!archUsuarios) {
		fclose(archPosteos);
		return -1;
	}

	metricas.inicio = ahora();

	while (fread(&usuario, sizeof(sUsuarios), 1, archUsuarios) == 1) {
		metricas.cantUsuarios++;

		for (i = 0; i < CANT_FEEDS; i++) {
			p(cache, archPosteos, &metricas, &usuario, &post, i);
		}
	}

	metricas.fin = ahora();
	metricas.segundos = metricas.fin - metricas.inicio;

	printf("Usuarios procesados: %u\n", metricas.cantUsuarios);
	printf("Posteos consultados: %u\n", metricas.cantLecturasPosteos);
	printf("Tiempo total: %.9f segundos\n", metricas.segundos);

	if (metricas.cantUsuarios > 0)
		printf("Promedio por usuario: %.9f segundos\n",
		       metricas.segundos / metricas.cantUsuarios);

	if (metricas.cantLecturasPosteos > 0)
		printf("Promedio por posteo: %.9f segundos\n",
		       metricas.segundos / metricas.cantLecturasPosteos);

	printf("Cantidad de aciertos: %u\n", metricas.cantAciertos);
	printf("Cantidad de fallos: %u\n", metricas.cantFallos);
	printf("Cantidad de lecturas realizasl al archivo posteos: %u\n",
	       metricas.cantLecturasPosteos);

#ifdef CACHE_INDICE
	if (usoCache == SIN_CACHE && tipoLote == LOTE_ORDENADO) {
		printf("Altura de arbol de cache: %d\n",
		       lru_cache_height(cache));
	}
#endif

	fclose(archPosteos);
	fclose(archUsuarios);
	return 1;
}

int cmp(const void *a, const void *b)
{
	const sPosteos *X = (const sPosteos *)a;
	const sPosteos *Y = (const sPosteos *)b;
	return (X->id > Y->id) - (X->id < Y->id);
}
