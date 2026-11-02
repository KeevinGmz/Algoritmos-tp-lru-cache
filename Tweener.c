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
/* Lee el reloj en segundos. Restar dos lecturas permite medir
 * cuanto tarda el bloque de procesamiento.
 */
static double ahora(void)
{
	LARGE_INTEGER freq, cont;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cont);
	return (double)cont.QuadPart / (double)freq.QuadPart;
}
#else
#include <sys/stat.h>
/* Lee el reloj en segundos. Restar dos lecturas permite medir
 * cuanto tarda el bloque de procesamiento.
 */
static double ahora(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}
#endif

/* Resuelve un pedido del feed: primero busca el posteo en la cache.
 * Si no esta, lo lee del archivo y lo agrega. Suma un acierto o un fallo,
 * y cuenta una lectura de archivo cuando tiene que usar fread.
 */
void procesarLoteConCache(void *cache, FILE *archPosteos, sMetricas *metricas,
			 sUsuarios *usuario, sPosteos *post, const int pos)
{
	lru_status_t estaEnCache;
	/* post entrega la clave y tambien recibe el registro completo. */
	post->id = *(usuario->feed + pos);
	estaEnCache = lru_cache_get(cache, post, sizeof(*post));

	/*el posteo no estaba en la cache */
	if (estaEnCache == LRU_MISS)
    {
		metricas->cantLecturasPosteos++;
		metricas->cantFallos++;

		fseek(archPosteos, *(usuario->feed + pos) * sizeof(sPosteos),
		      SEEK_SET);
		fread(post, sizeof(sPosteos), 1, archPosteos);


		lru_cache_insert(cache, post, sizeof(sPosteos));
	}

	else if(estaEnCache == LRU_HIT)
    {
		metricas->cantAciertos++;
	}

}

/* Resuelve un pedido leyendo siempre el archivo de posteos.
 * El ID indica que registro leer y se multiplica por sizeof(sPosteos)
 * para obtener su posicion en bytes.
 */
void procesarLoteSinCache(void *cache, FILE *archPosteos, sMetricas *metricas,
			 sUsuarios *usuario, sPosteos *post, const int pos)
{
    (void)cache;

    /* Un posteo mas leido del archivo */
	metricas->cantLecturasPosteos++;

	/* Un posteo mas pedido y no resuelto por cache*/
	metricas->cantFallos++;

	fseek(archPosteos, *(usuario->feed + pos) * sizeof(sPosteos), SEEK_SET);
	fread(post, sizeof(sPosteos), 1, archPosteos);

}

/* Genera un numero pseudoaleatorio entre 0 y n - 1.
 */
static int randRango(int n)
{
	unsigned r = ((unsigned)rand() << 15) | (unsigned)rand();
	return (int)(r % (unsigned)n);
}

/* Genera posteos.dat y UN solo usuarios.dat, segun el tipo de lote que se pida*/
/* Crea los dos archivos binarios para la simulacion.
 * El tipo de lote decide como se eligen los IDs del feed;
 * la semilla permite repetir la misma prueba.
 */
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

	for (i = 0; i < CANT_POSTEOS; i++)
    {
        /* Limpio el struct para que el relleno quede en cero */
		memset(&posteo, 0, sizeof(posteo));

		posteo.id = i;
		sprintf(posteo.texto, "Posteo %d para el lote de pruebas.", i);

		if (fwrite(&posteo, sizeof(posteo), 1, posteos) != 1) {
			fclose(posteos);
			fclose(usuarios);
			return 0;
		}
	}

	for (i = 0; i < CANT_USUARIOS; i++)
    {
        /* Limpio el struct para que el relleno quede en cero */
		memset(&usuario, 0, sizeof(usuario));

		sprintf(usuario.usuario, "Usuario%03d", i + 1);

		for (j = 0; j < CANT_FEEDS; j++)
        {
			if (tipoLote == LOTE_ORDENADO)
			{
				*(usuario.feed+j) = i * CANT_FEEDS + j;
			}

			else
            {
				/* No repetir un posteo dentro del mismo feed. */
				do {
					if (tipoLote == LOTE_CONCENTRADO)
						id = randRango(100); /* concentrado: ids 0..99 */

					else
						id = randRango(CANT_POSTEOS); /* disperso: todo el rango */

					for (k = 0;
					     k < j && *(usuario.feed+k) != id;
					     k++)
						;

				} while (k < j);

				*(usuario.feed+j) = id;
			}
		}

		if (fwrite(&usuario, sizeof(usuario), 1, usuarios) != 1)
        {
			fclose(posteos);
			fclose(usuarios);
			return 0;
		}
	}

	cierrePosteos = fclose(posteos);
	cierreUsuarios = fclose(usuarios);

	return cierrePosteos == 0 && cierreUsuarios == 0;
}

/* Version anterior del procesamiento con cache, conservada como referencia.
 * El main actual llama a procesarLotes.
 */
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
			post.id = *(usuario.feed + i);
			estaEnCache = lru_cache_get(cache, &post, sizeof(post));

			if (estaEnCache == LRU_MISS) {
				cantLecturasPosteos++;
				cantFallos++;
				fseek(archPosteos,
				      *(usuario.feed+i)* sizeof(sPosteos),
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
	printf("Posteos consultados: %u\n", cantAciertos + cantFallos);
	printf("Tiempo total: %.9f segundos\n", segundos);

	if (cantUsuarios > 0)
		printf("Promedio por usuario: %.9f segundos\n",
		       segundos / cantUsuarios);

	if (cantAciertos + cantFallos > 0)
		printf("Promedio por posteo: %.9f segundos\n",
		       segundos / (cantAciertos + cantFallos));

	printf("Cantidad de aciertos: %u\n", cantAciertos);
	printf("Cantidad de fallos: %u\n", cantFallos);
	printf("Cantidad de lecturas realizasl al archivo posteos: %u\n",
	       cantLecturasPosteos);

	fclose(archPosteos);
	fclose(archUsuarios);
	return 1;
}

/* Abre los archivos y elige el procedimiento con cache o sin cache.
 * Lee los usuarios en orden y procesa los IDs de cada feed.
 * Mide ese recorrido y muestra cantidades, tiempo total y promedios.
 */
int procesarLotes(lru_cache_t *cache, int usoCache, int tipoLote,
		  char *nomArchUsuarios, char *nomArchPosteos)
{
	FILE *archPosteos, *archUsuarios;
	sPosteos post;
	sUsuarios usuario;
	sMetricas metricas = { 0 };
	int i = 0;
	procesarLote p;
	(void)tipoLote;

	if (usoCache == 1) {
		p = procesarLoteConCache;
	}

	else
        p = procesarLoteSinCache;

	archPosteos = fopen(nomArchPosteos, "rb");
	if (!archPosteos)
		return -1;

	archUsuarios = fopen(nomArchUsuarios, "rb");
	if (!archUsuarios) {
		fclose(archPosteos);
		return -1;
	}

    /* ARRANCA EL CRONOMETRO. Se toma justo antes del ciclo, asi no se mide
    el tiempo de abrir archivos */
	metricas.inicio = ahora();

	while (fread(&usuario, sizeof(sUsuarios), 1, archUsuarios) == 1)
    {
		metricas.cantUsuarios++;

		for (i = 0; i < CANT_FEEDS; i++)
        {
            /* Este usuario tiene 20 ids en su feed (*(feed + 0) a *(feed + 19)).
            Para cada uno llamo a p, que busca ese posteo y suma las metricas.
            "i" le dice cual de los 20 ids tiene que usar */
			p(cache, archPosteos, &metricas, &usuario, &post, i);
		}
	}

	/* PARA EL CRONOMETRO y calculo el tiempo total */
	metricas.fin = ahora();
	metricas.segundos = metricas.fin - metricas.inicio;

	printf("Usuarios procesados: %u\n", metricas.cantUsuarios);
	printf("Posteos consultados: %u\n", metricas.cantAciertos + metricas.cantFallos);
	printf("Tiempo total: %.9f segundos\n", metricas.segundos);

	/* Promedio por usuario: tiempo total / cantidad de usuarios.
	   El if evita dividir por cero */
	if (metricas.cantUsuarios > 0)
		printf("Promedio por usuario: %.9f segundos\n",
		       metricas.segundos / metricas.cantUsuarios);

    /* Promedio por posteo: tiempo total / posteos pedidos.
    Posteos pedidos = aciertos + fallos (usuarios x 20) */
	if (metricas.cantAciertos + metricas.cantFallos > 0)
		printf("Promedio por posteo: %.9f segundos\n",
            metricas.segundos / (metricas.cantAciertos+metricas.cantFallos) );

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

    /* CERRAR LOS ARCHIVOS y devolver 1: todo salio bien */
    fclose(archPosteos);
	fclose(archUsuarios);

	return 1;
}

/* Comparador propio de Tweener: compara el ID de dos posteos.
 * Devuelve negativo, cero o positivo segun el primero sea menor, igual o mayor.
 * La cache recibe este comparador para no depender de sPosteos.
 */
int cmp(const void *a, const void *b)
{
	const sPosteos *X = (const sPosteos *)a;
	const sPosteos *Y = (const sPosteos *)b;

	return (X->id > Y->id) - (X->id < Y->id);
}
