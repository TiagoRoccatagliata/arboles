/*
 * ============================================================================
 *  IMPLEMENTACIÓN DE ÁRBOLES B, B+ Y COMPARACIÓN CON AVL
 * ============================================================================
 *  Trabajo Práctico — Estructuras de Datos Avanzadas
 *
 *  Estructuras implementadas:
 *    1. Árbol-B   (orden configurable, por defecto 4)
 *    2. Árbol B+  (orden configurable, por defecto 4)
 *    3. Árbol AVL (para comparación)
 *
 *  Funcionalidades:
 *    - Búsqueda de un elemento
 *    - Acceso secuencial (in-order)
 *    - Construcción inicial (inserción masiva desde array ordenado)
 *    - Inserción estándar con reestructuración (split)
 *    - Eliminación estándar y perezosa (lazy delete)
 *    - Comparación de rendimiento con AVL
 *
 *  Compilar: gcc -o arboles arboles.c -lm
 *  Ejecutar: ./arboles
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  SECCIÓN 0: CONFIGURACIÓN GLOBAL
 * ═══════════════════════════════════════════════════════════════════════════ */

#define ORDEN_B   4   /* Árbol-B: max 3 claves, max 4 hijos por nodo      */
#define ORDEN_BP  4   /* Árbol B+: max 3 claves, max 4 hijos por nodo     */
#define MIN_CLAVES_B  ((ORDEN_B + 1) / 2 - 1)   /* Mínimo claves no-raíz */
#define MIN_CLAVES_BP ((ORDEN_BP + 1) / 2 - 1)

/* Colores para la terminal */
#define RESET   "\033[0m"
#define ROJO    "\033[1;31m"
#define VERDE   "\033[1;32m"
#define AMARILLO "\033[1;33m"
#define CYAN    "\033[1;36m"
#define NEGRITA "\033[1m"

/* Contador global de operaciones para comparaciones */
static long g_comparaciones = 0;
static long g_splits = 0;
static long g_rotaciones = 0;

void resetear_contadores(void) {
    g_comparaciones = 0;
    g_splits = 0;
    g_rotaciones = 0;
}


/* ╔═════════════════════════════════════════════════════════════════════════╗
 * ║                                                                       ║
 * ║                    SECCIÓN 1: ÁRBOL-B (B-TREE)                        ║
 * ║                                                                       ║
 * ╚═════════════════════════════════════════════════════════════════════════╝
 *
 * Propiedades (orden m = ORDEN_B = 4):
 *   - Cada nodo interno tiene entre ⌈m/2⌉ = 2 y m = 4 hijos.
 *   - Cada nodo (excepto raíz) tiene entre ⌈m/2⌉-1 = 1 y m-1 = 3 claves.
 *   - La raíz tiene al menos 1 clave (si no está vacía).
 *   - Todas las hojas están al mismo nivel.
 *   - Las claves en cada nodo están ordenadas.
 *   - Las claves almacenan los datos directamente (a diferencia de B+).
 */

typedef struct NodoB {
    int claves[ORDEN_B - 1];       /* Array de claves (max 3)              */
    struct NodoB *hijos[ORDEN_B];  /* Array de punteros a hijos (max 4)    */
    int n;                         /* Cantidad actual de claves            */
    int es_hoja;                   /* 1 si es hoja, 0 si es interno       */
    int eliminado[ORDEN_B - 1];    /* Marcas para eliminación perezosa     */
} NodoB;


/* --- Crear un nodo vacío --- */
NodoB* b_crear_nodo(int es_hoja) {
    NodoB *nuevo = (NodoB*)malloc(sizeof(NodoB));
    if (!nuevo) { perror("malloc"); exit(EXIT_FAILURE); }
    nuevo->n = 0;
    nuevo->es_hoja = es_hoja;
    for (int i = 0; i < ORDEN_B; i++)
        nuevo->hijos[i] = NULL;
    for (int i = 0; i < ORDEN_B - 1; i++)
        nuevo->eliminado[i] = 0;
    return nuevo;
}


/* --- Búsqueda ---
 * Retorna el nodo donde se encontró y escribe el índice en *idx.
 * Si no existe, retorna NULL.
 *
 * Complejidad: O(log_m n) niveles × O(m) por nivel = O(m · log_m n)
 * Con m constante pequeño, es efectivamente O(log n).
 */
NodoB* b_buscar(NodoB *raiz, int valor, int *idx) {
    if (!raiz) return NULL;

    int i = 0;
    /* Búsqueda lineal dentro del nodo (para m pequeño es eficiente) */
    while (i < raiz->n && valor > raiz->claves[i]) {
        g_comparaciones++;
        i++;
    }
    if (i < raiz->n) g_comparaciones++;  /* Comparación final */

    /* ¿Encontramos la clave? */
    if (i < raiz->n && valor == raiz->claves[i]) {
        if (raiz->eliminado[i]) {
            /* Está marcado como eliminado (lazy delete) */
            *idx = i;
            return NULL;
        }
        *idx = i;
        return raiz;
    }

    /* Si es hoja, no está en el árbol */
    if (raiz->es_hoja)
        return NULL;

    /* Bajar al hijo correspondiente */
    return b_buscar(raiz->hijos[i], valor, idx);
}


/* --- Split de un hijo lleno ---
 *
 * Esta es LA operación fundamental del Árbol-B.
 * Cuando un hijo está lleno (tiene ORDEN_B-1 = 3 claves), lo partimos:
 *
 *  ANTES:   padre: [..., clave_i, ...]
 *                       |
 *              hijo: [A, B, C]   (lleno, 3 claves)
 *
 *  DESPUÉS:  padre: [..., clave_i, B, ...]   (B sube al padre)
 *                       |          |
 *              hijo: [A]    nuevo: [C]
 *
 * La clave mediana (B) "promociona" al padre, y el hijo se parte en dos.
 */
void b_split_hijo(NodoB *padre, int i) {
    g_splits++;

    NodoB *hijo = padre->hijos[i];
    int mediana = ORDEN_B / 2 - 1;  /* Índice de la mediana (para orden 4: índice 1) */

    /* Crear el nuevo nodo que contendrá la mitad derecha */
    NodoB *nuevo = b_crear_nodo(hijo->es_hoja);
    nuevo->n = hijo->n - mediana - 1;

    /* Copiar las claves de la mitad derecha al nuevo nodo */
    for (int j = 0; j < nuevo->n; j++) {
        nuevo->claves[j] = hijo->claves[mediana + 1 + j];
        nuevo->eliminado[j] = hijo->eliminado[mediana + 1 + j];
    }

    /* Si no es hoja, copiar también los hijos */
    if (!hijo->es_hoja) {
        for (int j = 0; j <= nuevo->n; j++)
            nuevo->hijos[j] = hijo->hijos[mediana + 1 + j];
    }

    hijo->n = mediana;  /* El hijo original se queda con la mitad izquierda */

    /* Hacer espacio en el padre para la clave que sube */
    for (int j = padre->n; j > i; j--) {
        padre->hijos[j + 1] = padre->hijos[j];
    }
    padre->hijos[i + 1] = nuevo;

    for (int j = padre->n - 1; j >= i; j--) {
        padre->claves[j + 1] = padre->claves[j];
        padre->eliminado[j + 1] = padre->eliminado[j];
    }
    padre->claves[i] = hijo->claves[mediana];
    padre->eliminado[i] = hijo->eliminado[mediana];
    padre->n++;
}


/* --- Inserción (no-lleno) ---
 * Precondición: el nodo 'nodo' NO está lleno.
 * Insertamos de forma recursiva, haciendo split preventivo
 * de cualquier hijo lleno ANTES de bajar (enfoque top-down).
 */
void b_insertar_no_lleno(NodoB *nodo, int valor) {
    int i = nodo->n - 1;

    if (nodo->es_hoja) {
        /* Desplazar claves mayores hacia la derecha para hacer hueco */
        while (i >= 0 && valor < nodo->claves[i]) {
            nodo->claves[i + 1] = nodo->claves[i];
            nodo->eliminado[i + 1] = nodo->eliminado[i];
            i--;
        }
        nodo->claves[i + 1] = valor;
        nodo->eliminado[i + 1] = 0;
        nodo->n++;
    } else {
        /* Encontrar el hijo por el que bajar */
        while (i >= 0 && valor < nodo->claves[i])
            i--;
        i++;

        /* Si el hijo está lleno, partirlo ANTES de bajar */
        if (nodo->hijos[i]->n == ORDEN_B - 1) {
            b_split_hijo(nodo, i);
            /* Después del split, decidir a cuál de los dos hijos ir */
            if (valor > nodo->claves[i])
                i++;
        }
        b_insertar_no_lleno(nodo->hijos[i], valor);
    }
}


/* --- Inserción pública ---
 * Caso especial: si la raíz está llena, el árbol crece hacia ARRIBA.
 * Esta es la magia del Árbol-B: la altura solo aumenta en la raíz.
 */
NodoB* b_insertar(NodoB *raiz, int valor) {
    if (!raiz) {
        raiz = b_crear_nodo(1);
        raiz->claves[0] = valor;
        raiz->eliminado[0] = 0;
        raiz->n = 1;
        return raiz;
    }

    /* Verificar si ya existe (y no está eliminado) */
    int idx;
    NodoB *encontrado = b_buscar(raiz, valor, &idx);
    if (encontrado) {
        return raiz;  /* Duplicado: no insertar */
    }

    if (raiz->n == ORDEN_B - 1) {
        /* ¡La raíz está llena! Crear nueva raíz */
        NodoB *nueva_raiz = b_crear_nodo(0);
        nueva_raiz->hijos[0] = raiz;
        b_split_hijo(nueva_raiz, 0);
        /* Ahora insertar en el subárbol correcto */
        int i = 0;
        if (valor > nueva_raiz->claves[0])
            i = 1;
        b_insertar_no_lleno(nueva_raiz->hijos[i], valor);
        return nueva_raiz;
    } else {
        b_insertar_no_lleno(raiz, valor);
        return raiz;
    }
}


/* --- Eliminación perezosa (Lazy Delete) ---
 * En vez de reestructurar el árbol, simplemente marcamos la clave.
 * Ventajas: O(log n) sin merges ni redistribuciones.
 * Desventajas: el árbol no se encoge, ocupa más memoria.
 */
int b_eliminar_perezosa(NodoB *raiz, int valor) {
    int idx;
    NodoB *nodo = b_buscar(raiz, valor, &idx);
    if (!nodo) return 0;  /* No encontrado o ya eliminado */

    nodo->eliminado[idx] = 1;
    return 1;
}


/* --- Eliminación estándar del Árbol-B ---
 *
 * Es la operación más compleja. Hay tres casos principales:
 *
 * CASO 1: La clave está en una hoja → simplemente borrarla y compactar.
 *         Si queda con menos del mínimo, reestructurar.
 *
 * CASO 2: La clave está en un nodo interno → reemplazarla con el
 *         predecesor in-order (máximo del subárbol izquierdo) o
 *         sucesor in-order (mínimo del subárbol derecho), y luego
 *         eliminar ese predecesor/sucesor recursivamente.
 *
 * CASO 3: Al bajar, si un hijo tiene el mínimo de claves, primero
 *         asegurar que tenga al menos MIN+1 (robar de hermano o fusionar).
 */

/* Encontrar el predecesor in-order (la mayor clave en el subárbol izq.) */
int b_predecesor(NodoB *nodo) {
    while (!nodo->es_hoja)
        nodo = nodo->hijos[nodo->n];
    return nodo->claves[nodo->n - 1];
}

/* Encontrar el sucesor in-order (la menor clave en el subárbol der.) */
int b_sucesor(NodoB *nodo) {
    while (!nodo->es_hoja)
        nodo = nodo->hijos[0];
    return nodo->claves[0];
}

/* Fusionar hijo[i] con hijo[i+1], bajando la clave padre[i] como pegamento */
void b_fusionar(NodoB *padre, int i) {
    NodoB *izq = padre->hijos[i];
    NodoB *der = padre->hijos[i + 1];

    /* Bajar la clave del padre al final del hijo izquierdo */
    izq->claves[izq->n] = padre->claves[i];
    izq->eliminado[izq->n] = padre->eliminado[i];

    /* Copiar claves del hijo derecho */
    for (int j = 0; j < der->n; j++) {
        izq->claves[izq->n + 1 + j] = der->claves[j];
        izq->eliminado[izq->n + 1 + j] = der->eliminado[j];
    }
    /* Copiar hijos del hijo derecho */
    if (!izq->es_hoja) {
        for (int j = 0; j <= der->n; j++)
            izq->hijos[izq->n + 1 + j] = der->hijos[j];
    }
    izq->n += der->n + 1;

    /* Eliminar la clave del padre y el puntero al hijo derecho */
    for (int j = i; j < padre->n - 1; j++) {
        padre->claves[j] = padre->claves[j + 1];
        padre->eliminado[j] = padre->eliminado[j + 1];
    }
    for (int j = i + 1; j < padre->n; j++)
        padre->hijos[j] = padre->hijos[j + 1];
    padre->n--;

    free(der);
    g_rotaciones++;
}

/* Asegurar que hijos[i] tenga al menos MIN+1 claves antes de bajar */
void b_asegurar_minimo(NodoB *padre, int i) {
    NodoB *hijo = padre->hijos[i];

    if (hijo->n >= MIN_CLAVES_B + 1)
        return;  /* Ya tiene suficientes */

    /* Intentar robar del hermano izquierdo */
    if (i > 0 && padre->hijos[i - 1]->n > MIN_CLAVES_B) {
        NodoB *hermano_izq = padre->hijos[i - 1];
        g_rotaciones++;

        /* Desplazar todo en hijo una posición a la derecha */
        for (int j = hijo->n; j > 0; j--) {
            hijo->claves[j] = hijo->claves[j - 1];
            hijo->eliminado[j] = hijo->eliminado[j - 1];
        }
        if (!hijo->es_hoja) {
            for (int j = hijo->n + 1; j > 0; j--)
                hijo->hijos[j] = hijo->hijos[j - 1];
        }

        /* Bajar clave del padre, subir última clave del hermano */
        hijo->claves[0] = padre->claves[i - 1];
        hijo->eliminado[0] = padre->eliminado[i - 1];
        if (!hijo->es_hoja)
            hijo->hijos[0] = hermano_izq->hijos[hermano_izq->n];
        padre->claves[i - 1] = hermano_izq->claves[hermano_izq->n - 1];
        padre->eliminado[i - 1] = hermano_izq->eliminado[hermano_izq->n - 1];

        hijo->n++;
        hermano_izq->n--;
    }
    /* Intentar robar del hermano derecho */
    else if (i < padre->n && padre->hijos[i + 1]->n > MIN_CLAVES_B) {
        NodoB *hermano_der = padre->hijos[i + 1];
        g_rotaciones++;

        /* Bajar clave del padre al final del hijo */
        hijo->claves[hijo->n] = padre->claves[i];
        hijo->eliminado[hijo->n] = padre->eliminado[i];
        if (!hijo->es_hoja)
            hijo->hijos[hijo->n + 1] = hermano_der->hijos[0];

        /* Subir primera clave del hermano al padre */
        padre->claves[i] = hermano_der->claves[0];
        padre->eliminado[i] = hermano_der->eliminado[0];

        /* Desplazar claves del hermano a la izquierda */
        for (int j = 0; j < hermano_der->n - 1; j++) {
            hermano_der->claves[j] = hermano_der->claves[j + 1];
            hermano_der->eliminado[j] = hermano_der->eliminado[j + 1];
        }
        if (!hermano_der->es_hoja) {
            for (int j = 0; j < hermano_der->n; j++)
                hermano_der->hijos[j] = hermano_der->hijos[j + 1];
        }

        hijo->n++;
        hermano_der->n--;
    }
    /* No se puede robar → fusionar */
    else {
        if (i < padre->n)
            b_fusionar(padre, i);
        else
            b_fusionar(padre, i - 1);
    }
}


/* Eliminación recursiva real */
void b_eliminar_rec(NodoB *nodo, int valor) {
    int i = 0;
    while (i < nodo->n && valor > nodo->claves[i])
        i++;

    if (i < nodo->n && valor == nodo->claves[i]) {
        /* Encontrada la clave en este nodo */
        if (nodo->es_hoja) {
            /* CASO 1: Clave en hoja → quitar y compactar */
            for (int j = i; j < nodo->n - 1; j++) {
                nodo->claves[j] = nodo->claves[j + 1];
                nodo->eliminado[j] = nodo->eliminado[j + 1];
            }
            nodo->n--;
        } else {
            /* CASO 2: Clave en nodo interno */
            if (nodo->hijos[i]->n > MIN_CLAVES_B) {
                /* Reemplazar con predecesor in-order */
                int pred = b_predecesor(nodo->hijos[i]);
                nodo->claves[i] = pred;
                nodo->eliminado[i] = 0;
                b_eliminar_rec(nodo->hijos[i], pred);
            } else if (nodo->hijos[i + 1]->n > MIN_CLAVES_B) {
                /* Reemplazar con sucesor in-order */
                int suc = b_sucesor(nodo->hijos[i + 1]);
                nodo->claves[i] = suc;
                nodo->eliminado[i] = 0;
                b_eliminar_rec(nodo->hijos[i + 1], suc);
            } else {
                /* Fusionar los dos hijos y bajar la clave */
                b_fusionar(nodo, i);
                b_eliminar_rec(nodo->hijos[i], valor);
            }
        }
    } else {
        /* La clave no está en este nodo; bajar al hijo correspondiente */
        if (nodo->es_hoja) return;  /* No existe */

        int ultimo = (i == nodo->n);
        b_asegurar_minimo(nodo, i);

        /* Después de asegurar, el índice puede haber cambiado si hubo fusión */
        if (ultimo && i > nodo->n)
            b_eliminar_rec(nodo->hijos[i - 1], valor);
        else
            b_eliminar_rec(nodo->hijos[i], valor);
    }
}


/* Eliminación pública (maneja el caso de la raíz) */
NodoB* b_eliminar(NodoB *raiz, int valor) {
    if (!raiz) return NULL;

    b_eliminar_rec(raiz, valor);

    /* Si la raíz quedó vacía pero tiene un hijo, ese hijo es la nueva raíz */
    if (raiz->n == 0 && !raiz->es_hoja) {
        NodoB *nueva_raiz = raiz->hijos[0];
        free(raiz);
        return nueva_raiz;
    }
    return raiz;
}


/* --- Recorrido In-Order (acceso secuencial) ---
 * Visita todas las claves en orden ascendente.
 * Complejidad: O(n) donde n es el número total de claves.
 */
void b_inorder(NodoB *nodo) {
    if (!nodo) return;
    for (int i = 0; i < nodo->n; i++) {
        if (!nodo->es_hoja)
            b_inorder(nodo->hijos[i]);
        if (!nodo->eliminado[i])
            printf("%d ", nodo->claves[i]);
    }
    if (!nodo->es_hoja)
        b_inorder(nodo->hijos[nodo->n]);
}

/* Variante que cuenta elementos */
int b_inorder_contar(NodoB *nodo) {
    if (!nodo) return 0;
    int total = 0;
    for (int i = 0; i < nodo->n; i++) {
        if (!nodo->es_hoja)
            total += b_inorder_contar(nodo->hijos[i]);
        if (!nodo->eliminado[i])
            total++;
    }
    if (!nodo->es_hoja)
        total += b_inorder_contar(nodo->hijos[nodo->n]);
    return total;
}


/* --- Imprimir estructura del árbol (para depuración) --- */
void b_imprimir(NodoB *nodo, int nivel) {
    if (!nodo) return;

    /* Indentación según el nivel */
    for (int i = 0; i < nivel; i++) printf("    ");

    printf("[");
    for (int i = 0; i < nodo->n; i++) {
        if (i > 0) printf(", ");
        if (nodo->eliminado[i])
            printf(ROJO "~%d~" RESET, nodo->claves[i]);  /* Marcado eliminado */
        else
            printf("%d", nodo->claves[i]);
    }
    printf("]");
    if (nodo->es_hoja) printf(" (hoja)");
    printf("\n");

    if (!nodo->es_hoja) {
        for (int i = 0; i <= nodo->n; i++)
            b_imprimir(nodo->hijos[i], nivel + 1);
    }
}

/* --- Altura del árbol --- */
int b_altura(NodoB *nodo) {
    if (!nodo) return 0;
    if (nodo->es_hoja) return 1;
    return 1 + b_altura(nodo->hijos[0]);
}

/* --- Liberar memoria --- */
void b_liberar(NodoB *nodo) {
    if (!nodo) return;
    if (!nodo->es_hoja) {
        for (int i = 0; i <= nodo->n; i++)
            b_liberar(nodo->hijos[i]);
    }
    free(nodo);
}


/* ╔═════════════════════════════════════════════════════════════════════════╗
 * ║                                                                       ║
 * ║                   SECCIÓN 2: ÁRBOL B+ (B-PLUS TREE)                   ║
 * ║                                                                       ║
 * ╚═════════════════════════════════════════════════════════════════════════╝
 *
 * Diferencias clave con el Árbol-B:
 *
 *   1. TODOS los datos residen en las HOJAS. Los nodos internos son
 *      puramente índices (solo contienen copias de claves para guiar
 *      la búsqueda).
 *
 *   2. Las hojas están enlazadas en una LISTA LIGADA, permitiendo
 *      acceso secuencial eficiente sin necesidad de recorrer el árbol.
 *
 *   3. Las hojas pueden almacenar más claves que los nodos internos
 *      (en esta implementación usamos el mismo orden para simplificar).
 *
 *   4. Al hacer split de una hoja, la clave mediana se COPIA al padre
 *      (no se mueve como en B-Tree), porque los datos permanecen abajo.
 *
 *            Árbol-B                    Árbol B+
 *         ┌─[10, 20]─┐             ┌──[10, 20]──┐
 *         │     │     │             │      │      │
 *        [5]  [15]  [25,30]      [5,8]→[10,15]→[20,25,30]
 *                                         ↑ datos en hojas + enlazadas
 */

typedef struct NodoBP {
    int claves[ORDEN_BP];             /* Una clave extra para overflow temporal */
    struct NodoBP *hijos[ORDEN_BP + 1];
    struct NodoBP *siguiente;          /* Puntero al siguiente nodo hoja      */
    int n;
    int es_hoja;
    int eliminado[ORDEN_BP];           /* Lazy delete para hojas              */
} NodoBP;


NodoBP* bp_crear_nodo(int es_hoja) {
    NodoBP *nuevo = (NodoBP*)malloc(sizeof(NodoBP));
    if (!nuevo) { perror("malloc"); exit(EXIT_FAILURE); }
    nuevo->n = 0;
    nuevo->es_hoja = es_hoja;
    nuevo->siguiente = NULL;
    for (int i = 0; i <= ORDEN_BP; i++)
        nuevo->hijos[i] = NULL;
    for (int i = 0; i < ORDEN_BP; i++)
        nuevo->eliminado[i] = 0;
    return nuevo;
}


/* --- Búsqueda en B+ ---
 * Siempre baja hasta una hoja, porque los datos solo están ahí.
 * Retorna el nodo hoja y el índice, o NULL si no existe.
 */
NodoBP* bp_buscar(NodoBP *raiz, int valor, int *idx) {
    if (!raiz) return NULL;

    NodoBP *actual = raiz;

    /* Bajar hasta una hoja */
    while (!actual->es_hoja) {
        int i = 0;
        while (i < actual->n && valor >= actual->claves[i]) {
            g_comparaciones++;
            i++;
        }
        actual = actual->hijos[i];
    }

    /* Buscar en la hoja */
    for (int i = 0; i < actual->n; i++) {
        g_comparaciones++;
        if (actual->claves[i] == valor) {
            if (actual->eliminado[i]) {
                *idx = i;
                return NULL;
            }
            *idx = i;
            return actual;
        }
    }
    return NULL;
}


/* --- Acceso secuencial en B+ ---
 * ¡Esta es la gran ventaja del B+!
 * Basta con ir a la hoja más a la izquierda y seguir los punteros 'siguiente'.
 * No hay que recorrer el árbol recursivamente.
 * Complejidad: O(n) con excelente localidad de caché.
 */
NodoBP* bp_primera_hoja(NodoBP *raiz) {
    if (!raiz) return NULL;
    NodoBP *actual = raiz;
    while (!actual->es_hoja)
        actual = actual->hijos[0];
    return actual;
}

void bp_recorrido_secuencial(NodoBP *raiz) {
    NodoBP *hoja = bp_primera_hoja(raiz);
    while (hoja) {
        for (int i = 0; i < hoja->n; i++) {
            if (!hoja->eliminado[i])
                printf("%d ", hoja->claves[i]);
        }
        hoja = hoja->siguiente;
    }
}

int bp_contar_elementos(NodoBP *raiz) {
    NodoBP *hoja = bp_primera_hoja(raiz);
    int total = 0;
    while (hoja) {
        for (int i = 0; i < hoja->n; i++) {
            if (!hoja->eliminado[i])
                total++;
        }
        hoja = hoja->siguiente;
    }
    return total;
}


/* --- Inserción en B+ ---
 *
 * Estrategia: inserción recursiva que retorna información de split
 * hacia arriba mediante una estructura auxiliar.
 *
 * Diferencia crucial con B-Tree:
 *   - Al partir una HOJA, la clave mediana se COPIA al padre
 *     (permanece en la hoja derecha).
 *   - Al partir un nodo INTERNO, la clave mediana se MUEVE al padre
 *     (igual que en B-Tree).
 */

typedef struct {
    int promovida;       /* Clave que sube al padre            */
    NodoBP *nuevo_nodo;  /* Nuevo nodo creado por el split     */
    int hubo_split;      /* 1 si hubo split, 0 si no           */
} ResultadoInsBP;


ResultadoInsBP bp_insertar_rec(NodoBP *nodo, int valor) {
    ResultadoInsBP resultado = { .hubo_split = 0, .nuevo_nodo = NULL };

    if (nodo->es_hoja) {
        /* --- Insertar en hoja --- */

        /* Encontrar posición */
        int pos = 0;
        while (pos < nodo->n && valor > nodo->claves[pos])
            pos++;

        /* Verificar duplicado */
        if (pos < nodo->n && nodo->claves[pos] == valor) {
            if (nodo->eliminado[pos]) {
                nodo->eliminado[pos] = 0;  /* Reactivar */
            }
            return resultado;
        }

        /* Desplazar e insertar */
        for (int j = nodo->n; j > pos; j--) {
            nodo->claves[j] = nodo->claves[j - 1];
            nodo->eliminado[j] = nodo->eliminado[j - 1];
        }
        nodo->claves[pos] = valor;
        nodo->eliminado[pos] = 0;
        nodo->n++;

        /* ¿Overflow? → Split de hoja */
        if (nodo->n == ORDEN_BP) {
            g_splits++;
            int mitad = ORDEN_BP / 2;

            NodoBP *nueva_hoja = bp_crear_nodo(1);

            /* Copiar mitad derecha a la nueva hoja */
            for (int j = mitad; j < nodo->n; j++) {
                nueva_hoja->claves[j - mitad] = nodo->claves[j];
                nueva_hoja->eliminado[j - mitad] = nodo->eliminado[j];
            }
            nueva_hoja->n = nodo->n - mitad;
            nodo->n = mitad;

            /* Mantener la lista enlazada de hojas */
            nueva_hoja->siguiente = nodo->siguiente;
            nodo->siguiente = nueva_hoja;

            /* COPIA de la primera clave de la nueva hoja sube al padre */
            resultado.promovida = nueva_hoja->claves[0];
            resultado.nuevo_nodo = nueva_hoja;
            resultado.hubo_split = 1;
        }

    } else {
        /* --- Nodo interno: bajar al hijo correspondiente --- */
        int i = 0;
        while (i < nodo->n && valor >= nodo->claves[i])
            i++;

        ResultadoInsBP sub = bp_insertar_rec(nodo->hijos[i], valor);

        if (sub.hubo_split) {
            /* Un hijo hizo split → incorporar la clave promovida */
            for (int j = nodo->n; j > i; j--) {
                nodo->claves[j] = nodo->claves[j - 1];
                nodo->eliminado[j] = nodo->eliminado[j - 1];
            }
            for (int j = nodo->n + 1; j > i + 1; j--)
                nodo->hijos[j] = nodo->hijos[j - 1];

            nodo->claves[i] = sub.promovida;
            nodo->eliminado[i] = 0;
            nodo->hijos[i + 1] = sub.nuevo_nodo;
            nodo->n++;

            /* ¿Overflow en nodo interno? → Split de nodo interno */
            if (nodo->n == ORDEN_BP) {
                g_splits++;
                int mitad = ORDEN_BP / 2;

                NodoBP *nuevo_interno = bp_crear_nodo(0);

                /* La clave mediana SUBE (se mueve, no se copia) */
                resultado.promovida = nodo->claves[mitad];

                /* Claves a la derecha de la mediana van al nuevo nodo */
                for (int j = mitad + 1; j < nodo->n; j++) {
                    nuevo_interno->claves[j - mitad - 1] = nodo->claves[j];
                    nuevo_interno->eliminado[j - mitad - 1] = nodo->eliminado[j];
                }
                for (int j = mitad + 1; j <= nodo->n; j++)
                    nuevo_interno->hijos[j - mitad - 1] = nodo->hijos[j];

                nuevo_interno->n = nodo->n - mitad - 1;
                nodo->n = mitad;

                resultado.nuevo_nodo = nuevo_interno;
                resultado.hubo_split = 1;
            }
        }
    }

    return resultado;
}


NodoBP* bp_insertar(NodoBP *raiz, int valor) {
    if (!raiz) {
        raiz = bp_crear_nodo(1);
        raiz->claves[0] = valor;
        raiz->eliminado[0] = 0;
        raiz->n = 1;
        return raiz;
    }

    ResultadoInsBP res = bp_insertar_rec(raiz, valor);

    if (res.hubo_split) {
        /* La raíz hizo split → nueva raíz */
        NodoBP *nueva_raiz = bp_crear_nodo(0);
        nueva_raiz->claves[0] = res.promovida;
        nueva_raiz->hijos[0] = raiz;
        nueva_raiz->hijos[1] = res.nuevo_nodo;
        nueva_raiz->n = 1;
        return nueva_raiz;
    }

    return raiz;
}


/* --- Eliminación perezosa en B+ --- */
int bp_eliminar_perezosa(NodoBP *raiz, int valor) {
    int idx;
    NodoBP *nodo = bp_buscar(raiz, valor, &idx);
    if (!nodo) return 0;
    nodo->eliminado[idx] = 1;
    return 1;
}


/* --- Eliminación estándar en B+ ---
 * Similar a B-Tree pero más simple en ciertos aspectos:
 * - Solo eliminamos de hojas (todos los datos están ahí).
 * - Los nodos internos pueden conservar "fantasmas" de claves eliminadas
 *   como guías de búsqueda (opcional, depende de la implementación).
 */

void bp_eliminar_de_hoja(NodoBP *hoja, int pos) {
    for (int j = pos; j < hoja->n - 1; j++) {
        hoja->claves[j] = hoja->claves[j + 1];
        hoja->eliminado[j] = hoja->eliminado[j + 1];
    }
    hoja->n--;
}

/* Eliminación recursiva con redistribución y fusión */
int bp_eliminar_rec(NodoBP *nodo, int valor, NodoBP *padre, int idx_en_padre) {
    if (nodo->es_hoja) {
        /* Buscar la clave en la hoja */
        int pos = -1;
        for (int i = 0; i < nodo->n; i++) {
            if (nodo->claves[i] == valor) {
                pos = i;
                break;
            }
        }
        if (pos == -1) return 0;  /* No encontrado */

        bp_eliminar_de_hoja(nodo, pos);

        /* Si es la raíz o tiene suficientes claves, terminamos */
        if (!padre || nodo->n >= MIN_CLAVES_BP)
            return 1;

        /* Underflow: intentar redistribuir o fusionar */
        /* Hermano izquierdo */
        if (idx_en_padre > 0) {
            NodoBP *hermano = padre->hijos[idx_en_padre - 1];
            if (hermano->n > MIN_CLAVES_BP) {
                /* Robar del hermano izquierdo */
                g_rotaciones++;
                for (int j = nodo->n; j > 0; j--) {
                    nodo->claves[j] = nodo->claves[j - 1];
                    nodo->eliminado[j] = nodo->eliminado[j - 1];
                }
                nodo->claves[0] = hermano->claves[hermano->n - 1];
                nodo->eliminado[0] = hermano->eliminado[hermano->n - 1];
                nodo->n++;
                hermano->n--;
                padre->claves[idx_en_padre - 1] = nodo->claves[0];
                return 1;
            }
        }
        /* Hermano derecho */
        if (idx_en_padre < padre->n) {
            NodoBP *hermano = padre->hijos[idx_en_padre + 1];
            if (hermano->n > MIN_CLAVES_BP) {
                g_rotaciones++;
                nodo->claves[nodo->n] = hermano->claves[0];
                nodo->eliminado[nodo->n] = hermano->eliminado[0];
                nodo->n++;
                bp_eliminar_de_hoja(hermano, 0);
                padre->claves[idx_en_padre] = hermano->claves[0];
                return 1;
            }
        }
        /* Fusionar con hermano */
        if (idx_en_padre > 0) {
            /* Fusionar con hermano izquierdo */
            NodoBP *hermano = padre->hijos[idx_en_padre - 1];
            g_rotaciones++;
            for (int j = 0; j < nodo->n; j++) {
                hermano->claves[hermano->n + j] = nodo->claves[j];
                hermano->eliminado[hermano->n + j] = nodo->eliminado[j];
            }
            hermano->n += nodo->n;
            hermano->siguiente = nodo->siguiente;
            /* Quitar del padre */
            for (int j = idx_en_padre - 1; j < padre->n - 1; j++) {
                padre->claves[j] = padre->claves[j + 1];
                padre->eliminado[j] = padre->eliminado[j + 1];
            }
            for (int j = idx_en_padre; j < padre->n; j++)
                padre->hijos[j] = padre->hijos[j + 1];
            padre->n--;
            free(nodo);
        } else {
            /* Fusionar con hermano derecho */
            NodoBP *hermano = padre->hijos[idx_en_padre + 1];
            g_rotaciones++;
            for (int j = 0; j < hermano->n; j++) {
                nodo->claves[nodo->n + j] = hermano->claves[j];
                nodo->eliminado[nodo->n + j] = hermano->eliminado[j];
            }
            nodo->n += hermano->n;
            nodo->siguiente = hermano->siguiente;
            for (int j = idx_en_padre; j < padre->n - 1; j++) {
                padre->claves[j] = padre->claves[j + 1];
                padre->eliminado[j] = padre->eliminado[j + 1];
            }
            for (int j = idx_en_padre + 1; j < padre->n; j++)
                padre->hijos[j] = padre->hijos[j + 1];
            padre->n--;
            free(hermano);
        }
        return 1;
    }

    /* Nodo interno: bajar al hijo correcto */
    int i = 0;
    while (i < nodo->n && valor >= nodo->claves[i])
        i++;

    return bp_eliminar_rec(nodo->hijos[i], valor, nodo, i);
}

NodoBP* bp_eliminar(NodoBP *raiz, int valor) {
    if (!raiz) return NULL;
    bp_eliminar_rec(raiz, valor, NULL, 0);

    /* Si la raíz quedó vacía y tiene hijos */
    if (raiz->n == 0 && !raiz->es_hoja) {
        NodoBP *nueva = raiz->hijos[0];
        free(raiz);
        return nueva;
    }
    return raiz;
}


/* --- Imprimir estructura del B+ --- */
void bp_imprimir(NodoBP *nodo, int nivel) {
    if (!nodo) return;

    for (int i = 0; i < nivel; i++) printf("    ");

    printf("[");
    for (int i = 0; i < nodo->n; i++) {
        if (i > 0) printf(", ");
        if (nodo->es_hoja && nodo->eliminado[i])
            printf(ROJO "~%d~" RESET, nodo->claves[i]);
        else
            printf("%d", nodo->claves[i]);
    }
    printf("]");
    if (nodo->es_hoja) {
        printf(" (hoja)");
        if (nodo->siguiente)
            printf(" -> [%d...]", nodo->siguiente->claves[0]);
    }
    printf("\n");

    if (!nodo->es_hoja) {
        for (int i = 0; i <= nodo->n; i++)
            bp_imprimir(nodo->hijos[i], nivel + 1);
    }
}

int bp_altura(NodoBP *nodo) {
    if (!nodo) return 0;
    if (nodo->es_hoja) return 1;
    return 1 + bp_altura(nodo->hijos[0]);
}

void bp_liberar(NodoBP *nodo) {
    if (!nodo) return;
    if (!nodo->es_hoja) {
        for (int i = 0; i <= nodo->n; i++)
            bp_liberar(nodo->hijos[i]);
    }
    free(nodo);
}


/* ╔═════════════════════════════════════════════════════════════════════════╗
 * ║                                                                       ║
 * ║               SECCIÓN 3: ÁRBOL AVL (PARA COMPARACIÓN)                 ║
 * ║                                                                       ║
 * ╚═════════════════════════════════════════════════════════════════════════╝
 *
 * El AVL es un ABB auto-balanceado donde la diferencia de alturas entre
 * los subárboles izquierdo y derecho de todo nodo es como máximo 1.
 *
 * Se incluye para comparar:
 *   - Altura del árbol vs B/B+ para el mismo conjunto de datos
 *   - Cantidad de rotaciones/reestructuraciones
 *   - Cantidad de comparaciones en búsqueda
 */

typedef struct NodoAVL {
    int clave;
    struct NodoAVL *izq, *der;
    int altura;
} NodoAVL;

int avl_altura(NodoAVL *n) { return n ? n->altura : 0; }
int avl_max(int a, int b) { return a > b ? a : b; }
int avl_balance(NodoAVL *n) { return n ? avl_altura(n->izq) - avl_altura(n->der) : 0; }

NodoAVL* avl_crear_nodo(int clave) {
    NodoAVL *n = (NodoAVL*)malloc(sizeof(NodoAVL));
    n->clave = clave;
    n->izq = n->der = NULL;
    n->altura = 1;
    return n;
}

NodoAVL* avl_rotar_derecha(NodoAVL *y) {
    g_rotaciones++;
    NodoAVL *x = y->izq;
    NodoAVL *T2 = x->der;
    x->der = y;
    y->izq = T2;
    y->altura = 1 + avl_max(avl_altura(y->izq), avl_altura(y->der));
    x->altura = 1 + avl_max(avl_altura(x->izq), avl_altura(x->der));
    return x;
}

NodoAVL* avl_rotar_izquierda(NodoAVL *x) {
    g_rotaciones++;
    NodoAVL *y = x->der;
    NodoAVL *T2 = y->izq;
    y->izq = x;
    x->der = T2;
    x->altura = 1 + avl_max(avl_altura(x->izq), avl_altura(x->der));
    y->altura = 1 + avl_max(avl_altura(y->izq), avl_altura(y->der));
    return y;
}

NodoAVL* avl_insertar(NodoAVL *nodo, int clave) {
    if (!nodo) return avl_crear_nodo(clave);

    g_comparaciones++;
    if (clave < nodo->clave)
        nodo->izq = avl_insertar(nodo->izq, clave);
    else if (clave > nodo->clave)
        nodo->der = avl_insertar(nodo->der, clave);
    else
        return nodo;  /* Duplicado */

    nodo->altura = 1 + avl_max(avl_altura(nodo->izq), avl_altura(nodo->der));
    int bal = avl_balance(nodo);

    /* Caso Izq-Izq */
    if (bal > 1 && clave < nodo->izq->clave)
        return avl_rotar_derecha(nodo);
    /* Caso Der-Der */
    if (bal < -1 && clave > nodo->der->clave)
        return avl_rotar_izquierda(nodo);
    /* Caso Izq-Der */
    if (bal > 1 && clave > nodo->izq->clave) {
        nodo->izq = avl_rotar_izquierda(nodo->izq);
        return avl_rotar_derecha(nodo);
    }
    /* Caso Der-Izq */
    if (bal < -1 && clave < nodo->der->clave) {
        nodo->der = avl_rotar_derecha(nodo->der);
        return avl_rotar_izquierda(nodo);
    }

    return nodo;
}

NodoAVL* avl_buscar(NodoAVL *nodo, int clave) {
    if (!nodo) return NULL;
    g_comparaciones++;
    if (clave < nodo->clave) return avl_buscar(nodo->izq, clave);
    if (clave > nodo->clave) return avl_buscar(nodo->der, clave);
    return nodo;
}

void avl_inorder(NodoAVL *nodo) {
    if (!nodo) return;
    avl_inorder(nodo->izq);
    printf("%d ", nodo->clave);
    avl_inorder(nodo->der);
}

int avl_contar(NodoAVL *nodo) {
    if (!nodo) return 0;
    return 1 + avl_contar(nodo->izq) + avl_contar(nodo->der);
}

void avl_liberar(NodoAVL *nodo) {
    if (!nodo) return;
    avl_liberar(nodo->izq);
    avl_liberar(nodo->der);
    free(nodo);
}


/* ╔═════════════════════════════════════════════════════════════════════════╗
 * ║                                                                       ║
 * ║               SECCIÓN 4: PROGRAMA PRINCIPAL Y COMPARACIONES           ║
 * ║                                                                       ║
 * ╚═════════════════════════════════════════════════════════════════════════╝ */

void separador(const char *titulo) {
    printf("\n");
    printf(CYAN "════════════════════════════════════════════════════════════\n" RESET);
    printf(NEGRITA "  %s\n" RESET, titulo);
    printf(CYAN "════════════════════════════════════════════════════════════\n" RESET);
}

void subseparador(const char *titulo) {
    printf("\n" AMARILLO "  ── %s ──\n" RESET, titulo);
}


int main(void) {
    printf(NEGRITA "\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║     IMPLEMENTACIÓN DE ÁRBOLES B, B+ Y COMPARACIÓN AVL  ║\n");
    printf("  ║                    Orden = %d                            ║\n", ORDEN_B);
    printf("  ╚══════════════════════════════════════════════════════════╝\n" RESET);


    /* ================================================================
     *  DEMO 1: ÁRBOL-B
     * ================================================================ */
    separador("DEMOSTRACIÓN DEL ÁRBOL-B");

    int datos[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 25, 35, 40, 15, 22, 28};
    int n_datos = sizeof(datos) / sizeof(datos[0]);

    printf("\n  Insertando %d elementos: ", n_datos);
    for (int i = 0; i < n_datos; i++) printf("%d ", datos[i]);
    printf("\n\n");

    NodoB *arbol_b = NULL;
    resetear_contadores();

    for (int i = 0; i < n_datos; i++) {
        arbol_b = b_insertar(arbol_b, datos[i]);
        printf("  Tras insertar " VERDE "%2d" RESET ": ", datos[i]);
        b_inorder(arbol_b);
        printf("\n");
    }

    subseparador("Estructura final del Árbol-B");
    b_imprimir(arbol_b, 1);
    printf("\n  Altura: %d | Splits: %ld\n", b_altura(arbol_b), g_splits);

    subseparador("Búsqueda en Árbol-B");
    int buscar[] = {17, 99, 5, 40};
    for (int i = 0; i < 4; i++) {
        int idx;
        resetear_contadores();
        NodoB *res = b_buscar(arbol_b, buscar[i], &idx);
        printf("  Buscar %2d: %s (comparaciones: %ld)\n",
               buscar[i],
               res ? VERDE "ENCONTRADO" RESET : ROJO "NO ENCONTRADO" RESET,
               g_comparaciones);
    }

    subseparador("Recorrido In-Order (acceso secuencial)");
    printf("  ");
    b_inorder(arbol_b);
    printf("\n  Total elementos: %d\n", b_inorder_contar(arbol_b));

    subseparador("Eliminación perezosa (lazy delete) de 12 y 20");
    b_eliminar_perezosa(arbol_b, 12);
    b_eliminar_perezosa(arbol_b, 20);
    printf("  Estructura (elementos eliminados en " ROJO "rojo" RESET "):\n");
    b_imprimir(arbol_b, 1);
    printf("  Recorrido (sin eliminados): ");
    b_inorder(arbol_b);
    printf("\n  Elementos activos: %d\n", b_inorder_contar(arbol_b));

    subseparador("Eliminación estándar (con reestructuración) de 6 y 30");
    resetear_contadores();
    arbol_b = b_eliminar(arbol_b, 6);
    arbol_b = b_eliminar(arbol_b, 30);
    printf("  Estructura tras eliminar 6 y 30:\n");
    b_imprimir(arbol_b, 1);
    printf("  Reestructuraciones: %ld\n", g_rotaciones);


    /* ================================================================
     *  DEMO 2: ÁRBOL B+
     * ================================================================ */
    separador("DEMOSTRACIÓN DEL ÁRBOL B+");

    NodoBP *arbol_bp = NULL;
    resetear_contadores();

    printf("\n  Insertando los mismos %d elementos...\n\n", n_datos);

    for (int i = 0; i < n_datos; i++) {
        arbol_bp = bp_insertar(arbol_bp, datos[i]);
        printf("  Tras insertar " VERDE "%2d" RESET ": ", datos[i]);
        bp_recorrido_secuencial(arbol_bp);
        printf("\n");
    }

    subseparador("Estructura final del Árbol B+");
    bp_imprimir(arbol_bp, 1);
    printf("\n  Altura: %d | Splits: %ld\n", bp_altura(arbol_bp), g_splits);

    subseparador("Búsqueda en Árbol B+");
    for (int i = 0; i < 4; i++) {
        int idx;
        resetear_contadores();
        NodoBP *res = bp_buscar(arbol_bp, buscar[i], &idx);
        printf("  Buscar %2d: %s (comparaciones: %ld)\n",
               buscar[i],
               res ? VERDE "ENCONTRADO" RESET : ROJO "NO ENCONTRADO" RESET,
               g_comparaciones);
    }

    subseparador("Acceso secuencial (ventaja del B+: lista enlazada de hojas)");
    printf("  ");
    bp_recorrido_secuencial(arbol_bp);
    printf("\n  Total elementos: %d\n", bp_contar_elementos(arbol_bp));

    subseparador("Eliminación perezosa de 12 y 20");
    bp_eliminar_perezosa(arbol_bp, 12);
    bp_eliminar_perezosa(arbol_bp, 20);
    printf("  Estructura (eliminados en " ROJO "rojo" RESET "):\n");
    bp_imprimir(arbol_bp, 1);
    printf("  Recorrido secuencial (sin eliminados): ");
    bp_recorrido_secuencial(arbol_bp);
    printf("\n");

    subseparador("Eliminación estándar de 6 y 30");
    resetear_contadores();
    arbol_bp = bp_eliminar(arbol_bp, 6);
    arbol_bp = bp_eliminar(arbol_bp, 30);
    printf("  Estructura tras eliminar 6 y 30:\n");
    bp_imprimir(arbol_bp, 1);
    printf("  Reestructuraciones: %ld\n", g_rotaciones);


    /* ================================================================
     *  DEMO 3: COMPARACIÓN CON AVL
     * ================================================================ */
    separador("COMPARACIÓN: ÁRBOL-B vs B+ vs AVL");

    int N_PRUEBA = 1000;
    int *datos_prueba = (int*)malloc(N_PRUEBA * sizeof(int));

    /* Generar datos de prueba (permutación aleatoria) */
    srand(42);
    for (int i = 0; i < N_PRUEBA; i++)
        datos_prueba[i] = i + 1;
    for (int i = N_PRUEBA - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = datos_prueba[i];
        datos_prueba[i] = datos_prueba[j];
        datos_prueba[j] = tmp;
    }

    printf("\n  Insertando %d elementos aleatorios en cada estructura...\n", N_PRUEBA);

    /* --- Insertar en Árbol-B --- */
    NodoB *test_b = NULL;
    resetear_contadores();
    for (int i = 0; i < N_PRUEBA; i++)
        test_b = b_insertar(test_b, datos_prueba[i]);
    long splits_b = g_splits;
    int altura_b = b_altura(test_b);

    /* --- Insertar en Árbol B+ --- */
    NodoBP *test_bp = NULL;
    resetear_contadores();
    for (int i = 0; i < N_PRUEBA; i++)
        test_bp = bp_insertar(test_bp, datos_prueba[i]);
    long splits_bp = g_splits;
    int altura_bp = bp_altura(test_bp);

    /* --- Insertar en AVL --- */
    NodoAVL *test_avl = NULL;
    resetear_contadores();
    for (int i = 0; i < N_PRUEBA; i++)
        test_avl = avl_insertar(test_avl, datos_prueba[i]);
    long rots_avl = g_rotaciones;
    int altura_avl = avl_altura(test_avl);

    printf("\n  ┌─────────────────────┬──────────┬──────────┬──────────┐\n");
    printf("  │     Métrica         │  Árbol-B │ Árbol B+ │   AVL    │\n");
    printf("  ├─────────────────────┼──────────┼──────────┼──────────┤\n");
    printf("  │ Altura              │    %3d   │    %3d   │   %3d    │\n",
           altura_b, altura_bp, altura_avl);
    printf("  │ Splits/Rotaciones   │    %3ld   │    %3ld   │   %3ld    │\n",
           splits_b, splits_bp, rots_avl);

    /* --- Comparar búsquedas --- */
    long comp_b_total = 0, comp_bp_total = 0, comp_avl_total = 0;
    int busquedas = 100;
    for (int i = 0; i < busquedas; i++) {
        int val = datos_prueba[rand() % N_PRUEBA];
        int idx;

        resetear_contadores();
        b_buscar(test_b, val, &idx);
        comp_b_total += g_comparaciones;

        resetear_contadores();
        bp_buscar(test_bp, val, &idx);
        comp_bp_total += g_comparaciones;

        resetear_contadores();
        avl_buscar(test_avl, val);
        comp_avl_total += g_comparaciones;
    }

    printf("  │ Comp. búsqueda avg  │  %5.1f   │  %5.1f   │  %5.1f   │\n",
           (double)comp_b_total / busquedas,
           (double)comp_bp_total / busquedas,
           (double)comp_avl_total / busquedas);
    printf("  │ Elementos           │   %4d   │   %4d   │  %4d    │\n",
           b_inorder_contar(test_b), bp_contar_elementos(test_bp), avl_contar(test_avl));
    printf("  └─────────────────────┴──────────┴──────────┴──────────┘\n");


    /* --- Análisis teórico --- */
    separador("ANÁLISIS COMPARATIVO");

    printf("\n");
    printf("  " NEGRITA "1. ALTURA:" RESET "\n");
    printf("     AVL:  O(1.44 · log₂ n) = O(log₂ n)\n");
    printf("     B/B+: O(log_m n) donde m = orden del árbol\n");
    printf("     → Para n=%d y m=%d: B/B+ requieren MUCHOS menos niveles.\n", N_PRUEBA, ORDEN_B);
    printf("     → AVL: ~%.0f niveles teóricos, B: ~%.0f niveles teóricos.\n",
           1.44 * log2(N_PRUEBA), log(N_PRUEBA) / log(ORDEN_B));

    printf("\n  " NEGRITA "2. REESTRUCTURACIÓN:" RESET "\n");
    printf("     AVL:  Rotaciones simples/dobles. Máximo O(log n) por inserción.\n");
    printf("     B/B+: Splits de nodos. Menos frecuentes pero más costosos.\n");
    printf("     → AVL tuvo %ld rotaciones vs %ld/%ld splits en B/B+.\n",
           rots_avl, splits_b, splits_bp);

    printf("\n  " NEGRITA "3. ACCESO SECUENCIAL:" RESET "\n");
    printf("     AVL:  In-order traversal recursivo O(n), pero con pobre localidad.\n");
    printf("     B:    In-order recursivo O(n), mejor localidad por nodos más grandes.\n");
    printf("     B+:   Recorrido lineal de hojas enlazadas O(n), MEJOR localidad.\n");
    printf("     → B+ es ideal para rangos y acceso secuencial (bases de datos).\n");

    printf("\n  " NEGRITA "4. USO EN DISCO vs MEMORIA:" RESET "\n");
    printf("     AVL:  Ideal para datos en memoria (punteros simples).\n");
    printf("     B/B+: Ideales para disco (nodos = bloques, minimizan I/O).\n");

    printf("\n  " NEGRITA "5. ELIMINACIÓN:" RESET "\n");
    printf("     AVL:  Rotaciones para rebalancear. Máx O(log n) rotaciones.\n");
    printf("     B/B+: Redistribución o fusión de nodos. Más complejo pero\n");
    printf("           la eliminación perezosa simplifica enormemente.\n");


    /* Limpieza */
    b_liberar(arbol_b);
    bp_liberar(arbol_bp);
    b_liberar(test_b);
    bp_liberar(test_bp);
    avl_liberar(test_avl);
    free(datos_prueba);

    printf("\n" VERDE "  ✓ Memoria liberada correctamente.\n" RESET);
    printf("\n");

    return 0;
}