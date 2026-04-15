#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// https://www.youtube.com/watch?v=7Bg9PQ33xSo
// arbol n-ario:
//  no existe concepto de arbol vacio
//  maximo n hijos por nodo.
//  hoja: nodo sin subarboles.

// No podemos modelarlo como un arbol binario con nodo izq y der.
// Vamos a modelar sus nodo como una lista.

typedef struct nodo_arbol arbol;
typedef struct nodo_lista lista;

struct nodo_lista {
    // int valor; MAl: los hijos no son numeros, son arboles enteros.
    arbol *subarbol; // La lista guarda punteros a otros arboles.
    lista *sig;
};

struct nodo_arbol {
    int dato;
    lista *hijos;
};

// crear el arbol
arbol* crear_arbol(int dato) {
    arbol *nuevo = malloc(sizeof(arbol));
    nuevo->dato = dato;
    nuevo->hijos = NULL; // lista de sub arboles vacia, es hijo
    return nuevo;
}

void agregar_hijo(arbol *padre, arbol *hijo){
    lista *nuevo_nodo_lista = malloc(sizeof(lista));
    nuevo_nodo_lista->subarbol = hijo;

    nuevo_nodo_lista->sig = padre->hijos;
    padre->hijos = nuevo_nodo_lista;
}

int main(){

    arbol *raiz = crear_arbol(10);

    arbol *hijo1 = crear_arbol(5);
    arbol *hijo2 = crear_arbol(8);
    arbol *hijo3 = crear_arbol(3);

    agregar_hijo(raiz, hijo1);
    agregar_hijo(raiz, hijo2);
    agregar_hijo(raiz, hijo3);

    printf("El contenido de la raiz es: %d\n", raiz->dato);

    printf("El primer hijo en la lista de la raiz es: %d\n", raiz->hijos->subarbol->dato);
    
    return 0;
};

// Liberar memoria? free ? de nietos a raiz?

// Problema.
// hermano de la derecha: imposible de saber. solo hay data y la lista.
// Padre de un nodo: desde una hoja no puedo ver hacia arriba. puntero arbol *padre
// recorrido inorde -> izquierda-> Raiz -> derecho.