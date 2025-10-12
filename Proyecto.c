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
    tablaHash* tabla = (tablaHash*)malloc(sizeof(tablaHash));
    tabla -> nbuckets = nbuckets;
    tabla -> buckets = (nodoHash**)malloc(sizeof(nodoHash*) * nbuckets);
    memset(tabla -> buckets, 0, sizeof(nodoHash*) * nbuckets);
    return tabla;
}

/*
Esta función libera memoria, iterando en cada cubeta primeramente guardando el nodo siguiente antes
de liberar el nodo actual, Si el usuario pasó una función liberarValor y actual->valor no es NULL,
llama a liberarValor(actual->valor) para liberar la memoria del valor.
Luego se libera el array de cubetas y finalmente la estructura de la tabla hash.
*/

static liberarTablaHash(tablaHash* tabla, void (*liberarValor)(void *)) {
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

/*
Esta función elimina un nodo de la tabla hash basado en la llave proporcionada.
Primero calcula el índice de la cubeta usando la función hash.
Luego recorre la lista enlazada en esa cubeta para encontrar el nodo con la llave correspondiente.
Si encuentra el nodo, lo elimina de la lista enlazada y libera su memoria.
Si no encuentra la llave, simplemente retorna sin hacer nada.
*/

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

int main(int argc, char **argv)
{
    printf("Bueno almenos ya esta una parte xd");
    return 0;
}
