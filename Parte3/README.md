# Lotes de prueba de la parte 3

## Archivos listos para usar

La carpeta `Lotes/` contiene una base compartida de 50.000 posteos y tres
archivos de 1.000 usuarios cada uno. Cada usuario tiene un feed de 20 posteos.
Todos los lotes se generaron con la semilla **2026**.

| Archivo | Contenido |
| --- | --- |
| `posteos.dat` | 50.000 registros `tp_posteo_t`, ordenados por identificador. |
| `usuarios_concentrado.dat` | Los usuarios comparten un conjunto de 100 posteos frecuentes. Cada feed contiene 20 distintos. |
| `usuarios_disperso.dat` | Los 20.000 accesos usan identificadores diferentes, en orden aleatorio. |
| `usuarios_ordenado.dat` | Los 20.000 accesos usan identificadores de 0 a 19.999, en orden creciente incluso entre usuarios. |
| `parametros.txt` | Semilla, cantidades, tamanos de registros y convenciones. |

## Como regenerarlos en Code::Blocks

1. Abrir **`Parte3/GeneradorLotes.cbp`**, que es una aplicacion independiente.
2. Seleccionar el objetivo **Generar lotes**.
3. Usar **Build and Run**. Sin argumentos genera los parametros anteriores
   dentro de `Lotes/`, respecto de la carpeta de trabajo del programa.

Para otros parametros, configurar los argumentos del objetivo, por ejemplo:

```text
42 200 Lotes_semilla42 100
```

El orden es: **semilla, cantidad de usuarios, carpeta, cantidad de posteos frecuentes**.
Se aceptan entre 200 y 2.500 usuarios. El limite permite que el lote disperso
no repita posteos y que el ordenado sea creciente en toda la ejecucion.
El conjunto frecuente puede tener entre 20 y 50.000 posteos; usar 100 permite
contrastar capacidades de cache menores y mayores que ese conjunto.
La carpeta de salida se crea si falta; su carpeta padre debe existir.
Si ya contiene estos archivos, el generador los reemplaza.

Tambien se puede compilar desde la raiz del repositorio:

```text
gcc -std=c89 -pedantic -Wall -Wextra -Iincludes Parte3/generar_lotes.c -o Parte3/generar_lotes.exe
```

Y ejecutar desde esa misma raiz:

```text
Parte3\generar_lotes.exe 2026 1000 Parte3\Lotes 100
```

## Formato y lectura desde C

Los tipos estan definidos en **`includes/datos_simulacion.h`**:

```c
typedef struct {
    int id;
    char texto[141];
} tp_posteo_t;

typedef struct {
    char usuario[15];
    int feed[20];
} tp_usuario_t;
```

Los identificadores empiezan en **0**: el posteo 0 es el primer registro.
Los archivos son binarios sin encabezado y se leen con `fread` en modo `rb`.
El generador inicializa tambien el relleno de cada estructura antes de escribirla.
Los textos son ASCII, tienen menos de 140 caracteres y terminan en `\0`.
Los nombres de usuario tambien terminan en `\0`.

Los archivos entregados usan enteros de 32 bits, orden de bytes little endian
y la alineacion normal de GCC/MinGW: **148 bytes por posteo y 96 por usuario**.
Usar el encabezado compartido y no aplicar `#pragma pack(1)`: hay bytes de
relleno entre campos o al final de cada registro. No sumar simplemente los
tamanos de los campos para calcular desplazamientos.

Para acceder directamente a un posteo:

```c
int leer_posteo(FILE *archivo, int id, tp_posteo_t *posteo)
{
    if (id < 0 || id >= TP_CANT_POSTEOS)
        return 0;

    if (fseek(archivo, (long)id * (long)sizeof(*posteo), SEEK_SET) != 0)
        return 0;

    return fread(posteo, sizeof(*posteo), 1, archivo) == 1;
}
```

La misma semilla y los mismos parametros, con este formato de registros,
producen exactamente los mismos archivos. El generador usa su propio estado
pseudoaleatorio de 32 bits para no depender de `rand()` de cada compilador.

## Uso posterior en la simulacion

Para cada lote de usuarios, procesar la misma secuencia con:

1. Acceso directo a posteos sin cache.
2. La cache de la parte 1.
3. La cache de la parte 2.

Se sugieren capacidades **10, 50, 100, 500 y 1.000**, reiniciando la cache
antes de cada ejecucion. En el concentrado, una capacidad de 100 o mas puede
conservar todos los posteos frecuentes; en el disperso y el ordenado no hay
reutilizacion de claves dentro de esta configuracion.

En el ordenado, registrar tambien la altura del arbol: una insercion creciente
en un ABB sin balanceo puede degenerarlo en una cadena. Ese comportamiento
es parte del experimento, por lo que el generador no mezcla los identificadores.

Estos archivos son los **datos de entrada**. Los tiempos, aciertos, fallos,
lecturas del archivo y el informe se obtienen despues, ejecutando la simulacion.
