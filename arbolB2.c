#include <stdio.h>
#include <stdlib.h>

#define ORDEN 4  

typedef struct nodo_b {
    int claves[ORDEN - 1];
    int borrado[ORDEN - 1]; 
    struct nodo_b *hijos[ORDEN];
    int cant_claves;
    int es_hoja;
} nodo_b;

nodo_b* crear_nodo(int es_hoja) {
    nodo_b *nuevo = (nodo_b*)malloc(sizeof(nodo_b));
    nuevo->cant_claves = 0;
    nuevo->es_hoja = es_hoja;
    for (int i = 0; i < ORDEN; i++) nuevo->hijos[i] = NULL;
    for (int i = 0; i < ORDEN - 1; i++) nuevo->borrado[i] = 0;
    return nuevo;
}

int buscar(nodo_b *raiz, int valor) {
    if (!raiz) return 0;
    int i = 0;
    while (i < raiz->cant_claves && valor > raiz->claves[i]) i++;
    
    if (i < raiz->cant_claves && valor == raiz->claves[i]) {
        return !raiz->borrado[i]; 
    }
    
    if (raiz->es_hoja) return 0;
    return buscar(raiz->hijos[i], valor);
}

void acceso_secuencial(nodo_b *raiz) {
    if (!raiz) return;
    int i;
    for (i = 0; i < raiz->cant_claves; i++) {
        if (!raiz->es_hoja) acceso_secuencial(raiz->hijos[i]);
        if (!raiz->borrado[i]) printf("%d ", raiz->claves[i]); 
    }
    if (!raiz->es_hoja) acceso_secuencial(raiz->hijos[i]);
}

void eliminar_perezoso(nodo_b *raiz, int valor) {
    if (!raiz) return;
    int i = 0;
    while (i < raiz->cant_claves && valor > raiz->claves[i]) i++;
    
    if (i < raiz->cant_claves && valor == raiz->claves[i]) {
        raiz->borrado[i] = 1;
        return;
    }
    if (!raiz->es_hoja) eliminar_perezoso(raiz->hijos[i], valor);
}

void dividir_hijo(nodo_b *padre, int i, nodo_b *hijo_lleno) {
    nodo_b *nuevo_nodo = crear_nodo(hijo_lleno->es_hoja);
    int mid = (ORDEN - 1) / 2; 
    
    nuevo_nodo->cant_claves = ORDEN - 1 - mid - 1; 
    
    for (int j = 0; j < nuevo_nodo->cant_claves; j++) {
        nuevo_nodo->claves[j] = hijo_lleno->claves[mid + 1 + j];
        nuevo_nodo->borrado[j] = hijo_lleno->borrado[mid + 1 + j];
    }
    
    if (!hijo_lleno->es_hoja) {
        for (int j = 0; j <= nuevo_nodo->cant_claves; j++) {
            nuevo_nodo->hijos[j] = hijo_lleno->hijos[mid + 1 + j];
        }
    }
    
    hijo_lleno->cant_claves = mid;
    
    for (int j = padre->cant_claves; j >= i + 1; j--) padre->hijos[j + 1] = padre->hijos[j];
    padre->hijos[i + 1] = nuevo_nodo;
    
    for (int j = padre->cant_claves - 1; j >= i; j--) {
        padre->claves[j + 1] = padre->claves[j];
        padre->borrado[j + 1] = padre->borrado[j];
    }
    
    padre->claves[i] = hijo_lleno->claves[mid];
    padre->borrado[i] = hijo_lleno->borrado[mid];
    padre->cant_claves++;
}

void insertar_no_lleno(nodo_b *nodo, int valor) {
    int i = nodo->cant_claves - 1;
    if (nodo->es_hoja) {
        while (i >= 0 && valor < nodo->claves[i]) {
            nodo->claves[i + 1] = nodo->claves[i];
            nodo->borrado[i + 1] = nodo->borrado[i];
            i--;
        }
        nodo->claves[i + 1] = valor;
        nodo->borrado[i + 1] = 0;
        nodo->cant_claves++;
    } else {
        while (i >= 0 && valor < nodo->claves[i]) i--;
        i++;
        if (nodo->hijos[i]->cant_claves == ORDEN - 1) {
            dividir_hijo(nodo, i, nodo->hijos[i]);
            if (valor > nodo->claves[i]) i++;
        }
        insertar_no_lleno(nodo->hijos[i], valor);
    }
}

nodo_b* insertar(nodo_b *raiz, int valor) {
    if (raiz->cant_claves == ORDEN - 1) {
        nodo_b *nueva_raiz = crear_nodo(0);
        nueva_raiz->hijos[0] = raiz;
        dividir_hijo(nueva_raiz, 0, raiz);
        insertar_no_lleno(nueva_raiz, valor);
        return nueva_raiz;
    } else {
        insertar_no_lleno(raiz, valor);
        return raiz;
    }
}

int main() {
    nodo_b *raiz = crear_nodo(1);

    raiz = insertar(raiz, 10);
    raiz = insertar(raiz, 20);
    raiz = insertar(raiz, 30);
    raiz = insertar(raiz, 40); 
    raiz = insertar(raiz, 50);

    printf("Acceso secuencial inicial:\n");
    acceso_secuencial(raiz); 
    printf("\n");

    printf("Busqueda de 30: %d\n", buscar(raiz, 30)); 

    eliminar_perezoso(raiz, 30);
    
    printf("Busqueda de 30 tras borrado perezoso: %d\n", buscar(raiz, 30)); 
    
    printf("Acceso secuencial tras borrado:\n");
    acceso_secuencial(raiz); 
    printf("\n");

    return 0;
}