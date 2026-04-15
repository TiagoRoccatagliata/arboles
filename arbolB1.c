#include <stdio.h>
#include <stdlib.h>

#define ORDEN 4  
typedef struct nodo_b_malo {
    int claves[ORDEN - 1];
    struct nodo_b_malo *hijos[ORDEN];
    int cant_claves;
    int es_hoja;
    //  No hay puntero al padre.
} nodo_b_malo;

nodo_b_malo* crear_nodo(int es_hoja) {
    nodo_b_malo *nuevo = (nodo_b_malo*)malloc(sizeof(nodo_b_malo));
    nuevo->cant_claves = 0;
    nuevo->es_hoja = es_hoja;
    for (int i = 0; i < ORDEN; i++) {
        nuevo->hijos[i] = NULL;
    }
    return nuevo;
}

void insertar_ingenuo(nodo_b_malo *raiz, int valor) {
    nodo_b_malo *actual = raiz;

    while (actual->es_hoja == 0) {
        int i = 0;
        while (i < actual->cant_claves && valor > actual->claves[i]) {
            i++;
        }
        actual = actual->hijos[i];
    }

    if (actual->cant_claves < ORDEN - 1) {
        actual->claves[actual->cant_claves] = valor;
        actual->cant_claves++;
        printf("Valor %d insertado sin problemas.\n", valor);
    } else {        
        printf("Error fatal insertando %d: El nodo está lleno.\n", valor);
    }
}

int main() {
    nodo_b_malo *raiz = crear_nodo(1);

    insertar_ingenuo(raiz, 10);
    insertar_ingenuo(raiz, 20);
    insertar_ingenuo(raiz, 30);

    insertar_ingenuo(raiz, 40); 

    return 0;
}