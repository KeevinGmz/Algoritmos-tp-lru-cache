/*
 * @file test_lru_cache_parte2.c
 * @brief Suite de tests del TDA LRU Cache - Parte 2 (lista doblemente
 *        enlazada + arbol binario de busqueda como indice).
 *
 * Esta version implementa el TDA con lru_cache_index.h:
 *   - nodos con doble enlace (next, prev) y punteros head / last;
 *   - un arbol binario de busqueda (bst_node_t) como indice de las claves;
 *   - la primitiva de lectura no recibe el tamanio del buffer de destino:
 *     lru_cache_get(cache, data);
 *   - la primitiva extra lru_cache_height(cache) informa la altura del arbol.
 *
 * Los tests de comportamiento son los mismos que los de la Parte 1 y viven en
 * "tests_comunes.h". Aca se agrega lo propio de esta implementacion:
 *   - altura del arbol indice;
 *   - consistencia lista/arbol en los tres casos de borrado del arbol (nodo
 *     hoja, con un hijo y con dos hijos);
 *   - que el reemplazo de una clave no duplique nodos en el arbol.
 *
 * Estado conocido de esta parte (no lo cubre esta suite, es implementacion):
 *   - src/lru_cache_index.c todavia no implementa las primitivas, asi que la
 *     suite compila pero no enlaza hasta que existan;
 *   - lru_cache_t de lru_cache_index.h todavia no declara la raiz del arbol
 *     (por ejemplo bst_node_t *root), que es donde el indice necesita vivir.
 *     Estos tests describen el contrato esperado de lru_cache_height.
 */

//#include <lru_cache_index.h>
#include "../Tweener.h"

/* La parte 2 no recibe el tamanio del buffer de destino: el adaptador lo
 * ignora y las primitivas copian el dato almacenado completo. */
#define CACHE_GET(cache, dato, tam) lru_cache_get((cache), (dato),(tam))

#include "tests_comunes.h"

/* ==========================================================================
 * Altura del arbol indice
 * ========================================================================== */

TEST(altura_cache_vacia_es_cero) {
	lru_cache_t cache;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 10, cmp_int), "iniciar");
	ASSERT_EQUAL(0, (int)lru_cache_height(&cache),
	             "arbol vacio: altura cero");

	lru_clear(&cache);
	TEST_PASSED("Altura de un arbol vacio");
}

TEST(altura_un_solo_elemento_es_uno) {
	lru_cache_t cache;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 10, cmp_int), "iniciar");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 500, 1), "agregar");
	ASSERT_EQUAL(1, (int)lru_cache_height(&cache), "un nodo: altura uno");

	lru_clear(&cache);
	TEST_PASSED("Altura de un arbol de un nodo");
}

TEST(altura_arbol_balanceado) {
	lru_cache_t cache;
	int claves[7];
	int i;

	/* Insercion en orden raiz-izq-der: 8 / 4,12 / 2,6,10,14 -> altura 3. */
	claves[0] = 8;
	claves[1] = 4;
	claves[2] = 12;
	claves[3] = 2;
	claves[4] = 6;
	claves[5] = 10;
	claves[6] = 14;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 10, cmp_int), "iniciar");
	for (i = 0; i < 7; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], 1), "agregar");
	}
	ASSERT_EQUAL(3, (int)lru_cache_height(&cache),
	             "un arbol completo de 3 niveles tiene altura 3");
	ASSERT_EQUAL(7, (int)cache_cantidad(&cache), "siete elementos");

	lru_clear(&cache);
	TEST_PASSED("Altura de un arbol balanceado");
}

TEST(altura_arbol_degenerado) {
	lru_cache_t cache;
	int cantidad;
	int i;

	cantidad = 1000;
	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, (unsigned)cantidad, cmp_int),
	             "iniciar con capacidad para todas las claves");
	for (i = 1; i <= cantidad; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, i, 1),
		             "agregar en orden creciente");
	}
	/* Al insertar claves crecientes cada nodo cae a la derecha del anterior:
	 * el arbol se degenera en una lista y la altura es la cantidad de nodos. */
	ASSERT_EQUAL(cantidad, (int)lru_cache_height(&cache),
	             "insercion creciente degenera el arbol");
	ASSERT_EQUAL(cantidad, (int)cache_cantidad(&cache),
	             "nada se descarto");

	lru_clear(&cache);
	TEST_PASSED("Altura de un arbol degenerado (lote ordenado)");
}

TEST(altura_no_cambia_al_reemplazar_ni_al_obtener) {
	lru_cache_t cache;
	int valorNuevo = 9;
	int leido = 0;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 5, cmp_int), "iniciar");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 8, 1), "agregar 8");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 4, 1), "agregar 4");
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 12, 1), "agregar 12");
	ASSERT_EQUAL(2, (int)lru_cache_height(&cache), "altura inicial");

	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, 12, valorNuevo),
	             "reemplazar 12");
	ASSERT_EQUAL(2, (int)lru_cache_height(&cache),
	             "reemplazar no debe crear un nodo nuevo en el arbol");
	ASSERT_EQUAL(3, (int)cache_cantidad(&cache), "siguen siendo tres");

	ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, 12, &leido), "obtener 12");
	ASSERT_EQUAL(valorNuevo, leido, "el valor reemplazado se conserva");
	ASSERT_EQUAL(2, (int)lru_cache_height(&cache),
	             "acceder cambia el orden de uso, no la forma del arbol");

	lru_clear(&cache);
	TEST_PASSED("Reemplazo y acceso no alteran la altura");
}

TEST(altura_tras_desborde_y_borrado) {
	lru_cache_t cache;
	int i;
	int claveDescartada = 1;
	int leido = 0;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 3, cmp_int), "iniciar");
	for (i = 1; i <= 3; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, i, 1),
		             "agregar creciente");
	}
	ASSERT_EQUAL(3, (int)lru_cache_height(&cache), "tres nodos en cadena");

	/* Agregar 4 descarta el 1: el arbol queda 2-3-4 y sigue con altura 3. */
	i = 4;
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, i, 1), "agregar 4");
	ASSERT_EQUAL(3, (int)cache_cantidad(&cache), "cantidad tope");
	ASSERT_EQUAL(3, (int)lru_cache_height(&cache), "la altura se mantiene");
	ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, claveDescartada,
	             &leido),
	             "el descartado por desborde no debe seguir en el indice");

	/* Borrar la raiz del subarbol: el arbol queda 3-4 -> altura 2. */
	i = 2;
	ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, i), "borrar la raiz");
	ASSERT_EQUAL(2, (int)lru_cache_height(&cache),
	             "altura luego de borrar la raiz");
	ASSERT_EQUAL(2, (int)cache_cantidad(&cache), "cantidad luego de borrar");
	i = 3;
	ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, i, &leido),
	             "el resto sigue disponible");

	lru_clear(&cache);
	ASSERT_EQUAL(0, (int)lru_cache_height(&cache),
	             "vaciar deja el arbol vacio");
	ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "y la lista tambien");

	lru_clear(&cache);
	TEST_PASSED("Altura despues de desborde, borrado y vaciado");
}

/* ==========================================================================
 * Consistencia lista / arbol en los tres casos de borrado del ABB
 * ========================================================================== */

TEST(borrar_nodo_hoja_del_arbol) {
	lru_cache_t cache;
	int claves[7];
	int claveHoja = 10;
	int i;
	int leido = 0;

	/* Arbol: 50 / 30,70 / 20,40,60,80. 10 queda como hoja de 20. */
	claves[0] = 50;
	claves[1] = 30;
	claves[2] = 70;
	claves[3] = 20;
	claves[4] = 40;
	claves[5] = 60;
	claves[6] = 80;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 20, cmp_int), "iniciar");
	for (i = 0; i < 7; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], 1), "agregar");
	}
	ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claveHoja, 1),
	             "agregar hoja 10");

	ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, claveHoja), "borrar hoja 10");
	ASSERT_EQUAL(7, (int)cache_cantidad(&cache), "quedan siete");
	ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, claveHoja, &leido),
	             "la hoja borrada no debe encontrarse");
	for (i = 0; i < 7; i++) {
		leido = 0;
		ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, claves[i], &leido),
		             "el resto del indice debe seguir intacto");
		ASSERT_EQUAL(1, leido, "con su valor");
	}

	lru_clear(&cache);
	TEST_PASSED("Borrado de una hoja del arbol indice");
}

TEST(borrar_nodo_con_un_hijo_del_arbol) {
	lru_cache_t cache;
	int claves[8];
	int claveConHijo = 20;
	int i;
	int leido = 0;

	/* Arbol: 50 / 30,70 / 20,40,60,80 / 10 (hijo izquierdo de 20). */
	claves[0] = 50;
	claves[1] = 30;
	claves[2] = 70;
	claves[3] = 20;
	claves[4] = 40;
	claves[5] = 60;
	claves[6] = 80;
	claves[7] = 10;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 20, cmp_int), "iniciar");
	for (i = 0; i < 8; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], 1), "agregar");
	}

	ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, claveConHijo),
	             "borrar 20, que tiene un solo hijo");
	ASSERT_EQUAL(7, (int)cache_cantidad(&cache), "quedan siete");
	ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, claveConHijo,
	             &leido), "el nodo borrado no debe encontrarse");
	leido = 0;
	ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, claves[7], &leido),
	             "el hijo huerfano debe seguir en el indice");
	ASSERT_EQUAL(1, leido, "con su valor");
	for (i = 0; i < 7; i++) {
		if (claves[i] == claveConHijo) {
			continue; /* la clave borrada no debe seguir */
		}
		ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, claves[i], &leido),
		             "el resto debe seguir disponible");
	}

	lru_clear(&cache);
	TEST_PASSED("Borrado de un nodo con un hijo");
}

TEST(borrar_raiz_con_dos_hijos) {
	lru_cache_t cache;
	int claves[7];
	int claveRaiz = 50;
	int i;
	int leido = 0;

	/* Arbol: 50 / 30,70 / 20,40,60,80. La raiz tiene dos hijos. */
	claves[0] = 50;
	claves[1] = 30;
	claves[2] = 70;
	claves[3] = 20;
	claves[4] = 40;
	claves[5] = 60;
	claves[6] = 80;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 20, cmp_int), "iniciar");
	for (i = 0; i < 7; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], 1), "agregar");
	}

	ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, claveRaiz),
	             "borrar la raiz del arbol");
	ASSERT_EQUAL(6, (int)cache_cantidad(&cache), "quedan seis");
	ASSERT_EQUAL(LRU_ERR_NOT_FOUND, cache_obtener(&cache, claveRaiz, &leido),
	             "la raiz borrada no debe encontrarse");
	for (i = 1; i < 7; i++) {
		leido = 0;
		ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, claves[i], &leido),
		             "los seis restantes deben seguir");
		ASSERT_EQUAL(1, leido, "con su valor");
	}
	ASSERT_EQUAL(3, (int)lru_cache_height(&cache),
	             "el arbol mantiene tres niveles");

	lru_clear(&cache);
	TEST_PASSED("Borrado de la raiz con dos hijos");
}

TEST(borrar_y_reinsertar_alternados_mantiene_ambas_estructuras) {
	lru_cache_t cache;
	int clave;
	int leido = 0;
	int ciclo;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 5, cmp_int), "iniciar");
	for (ciclo = 0; ciclo < 40; ciclo++) {
		clave = ciclo % 7;
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, clave, ciclo), "agregar");
		ASSERT_EQUAL(LRU_OK, cache_obtener(&cache, clave, &leido), "obtener");
		ASSERT_EQUAL(ciclo, leido, "el valor debe ser el ultimo agregado");
		ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, clave), "borrar");
		ASSERT_EQUAL(LRU_ERR_EMPTY, cache_obtener(&cache, clave, &leido),
		             "despues de borrar no debe quedar rastro en el indice");
		ASSERT_TRUE(cache_cantidad(&cache) <= 5,
		            "la capacidad nunca debe superarse");
		ASSERT_TRUE(lru_cache_height(&cache) >= 0,
		            "la altura debe seguir siendo valida");
	}
	ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "debe quedar vacia");
	ASSERT_EQUAL(0, (int)lru_cache_height(&cache), "y el arbol tambien");

	lru_clear(&cache);
	TEST_PASSED("Altas, accesos y bajas alternados");
}

TEST(altura_tras_borrar_todo_y_reinsertar) {
	lru_cache_t cache;
	int claves[7];
	int orden[4];
	int i;
	int clave;

	claves[0] = 1;
	claves[1] = 2;
	claves[2] = 3;
	claves[3] = 4;
	claves[4] = 5;
	claves[5] = 6;
	claves[6] = 7;
	orden[0] = 5;
	orden[1] = 3;
	orden[2] = 8;
	orden[3] = 1;

	ASSERT_EQUAL(LRU_OK, lru_cache_init(&cache, 20, cmp_int), "iniciar");
	for (i = 0; i < 7; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, claves[i], 1),
		             "agregar creciente");
	}
	ASSERT_EQUAL(7, (int)lru_cache_height(&cache), "arbol degenerado inicial");

	for (i = 0; i < 7; i++) {
		clave = claves[i];
		ASSERT_EQUAL(LRU_OK, cache_borrar(&cache, clave), "borrar");
	}
	ASSERT_EQUAL(0, (int)cache_cantidad(&cache), "quedo vacia");
	ASSERT_EQUAL(0, (int)lru_cache_height(&cache),
	             "altura cero tras borrar todo");

	for (i = 0; i < 4; i++) {
		ASSERT_EQUAL(LRU_OK, cache_agregar(&cache, orden[i], 1), "reinsertar");
	}
	ASSERT_EQUAL(3, (int)lru_cache_height(&cache), "5 / 3,8 / 1 -> altura tres");
	ASSERT_EQUAL(4, (int)cache_cantidad(&cache), "cuatro elementos");

	lru_clear(&cache);
	ASSERT_EQUAL(0, (int)lru_cache_height(&cache), "altura cero tras vaciar");

	lru_clear(&cache);
	TEST_PASSED("Altura tras borrar todo y reinsertar");
}

/* ==========================================================================
 * Main
 * ========================================================================== */

int main(void)
{
    lru_cache_t cache;
    lru_cache_init(&cache,20,cmp);
	/*printf("\n+----------------------------------------+\n");
	printf("|  TEST SUITE: LRU Cache - Parte 2       |\n");
	printf("|  lista doble + arbol indice            |\n");
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

	/* Propios de la parte 2: altura y consistencia lista/arbol. */
	/* RUN_TEST(altura_cache_vacia_es_cero);
	RUN_TEST(altura_un_solo_elemento_es_uno);
	RUN_TEST(altura_arbol_balanceado);
	RUN_TEST(altura_arbol_degenerado);
	RUN_TEST(altura_no_cambia_al_reemplazar_ni_al_obtener);
	RUN_TEST(altura_tras_desborde_y_borrado);
	RUN_TEST(borrar_nodo_hoja_del_arbol);
	RUN_TEST(borrar_nodo_con_un_hijo_del_arbol);
	RUN_TEST(borrar_raiz_con_dos_hijos);
	RUN_TEST(borrar_y_reinsertar_alternados_mantiene_ambas_estructuras);
	RUN_TEST(altura_tras_borrar_todo_y_reinsertar);
    */
	/*TEST_SUMMARY();*/
    generarLotePruebas(1,1);
    int r=leerArchivoPosteos();
    printf("---------------------------------------------\n");

    int d=leerArchivoPosteosParte1(&cache);
    //lru_clear(&cache);
    return d;
}
