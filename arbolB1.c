#include <stdio.h>
#include <stdlib.h>

#define ORDEN 4  // Máximo 3 claves por nodo, máximo 4 hijos

typedef struct nodo_b_malo {
    int claves[ORDEN - 1];
    struct nodo_b_malo *hijos[ORDEN];
    int cant_claves;
    int es_hoja;
    // PRIMER GRAVE ERROR: No hay puntero al padre.
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

    // 1. Buscamos la hoja correspondiente bajando por el árbol
    while (actual->es_hoja == 0) {
        int i = 0;
        // Buscamos por qué hijo bajar
        while (i < actual->cant_claves && valor > actual->claves[i]) {
            i++;
        }
        actual = actual->hijos[i];
    }

    // 2. Llegamos a la hoja. ¿Hay espacio en el array?
    if (actual->cant_claves < ORDEN - 1) {
        // Genial, hay espacio. Lo metemos y "ordenamos" (simplificado)
        actual->claves[actual->cant_claves] = valor;
        actual->cant_claves++;
        printf("Valor %d insertado sin problemas.\n", valor);
        // (Nota: faltaría hacer un shift para mantener el array ordenado, 
        // pero supongamos que funciona).
    } else {
        // ----------------------------------------------------
        // ¡BAM! CHOQUE CONTRA LA PARED.
        // ----------------------------------------------------
        // El nodo hoja está lleno (ya tiene 3 claves). 
        // La teoría dice: "Partir el nodo a la mitad, y promocionar 
        // (subir) la clave del medio al nodo padre".
        
        printf("Error fatal insertando %d: El nodo está lleno.\n", valor);
        
        // PROBLEMA 1: ¡No tengo puntero al padre! ¿Cómo sé a dónde subir la clave?
        // PROBLEMA 2: Si uso recursividad para no necesitar puntero al padre... 
        // ¿Qué pasa si el padre TAMBIÉN está lleno? 
        // PROBLEMA 3: ¿Qué pasa si el que se llenó es la mismísima RAÍZ? 
        // ¿Cómo hago que el árbol crezca hacia arriba?
    }
}

int main() {
    // Creamos una raíz que también es hoja
    nodo_b_malo *raiz = crear_nodo(1);

    // Llenamos el nodo
    insertar_ingenuo(raiz, 10);
    insertar_ingenuo(raiz, 20);
    insertar_ingenuo(raiz, 30);

    // Se rompe todo
    insertar_ingenuo(raiz, 40); 

    return 0;
}