#include <datos_simulacion.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define TAM_RUTA 1024
#define MODO_CONCENTRADO 0
#define MODO_DISPERSO 1
#define MODO_ORDENADO 2

/* Generador de 32 bits: no depende de la implementacion de rand(). */
static unsigned long siguiente(unsigned long *estado)
{
    *estado = (*estado * 1664525UL + 1013904223UL) & 0xFFFFFFFFUL;
    return *estado;
}

static int leer_numero(const char *texto, unsigned long *valor)
{
    char *fin;
    const char *p;

    if (!*texto)
        return 0;
    for (p = texto; *p; p++)
        if (*p < '0' || *p > '9')
            return 0;

    errno = 0;
    *valor = strtoul(texto, &fin, 10);
    return errno != ERANGE && *fin == '\0';
}

static FILE *abrir_salida(const char *carpeta, const char *nombre,
                         const char *modo)
{
    char ruta[TAM_RUTA];
    FILE *archivo;

    if (strlen(carpeta) + strlen(nombre) + 2 > sizeof(ruta))
    {
        fprintf(stderr, "La ruta de salida es demasiado larga.\n");
        return NULL;
    }
    sprintf(ruta, "%s/%s", carpeta, nombre);
    archivo = fopen(ruta, modo);
    if (!archivo)
        perror(ruta);
    return archivo;
}

static int generar_posteos(const char *carpeta, unsigned long semilla)
{
    FILE *archivo;
    tp_posteo_t posteo;
    unsigned long estado;
    int i;

    archivo = abrir_salida(carpeta, "posteos.dat", "wb");
    if (!archivo)
        return 0;

    estado = semilla;
    for (i = 0; i < TP_CANT_POSTEOS; i++)
    {
        /* Tambien se inicializa el relleno del struct para repetir los bytes. */
        memset(&posteo, 0, sizeof(posteo));
        posteo.id = i;
        sprintf(posteo.texto, "Posteo %05d - contenido de prueba %08lX.",
                i, siguiente(&estado));
        if (fwrite(&posteo, sizeof(posteo), 1, archivo) != 1)
        {
            fprintf(stderr, "No se pudo escribir posteos.dat.\n");
            fclose(archivo);
            return 0;
        }
    }
    return fclose(archivo) == 0;
}

static void intercambiar(int *a, int *b)
{
    int aux = *a;
    *a = *b;
    *b = aux;
}

static int generar_usuarios(const char *carpeta, const char *nombre, int modo,
                            unsigned usuarios, unsigned calientes,
                            unsigned long semilla)
{
    FILE *archivo;
    int *indices;
    tp_usuario_t usuario;
    unsigned long estado;
    unsigned i, j, elegido;
    int correcto;

    indices = malloc(TP_CANT_POSTEOS * sizeof(*indices));
    if (!indices)
    {
        fprintf(stderr, "No hay memoria para preparar los identificadores.\n");
        return 0;
    }
    for (i = 0; i < TP_CANT_POSTEOS; i++)
        indices[i] = (int)i;

    estado = semilla;
    if (modo != MODO_ORDENADO)
    {
        /* Permutacion sin repetidos de todos los identificadores. */
        for (i = TP_CANT_POSTEOS - 1; i > 0; i--)
        {
            elegido = (unsigned)(siguiente(&estado) % (i + 1UL));
            intercambiar(&indices[i], &indices[elegido]);
        }
    }

    archivo = abrir_salida(carpeta, nombre, "wb");
    if (!archivo)
    {
        free(indices);
        return 0;
    }

    correcto = 1;
    for (i = 0; i < usuarios; i++)
    {
        memset(&usuario, 0, sizeof(usuario));
        sprintf(usuario.usuario, "usuario%06u", i);
        for (j = 0; j < TP_TAM_FEED; j++)
        {
            if (modo == MODO_CONCENTRADO)
            {
                /* Cada feed elige 20 distintos del mismo conjunto frecuente. */
                elegido = j + (unsigned)(siguiente(&estado) % (calientes - j));
                intercambiar(&indices[j], &indices[elegido]);
                usuario.feed[j] = indices[j];
            }
            else
                usuario.feed[j] = indices[i * TP_TAM_FEED + j];
        }

        if (fwrite(&usuario, sizeof(usuario), 1, archivo) != 1)
        {
            fprintf(stderr, "No se pudo escribir %s.\n", nombre);
            correcto = 0;
            break;
        }
    }
    free(indices);
    if (fclose(archivo) != 0)
        correcto = 0;
    return correcto;
}

static int escribir_parametros(const char *carpeta, unsigned long semilla,
                               unsigned usuarios, unsigned calientes)
{
    FILE *archivo;
    int resultado, cerrado;

    archivo = abrir_salida(carpeta, "parametros.txt", "wb");
    if (!archivo)
        return 0;
    resultado = fprintf(archivo,
        "Generador de lotes del TP LRU Cache\n"
        "Semilla: %lu\n"
        "Posteos: %d\n"
        "Usuarios por lote: %u\n"
        "Posteos por usuario: %d\n"
        "Conjunto frecuente: %u\n"
        "Identificadores de posteos: 0 a %d\n"
        "Tamano de tp_posteo_t: %lu bytes\n"
        "Tamano de tp_usuario_t: %lu bytes\n",
        semilla, TP_CANT_POSTEOS, usuarios, TP_TAM_FEED, calientes,
        TP_CANT_POSTEOS - 1, (unsigned long)sizeof(tp_posteo_t),
        (unsigned long)sizeof(tp_usuario_t));
    if (resultado >= 0)
        resultado = fprintf(archivo,
        "Los archivos no tienen encabezado: contienen structs consecutivos.\n"
        "Acceso directo: fseek(archivo, (long)id * sizeof(tp_posteo_t), SEEK_SET).\n"
        "Concentrado: 20 distintos por usuario dentro del conjunto frecuente.\n"
        "Disperso: todos los accesos del lote usan identificadores diferentes.\n"
        "Ordenado: los identificadores aumentan en toda la secuencia de lectura.\n"
        "Capacidades sugeridas para comparar: 10, 50, 100, 500, 1000.\n");
    cerrado = fclose(archivo);
    return resultado >= 0 && cerrado == 0;
}

int main(int argc, char **argv)
{
    unsigned long semilla = 2026UL;
    unsigned long usuarios = 1000UL;
    unsigned long calientes = 100UL;
    unsigned int orden_bytes = 1;
    const char *carpeta = "Lotes";
    int creado;

    if (argc > 5 || (argc > 1 && !leer_numero(argv[1], &semilla)) ||
        (argc > 2 && !leer_numero(argv[2], &usuarios)) ||
        (argc > 4 && !leer_numero(argv[4], &calientes)))
    {
        fprintf(stderr,
            "Uso: generar_lotes [semilla] [usuarios] [carpeta] [frecuentes]\n");
        return EXIT_FAILURE;
    }
    if (argc > 3)
        carpeta = argv[3];
    if (CHAR_BIT != 8 || sizeof(int) != 4 ||
        *((unsigned char *)&orden_bytes) != 1 ||
        sizeof(tp_posteo_t) != 148 || sizeof(tp_usuario_t) != 96)
    {
        fprintf(stderr, "Se necesita int de 32 bits, little endian y alineacion normal.\n");
        return EXIT_FAILURE;
    }
    if (semilla > 0xFFFFFFFFUL || usuarios < 200 ||
        usuarios > TP_CANT_POSTEOS / TP_TAM_FEED ||
        calientes < TP_TAM_FEED || calientes > TP_CANT_POSTEOS)
    {
        fprintf(stderr,
            "Semilla: 0..4294967295. Usuarios: 200..2500. Frecuentes: 20..50000.\n");
        return EXIT_FAILURE;
    }

#ifdef _WIN32
    creado = _mkdir(carpeta);
#else
    creado = mkdir(carpeta, 0777);
#endif
    if (creado != 0 && errno != EEXIST)
    {
        perror(carpeta);
        return EXIT_FAILURE;
    }

    if (!generar_posteos(carpeta, semilla) ||
        !generar_usuarios(carpeta, "usuarios_concentrado.dat", MODO_CONCENTRADO,
                          (unsigned)usuarios, (unsigned)calientes, semilla) ||
        !generar_usuarios(carpeta, "usuarios_disperso.dat", MODO_DISPERSO,
                          (unsigned)usuarios, (unsigned)calientes, semilla) ||
        !generar_usuarios(carpeta, "usuarios_ordenado.dat", MODO_ORDENADO,
                          (unsigned)usuarios, (unsigned)calientes, semilla) ||
        !escribir_parametros(carpeta, semilla, (unsigned)usuarios,
                             (unsigned)calientes))
        return EXIT_FAILURE;

    printf("Lotes generados en: %s\n", carpeta);
    printf("Semilla: %lu. Posteos: %d. Usuarios por lote: %lu.\n",
           semilla, TP_CANT_POSTEOS, usuarios);
    return EXIT_SUCCESS;
}
