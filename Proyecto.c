

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*
En este apartado se crea el nodo que se usa en las listas doblemente enlazadas
este tendrá una llave la cual se asocia al valor que se quiere guardar en la tabla hash
el void* valor es un apuntador genérico que puede apuntar a cualquier tipo de dato
el siguiente es un apuntador al siguiente nodo en la lista enlazada
*/

#define MAX_REFERENCIAS 1000

typedef struct nodoHash { 
    int llave;
    void *valor;
    struct nodoHash *siguiente;

} nodoHash;

/* 
Se crea la estructura la tabla hash donde tendremos el número de cubetas que tendrá la tabla
y un arreglo de punteros a nodos donde cada entrada buckets[i] es la cabeza de la lista de esa cubeta
*/

typedef struct tablaHash {
    int nbuckets;
    nodoHash **buckets;
} tablaHash;

/*
Creamos la tabla hash, para esto se necesita el número de cubetas que tendrá la tabla
luego reservamos memoria para la estructura y malloc() devuelve un puntero sin inicializar
después se inicializa con calloc() que asigna memoria y la inicializa en 0
y finalmente se devuelve el puntero a la tabla hash creada
*/

static tablaHash* crearTablaHash(int nbuckets) {
    tablaHash* tabla = malloc(sizeof(tablaHash));
    tabla -> nbuckets = nbuckets;
    tabla -> buckets = calloc(nbuckets, sizeof(nodoHash*));
    return tabla;
}

/*
Esta función libera memoria, iterando en cada cubeta primeramente guardando el nodo siguiente antes
de liberar el nodo actual, Si el usuario pasó una función liberarValor y actual->valor no es NULL,
llama a liberarValor(actual->valor) para liberar la memoria del valor.
Luego se libera el array de cubetas y finalmente la estructura de la tabla hash.
*/

static void liberarTablaHash(tablaHash* tabla, void (*liberarValor)(void *)) {
    for(int i = 0; i < tabla -> nbuckets; i++) {
        nodoHash* actual = tabla -> buckets[i];
        while(actual) {
            nodoHash* temp = actual->siguiente;
            if(liberarValor && actual->valor) liberarValor(actual->valor);
            free(actual);
            actual = temp;
        }
    }
    free(tabla -> buckets);
    free(tabla);
}

/*
Esta funcion busca la posición en la tabla hash usando una función hash simple
y devuelve el índice de la cubeta correspondiente a la llave dada.

¿Cómo?

Se mezcla la llave usando operaciones bit a bit a través de desplazamientos y XOR y al final multiplica
por una constante grande para dispersar los bits. Finalmente, toma el resultado módulo el número de
cubetas para obtener un índice válido dentro del rango de la tabla hash.

técnicamente podemos aplicar la linea 85 más de una vez para mejorar la dispersión de los bits
pero en este caso con 2 veces es suficiente.
*/

static int hashTH(tablaHash *tabla, int llave) {
    uint32_t x = (uint32_t) llave;
    x = (x^(x >> 16) ^ x) * 0x45d9f3b;
    x = (x^(x >> 16) ^ x) * 0x45d9f3b;
    x = x^(x >> 16);

    return x % tabla -> nbuckets;
}

/*
La función realiza una búsqueda en la tabla hash a través de la llave proporcionada.
Si encuentra un nodo con la llave correspondiente, devuelve el valor asociado a esa llave.
Si no encuentra la llave, devuelve NULL.
*/

static void *obtenerTH(tablaHash *tabla, int llave) {
    int indice = hashTH(tabla, llave);
    nodoHash* actual = tabla -> buckets[indice];

    while(actual) {
        if(actual -> llave == llave) {
            return actual -> valor;
        }
        actual = actual -> siguiente;
    }
    return NULL;
}

/*
Técnicamente esta función inserta un nuevo nodo en la tabla hash o actualiza el valor
si la llave ya existe. Primero calcula el índice de la cubeta usando la función hash.
Luego recorre la lista enlazada en esa cubeta para verificar si la llave ya existe.
Si encuentra la llave, actualiza el valor y retorna. Si no encuentra la llave, crea
un nuevo nodo, lo inicializa con la llave y el valor proporcionados, y lo inserta al
inicio de la lista enlazada en esa cubeta.
*/

static void insertarTH(tablaHash *tabla, int llave, void *valor) {
    int indice = hashTH(tabla, llave);
    nodoHash* actual = tabla -> buckets[indice];

    while(actual) {
        if(actual -> llave == llave) {
            actual -> valor = valor;
            return;
        }
        actual = actual -> siguiente;
    }

    nodoHash *nuevo = malloc(sizeof(nodoHash));
    nuevo -> llave = llave;
    nuevo -> valor = valor;
    nuevo -> siguiente = tabla -> buckets[indice];
    tabla -> buckets[indice] = nuevo;
}


static void eliminarTH(tablaHash *tabla, int llave) {
    int indice = hashTH(tabla, llave);
    nodoHash* actual = tabla -> buckets[indice];
    nodoHash* anterior = NULL;

    while(actual) {
        if(actual -> llave == llave) {
            if(anterior) {
                anterior -> siguiente = actual -> siguiente;
            } else {
                tabla -> buckets[indice] = actual -> siguiente;
            }
            free(actual);
            return;
        }
        anterior = actual;
        actual = actual -> siguiente;
    }
}

/*
Estructuras LRU
*/

typedef struct nodoLRU {
    int pagina;
    struct nodoLRU *siguiente, *anterior;
} nodoLRU;

typedef struct cacheLRU {
    int capacidad, tamano;
    nodoLRU *cabeza, *cola;
    tablaHash *mapa;
} cacheLRU;

static cacheLRU* crearCacheLRU(int capacidad) {
    cacheLRU* cache = malloc(sizeof(cacheLRU));
    cache -> capacidad = capacidad;
    cache -> tamano = 0;
    cache -> cabeza = cache->cola = NULL;
    cache -> mapa = crearTablaHash((capacidad>0? capacidad : 1) * 4 + 3);
    return cache;
}

static void removerNodoLRU(cacheLRU *cache, nodoLRU *nodo) {
    if (!nodo) return;
    if (nodo->anterior) nodo->anterior->siguiente = nodo->siguiente;
    else cache->cabeza = nodo->siguiente;
    if (nodo->siguiente) nodo->siguiente->anterior = nodo-> anterior;
    else cache->cola = nodo->anterior;

    nodo->anterior = nodo->siguiente = NULL;   
}

static void insertarNodoLRU(cacheLRU *cache, nodoLRU *nodo) {
    nodo->siguiente = cache->cabeza;
    nodo->anterior = NULL;
    if (cache->cabeza) cache->cabeza->anterior = nodo;
    cache->cabeza = nodo;
    if (!cache->cola) cache->cola = nodo;
}

static void liberarNodoLRU(void *valor) {
    if (valor) {
        free((nodoLRU*)valor);
    }
}

static void destruirCacheLRU(cacheLRU *cache) {
    nodoLRU *actual = cache->cabeza;
    while(actual) {
        nodoLRU *temp = actual->siguiente;
        free(actual);
        actual = temp;
    }
    liberarTablaHash(cache->mapa, NULL);
    free(cache);
}

static int accederCacheLRU(cacheLRU *cache, int pagina) {
    nodoLRU *nodo = (nodoLRU*) obtenerTH(cache->mapa, pagina);
    if (nodo) {
        //movemos el nodo al frente de la lista
        removerNodoLRU(cache, nodo);
        insertarNodoLRU(cache, nodo);
        return 0;
    } else { //insertar nueva página
        nodoLRU *nuevo = malloc(sizeof(nodoLRU));
        nuevo->pagina = pagina;
        nuevo->siguiente = nuevo->anterior = NULL;
        
        if (cache->tamano < cache->capacidad) {
            insertarNodoLRU(cache, nuevo);
            insertarTH(cache->mapa, pagina, nuevo);
            cache->tamano++;
        }else {
            //eliminar la página menos recientemente usada (cola)
            if (!cache->cola) {
                free(nuevo);
                return 1;
            }

            int paginaEliminada = cache->cola->pagina;
            eliminarTH(cache->mapa, paginaEliminada);
            nodoLRU *colaVieja = cache->cola;
            removerNodoLRU(cache, colaVieja);
            free(colaVieja);
            insertarNodoLRU(cache, nuevo);
            insertarTH(cache->mapa, pagina, nuevo);
        }
        return 1;
    }
}

// Estructura para representar una página en el algoritmo del reloj
typedef struct paginaReloj {
    int pagina;               // Número de página virtual
    int bitReferencia;        // Bit de referencia (0 o 1)
    struct paginaReloj *siguiente; // Apuntador al siguiente nodo (estructura circular)
} paginaReloj;

// Estructura de la caché del reloj
typedef struct cacheReloj {
    int capacidad;            // Número de marcos físicos
    int tamano;               // Número actual de páginas en la caché
    paginaReloj *manecilla;   // Apuntador a la posición actual del reloj
    tablaHash *mapa;          // Tabla hash para acceso rápido
} cacheReloj;

// Crear la caché del reloj
static cacheReloj* crearCacheReloj(int capacidad) {
    cacheReloj* cache = malloc(sizeof(cacheReloj));
    cache->capacidad = capacidad;
    cache->tamano = 0;
    cache->manecilla = NULL;
    cache->mapa = crearTablaHash((capacidad > 0 ? capacidad : 1) * 4 + 3);
    return cache;
}

// Insertar una nueva página en la caché del reloj
static void insertarPaginaReloj(cacheReloj *cache, int pagina) {
    paginaReloj *nueva = malloc(sizeof(paginaReloj));
    nueva->pagina = pagina;
    nueva->bitReferencia = 1;
    nueva->siguiente = NULL;

    if (!cache->manecilla) {
        // Primer elemento, se apunta a sí mismo
        nueva->siguiente = nueva;
        cache->manecilla = nueva;
    } else {
        // Insertar después de la manecilla
        nueva->siguiente = cache->manecilla->siguiente;
        cache->manecilla->siguiente = nueva;
    }

    insertarTH(cache->mapa, pagina, nueva);
    cache->tamano++;
}

// Acceder a una página en la caché del reloj
static int accederCacheReloj(cacheReloj *cache, int pagina) {
    paginaReloj *nodo = (paginaReloj*) obtenerTH(cache->mapa, pagina);
    if (nodo) {
        // Página ya está en memoria, se marca el bit de referencia
        nodo->bitReferencia = 1;
        return 0; // No hubo fallo de página
    }

    // Fallo de página
    if (cache->tamano < cache->capacidad) {
        insertarPaginaReloj(cache, pagina);
        return 1;
    }

    // Reemplazo de página usando el algoritmo del reloj
    while (1) {
        if (cache->manecilla->bitReferencia == 0) {
            // Página candidata para reemplazo
            int paginaEliminada = cache->manecilla->pagina;
            paginaReloj *reemplazo = cache->manecilla;

            // Eliminar de la tabla hash
            eliminarTH(cache->mapa, paginaEliminada);

            // Reemplazar contenido
            reemplazo->pagina = pagina;
            reemplazo->bitReferencia = 1;
            insertarTH(cache->mapa, pagina, reemplazo);

            // Avanzar manecilla
            cache->manecilla = cache->manecilla->siguiente;
            return 1;
        } else {
            // Dar segunda oportunidad
            cache->manecilla->bitReferencia = 0;
            cache->manecilla = cache->manecilla->siguiente;
        }
    }
}

// Liberar la memoria de la caché del reloj
static void destruirCacheReloj(cacheReloj *cache) {
    if (!cache->manecilla) {
        liberarTablaHash(cache->mapa, NULL);
        free(cache);
        return;
    }

    paginaReloj *inicio = cache->manecilla;
    paginaReloj *actual = inicio->siguiente;

    while (actual != inicio) {
        paginaReloj *temp = actual->siguiente;
        free(actual);
        actual = temp;
    }

    free(inicio);
    liberarTablaHash(cache->mapa, NULL);
    free(cache);
}

void simular(int referencias[], int cantidad, int marcos) {
    // Crear cachés para ambos algoritmos
    cacheLRU *lru = crearCacheLRU(marcos);
    cacheReloj *reloj = crearCacheReloj(marcos);

    int fallosLRU = 0;
    int fallosReloj = 0;

    for (int i = 0; i < cantidad; i++) {
        int pagina = referencias[i];

        // Acceder a la caché LRU
        fallosLRU += accederCacheLRU(lru, pagina);

        // Acceder a la caché Reloj
        fallosReloj += accederCacheReloj(reloj, pagina);
    }

    // Mostrar resultados
    printf("\n--- RESULTADOS DE LA SIMULACIÓN ---\n");
    printf("Total de referencias: %d\n", cantidad);
    printf("Número de marcos físicos: %d\n", marcos);
    printf("Fallos de página con LRU: %d\n", fallosLRU);
    printf("Fallos de página con Reloj: %d\n", fallosReloj);

    // Liberar memoria
    destruirCacheLRU(lru);
    destruirCacheReloj(reloj);
}

void leer_referencias(int referencias[], int *cantidad) {
    printf("Ingrese la secuencia de referencias (terminar con -1):\n");
    int ref;
    *cantidad = 0;
    while (scanf("%d", &ref) && ref != -1) {
        referencias[(*cantidad)++] = ref;
    }
}


int main(int argc, char **argv)
{
    
    int referencias[MAX_REFERENCIAS];
    int cantidad;
    int marcos;

    leer_referencias(referencias, &cantidad);

    printf("Ingrese el número de marcos físicos:\n");
    scanf("%d", &marcos);

    simular(referencias, cantidad, marcos);

    return 0;

}
