/*
 * @file test_lru_cache_parte1.c
 * @brief Suite de tests del TDA LRU Cache - Parte 1 (lista simplemente
 *        enlazada).
 *
 * Esta version implementa el TDA con lru_cache_list.h:
 *   - nodos con un solo enlace (next);
 *   - la primitiva de lectura recibe el tamanio del buffer de destino:
 *     lru_cache_get(cache, data, tam).
 *
 * Los tests de comportamiento son los mismos que los de la Parte 2 y viven en
 * "tests_comunes.h", porque ambas versiones deben cumplir el mismo contrato.
 * Aca se agrega solo lo propio de esta version: los casos que dependen del
 * tamanio del buffer de destino (LRU_ERR_TAM), que la parte 2 no puede
 * expresar porque su lru_cache_get no recibe `tam`.
 */

#include <lru_cache_list.h>

/* La parte 1 conserva el parametro `tam` de la primitiva de lectura. */
#define CACHE_GET(cache, dato, tam) lru_cache_get((cache), (dato), (tam))

#include "tests_comunes.h"

/* ==========================================================================
 * Tamanio del buffer de destino (solo parte 1)
 * ========================================================================== */

TEST(obtener_con_buffer_corto_informa_error)
{
	lru_cache_t cache;
	registro_t registro;
	int leido = 0;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 1, 8), "agregar A");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 2, 8), "agregar B");

	registro_init(&registro, 1, 0);
	ASSERT_EQUAL(0, registro.valor, "no debe escribirse el valor");
	ASSERT_EQUAL(2, (int)cache_cantidad(&cache), "la cantidad no cambia");

	/* A no debe haberse promovido: sigue siendo el menos usado. */
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 3, 8), "agregar C");
	ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, 1, &leido),
		     "A debe seguir siendo el menos usado");

	lru_clear(&cache);
	TEST_PASSED("Buffer corto sin efectos colaterales");
}

TEST(obtener_buffer_corto_con_dato_de_usuario)
{
	lru_cache_t cache;
	alumno_t alumno;
	alumno_t leido;
	int legajo = 1001;

	alumno_init(&alumno, 1001, "Ana Gomez", 8.5);
	memset(&leido, 0, sizeof(leido));
	leido.legajo = legajo;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 2, cmp_int), "iniciar");
	ASSERT_EQUAL(LRU_OK, lru_cache_insert(&cache, &alumno, sizeof(alumno)),
		     "agregar alumno");
	ASSERT_TRUE(leido.nombre[0] == '\0', "no debe escribirse el nombre");
	ASSERT_EQUAL(1, (int)cache_cantidad(&cache), "la cantidad no cambia");

	lru_clear(&cache);
	TEST_PASSED("Buffer corto para un dato de usuario");
}

/* ==========================================================================
 * Main
 * ========================================================================== */

int main(void)
{
	printf("\n+----------------------------------------+\n");
	printf("|  TEST SUITE: LRU Cache - Parte 1       |\n");
	printf("|  lista simplemente enlazada            |\n");
	printf("+----------------------------------------+\n");

	RUN_TEST(iniciar_capacidad_cero_falla);
	RUN_TEST(iniciar_cache_vacia);
	RUN_TEST(insertar_y_obtener_un_entero);
	RUN_TEST(obtener_clave_inexistente_no_altera_la_cache);
	RUN_TEST(desborde_descarta_el_menos_usado);
	RUN_TEST(obtener_promueve_al_frente);
	RUN_TEST(orden_lru_completo);
	RUN_TEST(reemplazo_de_clave_existente_actualiza_y_promueve);
	RUN_TEST(capacidad_uno);
	RUN_TEST(capacidad_amplia_no_descarta);
	RUN_TEST(borrar_elemento_existente);
	RUN_TEST(borrar_clave_inexistente_informa_error);
	RUN_TEST(borrar_permite_reinsertar);
	RUN_TEST(vaciar_deja_la_cache_reutilizable);
	RUN_TEST(datos_de_tipo_definido_por_el_usuario);
	RUN_TEST(claves_de_texto);
	RUN_TEST(varias_claves_con_el_mismo_prefijo);
	RUN_TEST(secuencia_larga_contra_modelo);
	RUN_TEST(modelo_capacidades_chicas);

	/* Propios de la parte 1: tamanio del buffer de destino. */
	RUN_TEST(obtener_con_buffer_corto_informa_error);
	RUN_TEST(obtener_buffer_corto_con_dato_de_usuario);

	TEST_SUMMARY();
}
