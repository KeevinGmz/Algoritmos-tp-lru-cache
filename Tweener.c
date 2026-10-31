#include "Tweener.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
///#include <lru_cache_list.h>
#define CANT_POSTEOS 50000
#define CANT_USUARIOS 200
#define CANT_FEEDS 20

#include <windows.h>
static double ahora(void)
{
    LARGE_INTEGER freq, cont;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cont);
    return (double)cont.QuadPart / (double)freq.QuadPart;
}

///PARA LINUX
/*
static double ahora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
*/

static int randRango(int n)
{
    unsigned r = ((unsigned)rand() << 15) | (unsigned)rand();  /* 30 bits */
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
    if (!usuarios)
    {
        fclose(posteos);
        return 0;
    }

    srand(semilla);

    for (i = 0; i < CANT_POSTEOS; i++)
    {
        memset(&posteo, 0, sizeof(posteo));
        posteo.id = i;
        sprintf(posteo.texto, "Posteo %d para el lote de pruebas.", i);

        if (fwrite(&posteo, sizeof(posteo), 1, posteos) != 1)
        {
            fclose(posteos);
            fclose(usuarios);
            return 0;
        }
    }

    for (i = 0; i < CANT_USUARIOS; i++)
    {
        memset(&usuario, 0, sizeof(usuario));
        sprintf(usuario.usuario, "Usuario%03d", i + 1);

        for (j = 0; j < CANT_FEEDS; j++)
        {
            if (tipoLote == 3)
                usuario.feed[j] = i * 20 + j;
            else
            {
                /* No repetir un posteo dentro del mismo feed. */
                do
                {
                    if (tipoLote == LOTE_CONCENTRADO)
                        id = randRango(100);              /* concentrado: ids 0..99 */
                    else
                        id = randRango(CANT_POSTEOS);     /* disperso: todo el rango */

                    for (k = 0; k < j && usuario.feed[k] != id; k++)
                        ;
                } while (k < j);

                usuario.feed[j] = id;
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

int leerArchivoPosteos()
{
    FILE *archPosteos, *archUsuarios;
    sPosteos post;
    sUsuarios usuario;
    double inicio, fin, segundos;
    unsigned cantUsuarios = 0;
    unsigned cantPosteos = 0;
    int i = 0;

    archPosteos = fopen("posteos.dat", "rb");
    if (!archPosteos)
        return -1;

    archUsuarios = fopen("usuarios.dat", "rb");
    if (!archUsuarios)
    {
        fclose(archPosteos);
        return -1;
    }

    inicio = ahora();

    while (fread(&usuario, sizeof(sUsuarios), 1, archUsuarios) == 1)
    {
        cantUsuarios++;

        for (i = 0; i < 20; i++)
        {
            fseek(archPosteos, *(usuario.feed + i) * sizeof(sPosteos), SEEK_SET);
            fread(&post, sizeof(sPosteos), 1, archPosteos);
            cantPosteos++;
        }
    }

    fin = ahora();
    segundos = fin - inicio;

    printf("Usuarios procesados: %u\n", cantUsuarios);
    printf("Posteos consultados: %u\n", cantPosteos);
    printf("Tiempo total: %.9f segundos\n", segundos);

    if (cantUsuarios > 0)
        printf("Promedio por usuario: %.9f segundos\n", segundos / cantUsuarios);

    if (cantPosteos > 0)
        printf("Promedio por posteo: %.9f segundos\n", segundos / cantPosteos);

    fclose(archPosteos);
    fclose(archUsuarios);
    return 1;
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
    if (!archUsuarios)
    {
        fclose(archPosteos);
        return -1;
    }

    inicio = ahora();

    while (fread(&usuario, sizeof(sUsuarios), 1, archUsuarios) == 1)
    {
        cantUsuarios++;

        for (i = 0; i < 20; i++)
        {
            estaEnCache = lru_cache_get(cache, usuario.feed + i, sizeof(int));

            if (estaEnCache == LRU_ERR_NOT_FOUND || estaEnCache == LRU_ERR_EMPTY)
            {
                cantLecturasPosteos++;
                cantFallos++;
                fseek(archPosteos, *(usuario.feed + i) * sizeof(sPosteos), SEEK_SET);
                fread(&post, sizeof(sPosteos), 1, archPosteos);
                lru_cache_insert(cache, &post, sizeof(sPosteos));
            }
            else
                cantAciertos++;
        }
    }

    fin = ahora();
    segundos = fin - inicio;

    printf("Usuarios procesados: %u\n", cantUsuarios);
    printf("Posteos consultados: %u\n", cantLecturasPosteos);
    printf("Tiempo total: %.9f segundos\n", segundos);

    if (cantUsuarios > 0)
        printf("Promedio por usuario: %.9f segundos\n", segundos / cantUsuarios);

    if (cantLecturasPosteos > 0)
        printf("Promedio por posteo: %.9f segundos\n", segundos / cantLecturasPosteos);

    printf("Cantidad de aciertos: %u\n", cantAciertos);
    printf("Cantidad de fallos: %u\n", cantFallos);
    printf("Cantidad de lecturas realizasl al archivo posteos: %u\n", cantLecturasPosteos);

    fclose(archPosteos);
    fclose(archUsuarios);
    return 1;
}
int cmp(const void *a,const void*b)
{
    const sPosteos *X= (const sPosteos*) a;
    const sPosteos  *Y=(const sPosteos*)b;
return (X->id > Y->id) - (X->id < Y->id);
}
