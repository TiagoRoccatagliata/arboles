#include <stdio.h>
#include <stdlib.h>

#define ORDEN 4  // Máximo 3 claves, 4 hijos. Mínimo 1 clave.

typedef struct nodo_b {
    int claves[ORDEN - 1];
    int borrado[ORDEN - 1]; // 1 si está borrado (eliminación perezosa), 0 si es válido
    struct nodo_b *hijos[ORDEN];
    int cant_claves;
    int es_hoja;
} nodo_b;

// Construcción inicial
nodo_b* crear_nodo(int es_hoja) {
    nodo_b *nuevo = (nodo_b*)malloc(sizeof(nodo_b));
    nuevo->cant_claves = 0;
    nuevo->es_hoja = es_hoja;
    for (int i = 0; i < ORDEN; i++) nuevo->hijos[i] = NULL;
    for (int i = 0; i < ORDEN - 1; i++) nuevo->borrado[i] = 0;
    return nuevo;
}

// Búsqueda de un elemento
int buscar(nodo_b *raiz, int valor) {
    if (!raiz) return 0;
    int i = 0;
    while (i < raiz->cant_claves && valor > raiz->claves[i]) i++;
    
    if (i < raiz->cant_claves && valor == raiz->claves[i]) {
        return !raiz->borrado[i]; // Retorna 1 si existe y NO está borrado perezosamente
    }
    
    if (raiz->es_hoja) return 0;
    return buscar(raiz->hijos[i], valor);
}

// Acceso secuencial (Recorrido In-Order)
void acceso_secuencial(nodo_b *raiz) {
    if (!raiz) return;
    int i;
    for (i = 0; i < raiz->cant_claves; i++) {
        if (!raiz->es_hoja) acceso_secuencial(raiz->hijos[i]);
        if (!raiz->borrado[i]) printf("%d ", raiz->claves[i]); // Solo imprime los no borrados
    }
    if (!raiz->es_hoja) acceso_secuencial(raiz->hijos[i]);
}

// Eliminación (Perezosa - Minimalista)
void eliminar_perezoso(nodo_b *raiz, int valor) {
    if (!raiz) return;
    int i = 0;
    while (i < raiz->cant_claves && valor > raiz->claves[i]) i++;
    
    if (i < raiz->cant_claves && valor == raiz->claves[i]) {
        raiz->borrado[i] = 1; // Marcamos como eliminado sin reestructurar
        return;
    }
    if (!raiz->es_hoja) eliminar_perezoso(raiz->hijos[i], valor);
}

// Funciones de Inserción (Mecanismo de Reestructuración preventivo)
void dividir_hijo(nodo_b *padre, int i, nodo_b *hijo_lleno) {
    nodo_b *nuevo_nodo = crear_nodo(hijo_lleno->es_hoja);
    int mid = (ORDEN - 1) / 2; // Índice medio (1 para ORDEN 4)
    
    nuevo_nodo->cant_claves = ORDEN - 1 - mid - 1; 
    
    // Pasar las claves del final al nuevo nodo
    for (int j = 0; j < nuevo_nodo->cant_claves; j++) {
        nuevo_nodo->claves[j] = hijo_lleno->claves[mid + 1 + j];
        nuevo_nodo->borrado[j] = hijo_lleno->borrado[mid + 1 + j];
    }
    
    // Pasar los hijos si no es hoja
    if (!hijo_lleno->es_hoja) {
        for (int j = 0; j <= nuevo_nodo->cant_claves; j++) {
            nuevo_nodo->hijos[j] = hijo_lleno->hijos[mid + 1 + j];
        }
    }
    
    hijo_lleno->cant_claves = mid;
    
    // Hacer espacio en el padre y subir la clave
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
        // La raíz está llena, el árbol crece en altura
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

    // Inserción de varios elementos (resolviendo el crash de tu código)
    raiz = insertar(raiz, 10);
    raiz = insertar(raiz, 20);
    raiz = insertar(raiz, 30);
    raiz = insertar(raiz, 40); // Aquí antes fallaba, ahora parte la raíz.
    raiz = insertar(raiz, 50);

    printf("Acceso secuencial inicial:\n");
    acceso_secuencial(raiz); // Salida: 10 20 30 40 50
    printf("\n");

    printf("Busqueda de 30: %d\n", buscar(raiz, 30)); // 1 (Verdadero)

    eliminar_perezoso(raiz, 30);
    
    printf("Busqueda de 30 tras borrado perezoso: %d\n", buscar(raiz, 30)); // 0 (Falso)
    
    printf("Acceso secuencial tras borrado:\n");
    acceso_secuencial(raiz); // Salida: 10 20 40 50
    printf("\n");

    return 0;
}