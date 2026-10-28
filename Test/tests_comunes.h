/*
 * @file tests_comunes.h
 * @brief Contrato compartido por las dos implementaciones del TDA LRU Cache.
 *
 * Este archivo se incluye desde un unico .c por suite: define funciones de
 * test, no solo declaraciones.
 *
 * Estructuras del TDA (includes/lru_types.h, lru_cache_list.h,
 * lru_cache_index.h):
 *   - el estado se devuelve con lru_status_t (LRU_OK y los LRU_ERR_*);
 *   - una cache se describe con lru_cache_t y guarda bloques opacos de bytes;
 *   - la identidad de un bloque la define lru_cmp_t, que compara dos bloques.
 *
 * El TDA no separa clave de valor: cada test guarda un registro cuyo PRIMER
 * CAMPO es la clave. El comparador lee solo ese prefijo y el resto del bloque
 * viaja como dato, asi las pruebas de clave/valor siguen siendo validas para
 * una cache generica.
 *
 * La parte 2 declara lru_cache_get SIN el tamanio del buffer de destino, por
 * lo que no comparte firma con la parte 1. Cada suite define CACHE_GET antes
 * de incluir este archivo y las pruebas comunes llaman a la primitiva a
 * traves del adaptador suite_obtener. Los casos que dependen del tamanio del
 * buffer de destino viven en test_lru_cache_parte1.c.
 *
 * Orden de inclusion en cada suite:
 *   #include <lru_cache_list.h>          (o <lru_cache_index.h>)
 *   #define CACHE_GET(cache, dato, tam) ...
 *   #include "tests_comunes.h"
 *
 * El TDA tampoco expone accesores de capacidad ni de cantidad: los
 * contadores son campos publicos de lru_cache_t y los tests los leen
 * directamente.
 */

#ifndef TESTS_COMUNES_H
#define TESTS_COMUNES_H

/* Cada suite aporta su propio header: lru_cache_list.h y lru_cache_index.h
 * declaran los mismos tipos y funciones con firmas distintas, asi que no
 * pueden convivir en la misma unidad de traduccion. */
#include <lru_types.h>
#include <test_framework.h>

#include <string.h>

#ifndef CACHE_GET
#error "La suite debe definir CACHE_GET antes de incluir tests_comunes.h"
#endif

/* ==========================================================================
 * Utilidades de comparacion y de datos
 * ========================================================================== */

/* La clave es el primer campo del bloque: el comparador la lee sin depender
 * del resto del dato. */
typedef struct {
  int clave;
  int valor;
} registro_t;

static void registro_init(registro_t *registro, int clave, int valor) {
  registro->clave = clave;
  registro->valor = valor;
}

static int cmp_int(const void *claveA, const void *claveB) {
  return *(const int *)claveA - *(const int *)claveB;
}

static int cmp_texto(const void *claveA, const void *claveB) {
  return strcmp((const char *)claveA, (const char *)claveB);
}

/* Dato generico de tamano fijo, con relleno determinista para poder
 * compararlo con memcmp. El legajo es la clave y va primero. */
typedef struct {
  int legajo;
  char nombre[32];
  double promedio;
} alumno_t;

static void alumno_init(alumno_t *alumno, int legajo, const char *nombre,
                        double promedio) {
  memset(alumno, 0, sizeof(*alumno));
  alumno->legajo = legajo;
  strncpy(alumno->nombre, nombre, sizeof(alumno->nombre) - 1);
  alumno->promedio = promedio;
}

/* Bloque con clave de texto en el primer campo. */
typedef struct {
  char clave[16];
  long valor;
} texto_registro_t;

/* ==========================================================================
 * Adaptador de lectura y atajos sobre la interfaz publica
 * ========================================================================== */

/* La parte 1 recibe el tamanio del buffer de destino; la parte 2 no. */
static lru_status_t suite_obtener(lru_cache_t *cache, void *dato,
                                  unsigned tam) {
  (void)tam;
  return CACHE_GET(cache, dato, tam);
}

/* Un lru_cache_t no tiene accesores: los contadores son campos publicos. */
static unsigned cache_cantidad(const lru_cache_t *cache) {
  return cache->current_size;
}

static unsigned cache_capacidad(const lru_cache_t *cache) {
  return cache->max_cache_size;
}

static lru_status_t cache_agregar(lru_cache_t *cache, int clave, int valor) {
  registro_t registro;

  registro_init(&registro, clave, valor);
  return lru_cache_insert(cache, &registro, sizeof(registro));
}

static lru_status_t cache_obtener(lru_cache_t *cache, int clave, int *valor) {
  registro_t registro;
  lru_status_t estado;

  registro_init(&registro, clave, 0);
  estado = suite_obtener(cache, &registro, sizeof(registro));
  if (estado == LRU_OK) {
    *valor = registro.valor;
  }
  return estado;
}

static lru_status_t cache_borrar(lru_cache_t *cache, int clave) {
  registro_t registro;

  registro_init(&registro, clave, 0);
  return lru_cache_remove(cache, &registro,sizeof(registro_t));
}

/* Sobre una lista vacia, obtener y borrar informan LRU_ERR_EMPTY; con
 * elementos, una clave ausente informa LRU_ERR_NOT_FOUND. El modelo de
 * referencia solo distingue "esta" de "no esta". */
static lru_status_t estado_esperado(const lru_cache_t *cache, int hayModelo) {
  if (hayModelo) {
    return LRU_OK;
  }
  return cache_cantidad(cache) == 0 ? LRU_ERR_EMPTY : LRU_ERR_NOT_FOUND;
}

/* ==========================================================================
 * Creacion
 * ========================================================================== */

TEST(iniciar_capacidad_cero_falla) {
  lru_cache_t cache;
  int leido = 0;

  /* Se crea una cache valida y se comprueba que un pedido invalido no la
   * deja a medio inicializar. */
  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 4, cmp_int), "iniciar valida");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 2), "agregar un elemento");
  ASSERT_EQUAL(LRU_ERR_PARAM, lru_cache_init(&cache, 0, cmp_int),
               "capacidad cero debe rechazarse");
  ASSERT_EQUAL(4, (int)cache_capacidad(&cache),
               "la cache existente no debe cambiar");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache),
               "el elemento debe seguir almacenado");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "y debe seguir siendo accesible");
  ASSERT_EQUAL(2, leido, "con su valor");

  lru_clear(&cache);
  TEST_PASSED("Capacidad cero rechazada sin tocar la cache");
}

TEST(iniciar_cache_vacia) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 5, cmp_int),
               "iniciar con parametros validos debe funcionar");
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "debe empezar vacia");
  ASSERT_EQUAL(5, (int)cache_capacidad(&cache), "debe recordar la capacidad");
  ASSERT_EQUAL(LRU_ERR_EMPTY, cache_obtener(&cache, 7, &leido),
               "buscar en una lista vacia informa lista vacia");
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache),
               "el fallo de busqueda no debe alterar la cantidad");

  lru_clear(&cache);
  TEST_PASSED("Cache recien creada vacia");
}

/* ==========================================================================
 * Insertar y obtener
 * ========================================================================== */

TEST(insertar_y_obtener_un_entero) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 3, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 42, 1234),
               "agregar debe funcionar");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache), "debe haber un elemento");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 42, &leido),
               "obtener el elemento agregado debe funcionar");
  ASSERT_EQUAL(1234, leido, "el valor devuelto debe ser el almacenado");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache),
               "obtener no debe cambiar la cantidad");

  lru_clear(&cache);
  TEST_PASSED("Insertar y obtener un entero");
}

TEST(obtener_clave_inexistente_no_altera_la_cache) {
  lru_cache_t cache;
  registro_t registro;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 10), "agregar 1");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 20), "agregar 2");

  registro_init(&registro, 99, 0);
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND,
               suite_obtener(&cache, &registro, sizeof(registro)),
               "una clave inexistente debe informarse como no encontrada");
  ASSERT_EQUAL(0, registro.valor, "no debe escribirse nada en el buffer");
  ASSERT_EQUAL(2, (int)cache_cantidad(&cache),
               "el fallo no debe alterar la cantidad");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "el primer elemento debe seguir estando");
  ASSERT_EQUAL(10, leido, "con su valor");

  lru_clear(&cache);
  TEST_PASSED("Busqueda fallida sin efectos colaterales");
}

TEST(desborde_descarta_el_menos_usado) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 100), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 100), "agregar B");
  ASSERT_EQUAL(2, (int)cache_cantidad(&cache), "cache llena");

  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 3, 100),
               "agregar C desborda la capacidad");
  ASSERT_EQUAL(2, (int)cache_cantidad(&cache),
               "la cantidad no debe superar la capacidad");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 1, &leido),
               "A era el menos usado y debe haber sido descartado");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 2, &leido),
               "B debe seguir en la cache");
  ASSERT_EQUAL(100, leido, "B conserva su valor");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 3, &leido),
               "C debe estar en la cache");
  ASSERT_EQUAL(100, leido, "C conserva su valor");

  lru_clear(&cache);
  TEST_PASSED("Desborde descarta el menos usado");
}

TEST(obtener_promueve_al_frente) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 7), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 7), "agregar B");

  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "acceder a A lo vuelve el mas reciente");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 3, 7),
               "agregar C desborda la cache");

  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "A fue usado, no debe descartarse");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 2, &leido),
               "B quedo como el menos usado y debe descartarse");

  lru_clear(&cache);
  TEST_PASSED("Acceder promueve al frente");
}

TEST(orden_lru_completo) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 3, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 5), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 5), "agregar B");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 3, 5), "agregar C");
  /* orden de uso: C (mas reciente), B, A (menos reciente) */
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido), "usar A");
  /* orden de uso: A, C, B */
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 4, 5), "agregar D");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 2, &leido),
               "B debe ser el descartado");
  /* orden de uso: D, A, C */
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 5, 5), "agregar E");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 3, &leido),
               "C debe ser el descartado");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido), "A sigue vigente");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 4, &leido), "D sigue vigente");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 5, &leido), "E sigue vigente");
  ASSERT_EQUAL(3, (int)cache_cantidad(&cache), "la cantidad es la capacidad");

  lru_clear(&cache);
  TEST_PASSED("Orden LRU respetado en una secuencia larga");
}

TEST(reemplazo_de_clave_existente_actualiza_y_promueve) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 100), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 200), "agregar B");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 999), "reemplazar A");
  ASSERT_EQUAL(2, (int)cache_cantidad(&cache),
               "reemplazar no debe aumentar la cantidad");

  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "A debe seguir existiendo");
  ASSERT_EQUAL(999, leido, "A debe tener el valor nuevo");

  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 3, 200), "agregar C");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 2, &leido),
               "B era el menos usado: el reemplazo de A lo desplazo");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 1, &leido),
               "A quedo al principio por el reemplazo");

  lru_clear(&cache);
  TEST_PASSED("Reemplazo de clave existente");
}

TEST(capacidad_uno) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 1, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 3), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 3), "agregar B");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache), "capacidad uno respetada");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 1, &leido),
               "A debe haber sido descartado");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 2, &leido), "B debe estar");
  ASSERT_EQUAL(3, leido, "con su valor");

  lru_clear(&cache);
  TEST_PASSED("Cache de capacidad uno");
}

TEST(capacidad_amplia_no_descarta) {
  lru_cache_t cache;
  int claves[5];
  int valores[5];
  int i;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 10, cmp_int), "iniciar");
  for (i = 0; i < 5; i++) {
    claves[i] = i;
    valores[i] = i * 10;
    ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], valores[i]),
                 "agregar elemento");
  }
  ASSERT_EQUAL(5, (int)cache_cantidad(&cache), "nada debe descartarse");
  for (i = 4; i >= 0; i--) {
    leido = 0;
    ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, claves[i], &leido),
                 "todo debe seguir presente");
    ASSERT_EQUAL(valores[i], leido, "el valor debe ser el correcto");
  }

  lru_clear(&cache);
  TEST_PASSED("Sin desborde no se descarta nada");
}

/* ==========================================================================
 * Borrar
 * ========================================================================== */

TEST(borrar_elemento_existente) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 3, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 3), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 3), "agregar B");
  ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, 1), "borrar A");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache), "la cantidad debe bajar");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 1, &leido),
               "A no debe existir");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 2, &leido),
               "B debe seguir existiendo");
  ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, 2), "borrar B");
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "cache vacia");

  lru_clear(&cache);
  TEST_PASSED("Borrar elemento existente");
}

TEST(borrar_clave_inexistente_informa_error) {
  lru_cache_t cache;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 3, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 3), "agregar A");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_borrar(&cache, 8),
               "borrar una clave inexistente debe informar el error");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache),
               "el error no debe alterar la cantidad");

  lru_clear(&cache);
  TEST_PASSED("Borrar clave inexistente");
}

TEST(borrar_permite_reinsertar) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 4, 10), "agregar");
  ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, 4), "borrar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 4, 20),
               "volver a agregar la misma clave");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 4, &leido),
               "debe poder obtenerse");
  ASSERT_EQUAL(20, leido, "debe ser el valor nuevo");
  ASSERT_EQUAL(1, (int)cache_cantidad(&cache), "un unico elemento");

  lru_clear(&cache);
  TEST_PASSED("Reinsertar una clave borrada");
}

/* ==========================================================================
 * Vaciar
 * ========================================================================== */

TEST(vaciar_deja_la_cache_reutilizable) {
  lru_cache_t cache;
  int leido = 0;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 3), "agregar A");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 3), "agregar B");
  lru_clear(&cache);
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "debe quedar vacia");
  ASSERT_EQUAL(LRU_ERR_EMPTY, cache_obtener(&cache, 1, &leido),
               "no debe quedar ningun elemento");
  ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 3),
               "debe poder volver a usarse");
  ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 2, &leido),
               "el nuevo elemento debe estar");
  ASSERT_EQUAL(2, (int)cache_capacidad(&cache),
               "vaciar no cambia la capacidad");

  lru_clear(&cache);
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "vaciar dos veces es seguro");

  lru_clear(&cache);
  TEST_PASSED("Vaciar y reutilizar");
}

/* ==========================================================================
 * Genericidad
 * ========================================================================== */

TEST(datos_de_tipo_definido_por_el_usuario) {
  lru_cache_t cache;
  alumno_t alumnoA;
  alumno_t alumnoB;
  alumno_t leido;

  alumno_init(&alumnoA, 1001, "Ana Gomez", 8.5);
  alumno_init(&alumnoB, 1002, "Beto Diaz", 6.25);
  memset(&leido, 0, sizeof(leido));
  leido.legajo = 1001; /* la busqueda usa la clave del primer campo */

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &alumnoA, sizeof(alumnoA)),
               "agregar alumno A");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &alumnoB, sizeof(alumnoB)),
               "agregar alumno B");
  ASSERT_EQUAL(LRU_OK, suite_obtener(&cache, &leido, sizeof(leido)),
               "obtener alumno A");
  ASSERT_TRUE(memcmp(&alumnoA, &leido, sizeof(alumnoA)) == 0,
              "el dato debe volver byte a byte igual");

  lru_clear(&cache);
  TEST_PASSED("Datos de tipo definido por el usuario");
}

TEST(claves_de_texto) {
  lru_cache_t cache;
  texto_registro_t registroA;
  texto_registro_t registroB;
  texto_registro_t leido;
  texto_registro_t buscado;

  memset(&registroA, 0, sizeof(registroA));
  memset(&registroB, 0, sizeof(registroB));
  strcpy(registroA.clave, "alfa");
  registroA.valor = 4321L;
  strcpy(registroB.clave, "beta");
  registroB.valor = 99L;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_texto), "iniciar");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &registroA, sizeof(registroA)),
               "agregar con clave de texto");
  memset(&leido, 0, sizeof(leido));
  strcpy(leido.clave, "alfa"); /* la busqueda usa la clave del primer campo */
  ASSERT_EQUAL(LRU_OK, suite_obtener(&cache, &leido, sizeof(leido)),
               "obtener con clave de texto");
  ASSERT_TRUE(leido.valor == 4321L, "el valor debe coincidir");

  memset(&buscado, 0, sizeof(buscado));
  strcpy(buscado.clave, "gamma");
  ASSERT_EQUAL(LRU_ERR_NOT_FOUND,
               suite_obtener(&cache, &buscado, sizeof(buscado)),
               "otra clave de texto no debe encontrarse");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &registroB, sizeof(registroB)),
               "segunda clave de texto");
  ASSERT_EQUAL(2, (int)cache_cantidad(&cache), "dos elementos");

  lru_clear(&cache);
  TEST_PASSED("Claves de texto y valores long");
}

TEST(varias_claves_con_el_mismo_prefijo) {
  lru_cache_t cache;
  texto_registro_t registroA;
  texto_registro_t registroB;
  texto_registro_t leido;

  memset(&registroA, 0, sizeof(registroA));
  memset(&registroB, 0, sizeof(registroB));
  strcpy(registroA.clave, "clave-1");
  registroA.valor = 1L;
  strcpy(registroB.clave, "clave-10");
  registroB.valor = 2L;

  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 4, cmp_texto), "iniciar");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &registroA, sizeof(registroA)),
               "agregar clave-1");
  ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &registroB, sizeof(registroB)),
               "agregar clave-10");

  memset(&leido, 0, sizeof(leido));
  strcpy(leido.clave, "clave-1");
  ASSERT_EQUAL(LRU_OK, suite_obtener(&cache, &leido, sizeof(leido)),
               "obtener clave-1");
  ASSERT_TRUE(leido.valor == 1L, "clave-1 no debe confundirse con clave-10");

  memset(&leido, 0, sizeof(leido));
  strcpy(leido.clave, "clave-10");
  ASSERT_EQUAL(LRU_OK, suite_obtener(&cache, &leido, sizeof(leido)),
               "obtener clave-10");
  ASSERT_TRUE(leido.valor == 2L, "clave-10 debe tener su propio valor");

  lru_clear(&cache);
  TEST_PASSED("Claves que comparten prefijo");
}

/* ==========================================================================
 * Modelo de referencia: prueba de consistencia a largo plazo
 * ========================================================================== */

#define MODELO_MAX 32

typedef struct {
  int presente;
  int clave;
  int valor;
  unsigned long sello;
} modelo_entrada_t;

typedef struct {
  modelo_entrada_t entradas[MODELO_MAX];
  unsigned capacidad;
  unsigned cantidad;
  unsigned long reloj;
} modelo_t;

static void modelo_init(modelo_t *modelo, unsigned capacidad) {
  memset(modelo, 0, sizeof(*modelo));
  modelo->capacidad = capacidad;
}

static unsigned modelo_buscar(const modelo_t *modelo, int clave) {
  unsigned i;

  for (i = 0; i < MODELO_MAX; i++) {
    if (modelo->entradas[i].presente && modelo->entradas[i].clave == clave) {
      return i;
    }
  }
  return MODELO_MAX;
}

static void modelo_agregar(modelo_t *modelo, int clave, int valor) {
  unsigned i;
  unsigned victima;
  unsigned long menor;

  i = modelo_buscar(modelo, clave);
  modelo->reloj++;
  if (i != MODELO_MAX) {
    modelo->entradas[i].valor = valor;
    modelo->entradas[i].sello = modelo->reloj;
    return;
  }

  for (i = 0; i < MODELO_MAX; i++) {
    if (!modelo->entradas[i].presente) {
      break;
    }
  }
  modelo->entradas[i].presente = 1;
  modelo->entradas[i].clave = clave;
  modelo->entradas[i].valor = valor;
  modelo->entradas[i].sello = modelo->reloj;
  modelo->cantidad++;

  if (modelo->cantidad > modelo->capacidad) {
    victima = MODELO_MAX;
    menor = 0;
    for (i = 0; i < MODELO_MAX; i++) {
      if (!modelo->entradas[i].presente) {
        continue;
      }
      if (victima == MODELO_MAX || modelo->entradas[i].sello < menor) {
        victima = i;
        menor = modelo->entradas[i].sello;
      }
    }
    modelo->entradas[victima].presente = 0;
    modelo->cantidad--;
  }
}

static int modelo_obtener(modelo_t *modelo, int clave, int *valor) {
  unsigned i;

  i = modelo_buscar(modelo, clave);
  if (i == MODELO_MAX) {
    return 0;
  }
  modelo->reloj++;
  modelo->entradas[i].sello = modelo->reloj;
  *valor = modelo->entradas[i].valor;
  return 1;
}

static int modelo_borrar(modelo_t *modelo, int clave) {
  unsigned i;

  i = modelo_buscar(modelo, clave);
  if (i == MODELO_MAX) {
    return 0;
  }
  modelo->entradas[i].presente = 0;
  modelo->cantidad--;
  return 1;
}

/**
 * @brief Generador congruencial lineal de 32 bits: misma secuencia en
 * cualquier plataforma.
 */
static unsigned long rng_siguiente(unsigned long *estado) {
  *estado = (*estado * 1103515245UL + 12345UL) & 0xFFFFFFFFUL;
  return (*estado >> 16) & 0x7FFFUL;
}

TEST(secuencia_larga_contra_modelo) {
  lru_cache_t cache;
  modelo_t modelo;
  unsigned long estado;
  int paso;
  int operacion;
  int clave;
  int valor;
  int leido;
  int claveVerificada;
  int hayModelo;
  int valorModelo;

  estado = 2026UL;
  ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 8, cmp_int), "iniciar");
  modelo_init(&modelo, 8);

  for (paso = 0; paso < 2000; paso++) {
    operacion = (int)(rng_siguiente(&estado) % 10UL);
    clave = (int)(rng_siguiente(&estado) % 20UL);

    if (operacion < 6) {
      valor = (int)(rng_siguiente(&estado) % 1000UL);
      modelo_agregar(&modelo, clave, valor);
      ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, clave, valor),
                   "agregar no debe fallar");
    } else if (operacion < 9) {
      hayModelo = modelo_obtener(&modelo, clave, &valorModelo);
      leido = 0;
      ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                   cache_obtener(&cache, clave, &leido),
                   "obtener debe coincidir con el modelo");
      if (hayModelo) {
        ASSERT_EQUAL(valorModelo, leido, "el valor debe coincidir");
      }
    } else {
      hayModelo = modelo_borrar(&modelo, clave);
      ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                   cache_borrar(&cache, clave),
                   "borrar debe coincidir con el modelo");
    }

    /* Verificacion completa: obtener promueve, asi que se aplica la misma
     * promocion al modelo para no desincronizar el orden de uso. */
    for (claveVerificada = 0; claveVerificada < 20; claveVerificada++) {
      hayModelo = modelo_obtener(&modelo, claveVerificada, &valorModelo);
      leido = 0;
      ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                   cache_obtener(&cache, claveVerificada, &leido),
                   "la cache debe tener exactamente las claves del modelo");
      if (hayModelo) {
        ASSERT_EQUAL(valorModelo, leido,
                     "el valor debe coincidir con el modelo");
      }
    }
    ASSERT_EQUAL((int)modelo.cantidad, (int)cache_cantidad(&cache),
                 "la cantidad debe coincidir con el modelo");
    ASSERT_TRUE(cache_cantidad(&cache) <= 8,
                "la capacidad nunca debe superarse");
  }

  lru_clear(&cache);
  ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "vaciar deja la cache vacia");
  lru_clear(&cache);
  TEST_PASSED("2000 operaciones contra el modelo de referencia");
}

TEST(modelo_capacidades_chicas) {
  lru_cache_t cache;
  modelo_t modelo;
  unsigned long estado;
  unsigned capacidad;
  int paso;
  int operacion;
  int clave;
  int valor;
  int leido;
  int claveVerificada;
  int hayModelo;
  int valorModelo;

  estado = 777UL;
  for (capacidad = 1; capacidad <= 3; capacidad++) {
    ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, capacidad, cmp_int), "iniciar");
    modelo_init(&modelo, capacidad);

    for (paso = 0; paso < 400; paso++) {
      operacion = (int)(rng_siguiente(&estado) % 10UL);
      clave = (int)(rng_siguiente(&estado) % 8UL);

      if (operacion < 6) {
        valor = (int)(rng_siguiente(&estado) % 1000UL);
        modelo_agregar(&modelo, clave, valor);
        ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, clave, valor), "agregar");
      } else if (operacion < 9) {
        hayModelo = modelo_obtener(&modelo, clave, &valorModelo);
        leido = 0;
        ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                     cache_obtener(&cache, clave, &leido), "obtener");
        if (hayModelo) {
          ASSERT_EQUAL(valorModelo, leido, "valor");
        }
      } else {
        hayModelo = modelo_borrar(&modelo, clave);
        ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                     cache_borrar(&cache, clave), "borrar");
      }

      for (claveVerificada = 0; claveVerificada < 8; claveVerificada++) {
        hayModelo = modelo_obtener(&modelo, claveVerificada, &valorModelo);
        leido = 0;
        ASSERT_EQUAL(estado_esperado(&cache, hayModelo),
                     cache_obtener(&cache, claveVerificada, &leido),
                     "presencia igual al modelo");
        if (hayModelo) {
          ASSERT_EQUAL(valorModelo, leido, "valor igual al modelo");
        }
      }
      ASSERT_EQUAL((int)modelo.cantidad, (int)cache_cantidad(&cache),
                   "cantidad");
      ASSERT_TRUE(cache_cantidad(&cache) <= capacidad,
                  "la capacidad nunca se supera");
    }

    lru_clear(&cache);
    ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "vaciar");
    lru_clear(&cache);
  }
  TEST_PASSED("Modelo LRU con capacidades 1 a 3");
}

#endif /* TESTS_COMUNES_H */
