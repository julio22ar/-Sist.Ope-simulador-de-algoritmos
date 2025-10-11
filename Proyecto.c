#include <stdio.h>
#include <stdlib.h>

#define SIZEHASH 100

typedef struct nodoLRU {
    int pagina;
    struct nodoLRU *anterior;
    struct nodoLRU *siguiente;
} nodoLRU;

nodoLRU* tablaHash[SIZEHASH];
nodoLRU *inicio = NULL;
nodoLRU *final = NULL;

nodoLRU* crearNodo(int pagina) {
    nodoLRU* nuevo = (nodoLRU*)malloc(sizeof(nodoLRU));
    nuevo -> pagina = pagina;
    nuevo -> anterior = nuevo -> siguiente = NULL;
    return nuevo;
}

void insertarNodoPrimeraPosicion(nodoLRU* nodoAux) {
    if(nodoAux == inicio) {
        return;
    } 
    if(nodoAux -> anterior) {
       nodoAux -> anterior -> siguiente = nodoAux -> siguiente;
    }
    if(nodoAux -> siguiente) {
        nodoAux -> siguiente -> anterior = nodoAux -> anterior;
    }
    if(nodoAux == final) {
        final = nodoAux -> anterior;
    } else {
        nodoAux -> siguiente = inicio;
        nodoAux -> anterior = NULL;
        
        if(inicio) {
            inicio -> anterior = nodoAux;
        }
        
        inicio = nodoAux;
        
        if(!final) {
            final = nodoAux;
        }
    }
}

void agregarNuevaPagina(int pagina, int marco) {
    if(tablaHash[pagina]) {
        insertarNodoPrimeraPosicion(tablaHash[pagina]);
        return;
    } else {
        nodoLRU* nuevo = crearNodo(pagina);
        tablaHash[pagina] = nuevo;
        nuevo -> siguiente = inicio;
        
        if(inicio) {
            inicio -> anterior = nuevo;
            inicio = nuevo;
        }
        
        if(!final) {
            final = nuevo;
        }
        
        int contador = 0;
        nodoLRU* temp = inicio;
        
        while(temp) {
            contador++;
            temp = temp->siguiente;
        }
        
        if(contador > marco) {
        tablaHash[final -> pagina] = NULL;
        nodoLRU* eliminar = final;
        final = final -> anterior;
        
        if(final) final -> siguiente = NULL;
            free(eliminar);
        }
    }
}

void mostrarSecuencias(int referencia[], int *cantidad) {
    printf("Secuencia de referencias:\n ");
    for(int i = 0; i < cantidad; i++) {
        printf("%d", referencias[i]);
    }
    printf("\n");
}

int main()
{
    print("Hola");
    return 0;
}
