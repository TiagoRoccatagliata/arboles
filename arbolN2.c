#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct nodo_arbol {
    void* dato;
    struct nodo_arbol* padre;
    struct nodo_arbol* primer_hijo;
    struct nodo_arbol* hermano_der;
} arbol;

// Puntero a funcion para imprimir el dato generico
typedef void (*func_imprimir)(void*);
// Puntero a funcion para comparar datos genericos (retorna 0 si son iguales)
typedef int (*func_comparar)(void*, void*);
// Puntero a funcion para serializar el dato a string (para XML)
typedef char* (*func_a_string)(void*);
typedef void (*func_liberar_dato)(void*);

arbol* crear_arbol(void* dato) {
    arbol *nuevo = (arbol*)malloc(sizeof(arbol));
    if (nuevo == NULL) {
        printf("Error: No se pudo asignar memoria.\n");
        return NULL;
    }
    nuevo->dato = dato;
    nuevo->padre = NULL;
    nuevo->primer_hijo = NULL;
    nuevo->hermano_der = NULL;
    return nuevo;
}

void liberar_arbol(arbol *raiz, func_liberar_dato liberar_dato) {
    if (raiz == NULL) return;
    liberar_arbol(raiz->primer_hijo, liberar_dato);
    liberar_arbol(raiz->hermano_der, liberar_dato);
    
    if (liberar_dato != NULL) {
        liberar_dato(raiz->dato);
    }
    free(raiz);
}

// Retorna 1 si el arbol esta vacio (raiz es NULL), 0 en caso contrario
int arbol_vacio(arbol *raiz) {
    return (raiz == NULL);
}

// Retorna 1 si el nodo es nulo, 0 en caso contrario
int nodo_nulo(arbol *nodo) {
    return (nodo == NULL);
}

// Retorna la raiz del arbol (sube por los padres hasta encontrarla)
arbol* raiz(arbol *nodo) {
    if (nodo == NULL) return NULL;
    while (nodo->padre != NULL) {
        nodo = nodo->padre;
    }
    return nodo;
}

// Retorna el padre de un nodo
arbol* padre(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->padre;
}

// Retorna el primer hijo (mas a la izquierda) de un nodo
arbol* primer_hijo(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->primer_hijo;
}

// Retorna el hermano a derecha de un nodo
arbol* hermano_derecho(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->hermano_der;
}

// Retorna el contenido (dato) de un nodo
void* contenido(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->dato;
}

// Agrega un hijo al final de la lista de hijos de un nodo padre
void agregar_hijo(arbol *nodo_padre, arbol *hijo) {
    if (nodo_padre == NULL || hijo == NULL) return;

    hijo->padre = nodo_padre;

    if (nodo_padre->primer_hijo == NULL) {
        nodo_padre->primer_hijo = hijo;
    } else {
        arbol *aux = nodo_padre->primer_hijo;
        while (aux->hermano_der != NULL) {
            aux = aux->hermano_der;
        }
        aux->hermano_der = hijo;
    }
}

// Inserta un nodo como hijo de un nodo cuyo dato coincida con dato_padre.
// Busca recursivamente en el arbol.
// Retorna 1 si se inserto correctamente, 0 si no se encontro el padre.
arbol* buscar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp);

int insertar_nodo(arbol *raiz_arbol, void* dato_padre, void* dato_nuevo,
                  func_comparar cmp) {
    arbol *nodo_padre = buscar_nodo(raiz_arbol, dato_padre, cmp);
    if (nodo_padre == NULL) return 0;

    arbol *nuevo = crear_arbol(dato_nuevo);
    if (nuevo == NULL) return 0;

    agregar_hijo(nodo_padre, nuevo);
    return 1;
}

// Busca un nodo cuyo dato coincida segun la funcion de comparacion
arbol* buscar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp) {
    if (raiz_arbol == NULL) return NULL;

    if (cmp(raiz_arbol->dato, dato_buscado) == 0) {
        return raiz_arbol;
    }

    // Buscar en los hijos
    arbol *resultado = buscar_nodo(raiz_arbol->primer_hijo, dato_buscado, cmp);
    if (resultado != NULL) return resultado;

    // Buscar en los hermanos
    return buscar_nodo(raiz_arbol->hermano_der, dato_buscado, cmp);
}

// Elimina un nodo y todo su subarbol.
// Retorna 1 si se elimino, 0 si no se encontro.
// No se puede eliminar la raiz con esta funcion.
int eliminar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp) {
    arbol *nodo = buscar_nodo(raiz_arbol, dato_buscado, cmp);
    if (nodo == NULL || nodo->padre == NULL) {
        // No encontrado o es la raiz
        return 0;
    }

    arbol *papa = nodo->padre;

    // Caso 1: el nodo es el primer hijo del padre
    if (papa->primer_hijo == nodo) {
        papa->primer_hijo = nodo->hermano_der;
    } else {
        // Caso 2: el nodo esta en la lista de hermanos
        arbol *aux = papa->primer_hijo;
        while (aux != NULL && aux->hermano_der != nodo) {
            aux = aux->hermano_der;
        }
        if (aux != NULL) {
            aux->hermano_der = nodo->hermano_der;
        }
    }

    // Desconectar el nodo antes de liberar su subarbol
    nodo->hermano_der = NULL;
    liberar_arbol(nodo, free);
    return 1;
}

// PREORDEN: raiz -> hijos (de izquierda a derecha)
void preorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    imprimir(nodo->dato);

    arbol *hijo = nodo->primer_hijo;
    while (hijo != NULL) {
        preorden(hijo, imprimir);
        hijo = hijo->hermano_der;
    }
}

// INORDEN: primer hijo -> raiz -> resto de hijos
// (En un arbol N-ario se visita el primer hijo, luego la raiz,
//  y luego el resto de los hijos)
void inorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    // Recorrer el primer hijo
    if (nodo->primer_hijo != NULL) {
        inorden(nodo->primer_hijo, imprimir);
    }

    // Visitar la raiz
    imprimir(nodo->dato);

    // Recorrer el resto de los hijos
    if (nodo->primer_hijo != NULL) {
        arbol *hijo = nodo->primer_hijo->hermano_der;
        while (hijo != NULL) {
            inorden(hijo, imprimir);
            hijo = hijo->hermano_der;
        }
    }
}

// POSTORDEN: hijos (de izquierda a derecha) -> raiz
void postorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    arbol *hijo = nodo->primer_hijo;
    while (hijo != NULL) {
        postorden(hijo, imprimir);
        hijo = hijo->hermano_der;
    }

    imprimir(nodo->dato);
}


// Escribe el arbol en formato XML de forma recursiva
static void escribir_xml_recursivo(FILE *archivo, arbol *nodo, func_a_string a_string, int nivel) {
    if (nodo == NULL) return;

    // Indentacion
    for (int i = 0; i < nivel; i++) fprintf(archivo, "  ");

    char *str_dato = a_string(nodo->dato);

    if (nodo->primer_hijo == NULL) {
        // Nodo hoja
        fprintf(archivo, "<nodo dato=\"%s\"/>\n", str_dato);
    } else {
        // Nodo con hijos
        fprintf(archivo, "<nodo dato=\"%s\">\n", str_dato);

        arbol *hijo = nodo->primer_hijo;
        while (hijo != NULL) {
            escribir_xml_recursivo(archivo, hijo, a_string, nivel + 1);
            hijo = hijo->hermano_der;
        }

        for (int i = 0; i < nivel; i++) fprintf(archivo, "  ");
        fprintf(archivo, "</nodo>\n");
    }

    free(str_dato);
}

// Guarda el arbol completo en un archivo XML
int guardar_xml(arbol *raiz_arbol, const char *nombre_archivo,
                func_a_string a_string) {
    if (raiz_arbol == NULL || nombre_archivo == NULL) return 0;

    FILE *archivo = fopen(nombre_archivo, "w");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo '%s' para escritura.\n",
               nombre_archivo);
        return 0;
    }

    fprintf(archivo, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(archivo, "<arbol>\n");
    escribir_xml_recursivo(archivo, raiz_arbol, a_string, 1);
    fprintf(archivo, "</arbol>\n");

    fclose(archivo);
    printf("Arbol guardado exitosamente en '%s'.\n", nombre_archivo);
    return 1;
}

// Lee un arbol desde un archivo XML (parser simplificado)
// Formato esperado: <nodo dato="valor"> ... </nodo> o <nodo dato="valor"/>
static char* extraer_atributo_dato(const char *linea) {
    const char *inicio = strstr(linea, "dato=\"");
    if (inicio == NULL) return NULL;

    inicio += 6; // saltar 'dato="'
    const char *fin = strchr(inicio, '"');
    if (fin == NULL) return NULL;

    int longitud = fin - inicio;
    char *valor = (char*)malloc(longitud + 1);
    strncpy(valor, inicio, longitud);
    valor[longitud] = '\0';
    return valor;
}

// Funcion auxiliar para cargar el arbol. Recibe un puntero a funcion que
// convierte un string al tipo de dato deseado.
typedef void* (*func_desde_string)(const char*);

arbol* cargar_xml(const char *nombre_archivo, func_desde_string desde_string) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo '%s' para lectura.\n",
               nombre_archivo);
        return NULL;
    }

    // Pila para mantener los nodos padres durante el parseo
    arbol *pila[256];
    int tope = -1;
    arbol *raiz_arbol = NULL;

    char linea[1024];
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        // Ignorar la declaracion XML y las etiquetas <arbol>
        if (strstr(linea, "<?xml") || strstr(linea, "<arbol") ||
            strstr(linea, "</arbol")) {
            continue;
        }

        // Nodo hoja: <nodo dato="..."/>
        if (strstr(linea, "/>") != NULL) {
            char *valor = extraer_atributo_dato(linea);
            if (valor != NULL) {
                void *dato = desde_string(valor);
                free(valor);
                arbol *nuevo = crear_arbol(dato);

                if (tope >= 0) {
                    agregar_hijo(pila[tope], nuevo);
                } else {
                    raiz_arbol = nuevo;
                }
            }
        }
        // Nodo con hijos (apertura): <nodo dato="...">
        else if (strstr(linea, "<nodo") != NULL && strstr(linea, "</nodo") == NULL) {
            char *valor = extraer_atributo_dato(linea);
            if (valor != NULL) {
                void *dato = desde_string(valor);
                free(valor);
                arbol *nuevo = crear_arbol(dato);

                if (tope >= 0) {
                    agregar_hijo(pila[tope], nuevo);
                } else {
                    raiz_arbol = nuevo;
                }

                pila[++tope] = nuevo;
            }
        }
        // Cierre de nodo: </nodo>
        else if (strstr(linea, "</nodo") != NULL) {
            if (tope >= 0) tope--;
        }
    }

    fclose(archivo);
    printf("Arbol cargado exitosamente desde '%s'.\n", nombre_archivo);
    return raiz_arbol;
}


void imprimir_int(void* dato) {
    printf("%d ", *(int*)dato);
}

int comparar_int(void* a, void* b) {
    return *(int*)a - *(int*)b;
}

char* int_a_string(void* dato) {
    char *str = (char*)malloc(20);
    sprintf(str, "%d", *(int*)dato);
    return str;
}

void* int_desde_string(const char* str) {
    int *valor = (int*)malloc(sizeof(int));
    *valor = atoi(str);
    return valor;
}

// Helper para crear un int en el heap
int* nuevo_int(int valor) {
    int *p = (int*)malloc(sizeof(int));
    *p = valor;
    return p;
}


void imprimir_separador(const char *titulo) {
    printf("\n");
    printf("----------------------------------------------\n");
    printf("  %s\n", titulo);
    printf("----------------------------------------------\n");
}

int main() {
    imprimir_separador("CREACION DEL ARBOL");

    arbol *nodo1 = crear_arbol(nuevo_int(1));
    arbol *nodo2 = crear_arbol(nuevo_int(2));
    arbol *nodo3 = crear_arbol(nuevo_int(3));
    arbol *nodo4 = crear_arbol(nuevo_int(4));
    arbol *nodo5 = crear_arbol(nuevo_int(5));
    arbol *nodo6 = crear_arbol(nuevo_int(6));
    arbol *nodo7 = crear_arbol(nuevo_int(7));
    arbol *nodo8 = crear_arbol(nuevo_int(8));
    arbol *nodo9 = crear_arbol(nuevo_int(9));

    agregar_hijo(nodo1, nodo2);
    agregar_hijo(nodo1, nodo3);
    agregar_hijo(nodo1, nodo4);
    agregar_hijo(nodo2, nodo5);
    agregar_hijo(nodo2, nodo6);
    agregar_hijo(nodo4, nodo7);
    agregar_hijo(nodo4, nodo8);
    agregar_hijo(nodo6, nodo9);

    printf("Arbol creado con exito.\n");

    imprimir_separador("CONSULTAS BASICAS");

    printf("Arbol vacio?                  %s\n",
           arbol_vacio(nodo1) ? "Si" : "No");

    printf("Nodo nulo (nodo1)?            %s\n",
           nodo_nulo(nodo1) ? "Si" : "No");
    printf("Nodo nulo (NULL)?             %s\n",
           nodo_nulo(NULL) ? "Si" : "No");

    arbol *r = raiz(nodo9);
    printf("Raiz desde nodo 9:            %d\n", *(int*)contenido(r));

    arbol *p = padre(nodo6);
    printf("Padre de nodo 6:              %d\n", *(int*)contenido(p));

    arbol *ph = primer_hijo(nodo2);
    printf("Primer hijo de nodo 2:        %d\n", *(int*)contenido(ph));

    arbol *hd = hermano_derecho(nodo2);
    printf("Hermano derecho de nodo 2:    %d\n", *(int*)contenido(hd));

    printf("Contenido de nodo 4:          %d\n", *(int*)contenido(nodo4));

    // -------------------------------------------------------
    //  Recorridos
    // -------------------------------------------------------
    imprimir_separador("RECORRIDOS");

    printf("Preorden:   ");
    preorden(nodo1, imprimir_int);
    printf("\n");

    printf("Inorden:    ");
    inorden(nodo1, imprimir_int);
    printf("\n");

    printf("Postorden:  ");
    postorden(nodo1, imprimir_int);
    printf("\n");

    // -------------------------------------------------------
    //  Insercion por busqueda
    // -------------------------------------------------------
    imprimir_separador("INSERCION");

    int val_padre = 3;
    printf("Insertando nodo 10 como hijo de nodo %d...\n", val_padre);
    insertar_nodo(nodo1, nuevo_int(val_padre), nuevo_int(10), comparar_int);

    printf("Preorden:   ");
    preorden(nodo1, imprimir_int);
    printf("\n");

    // -------------------------------------------------------
    //  Eliminacion
    // -------------------------------------------------------
    imprimir_separador("ELIMINACION");

    int val_eliminar = 4;
    printf("Eliminando nodo %d (y su subarbol)...\n", val_eliminar);
    eliminar_nodo(nodo1, &val_eliminar, comparar_int);

    printf("Preorden:   ");
    preorden(nodo1, imprimir_int);
    printf("\n");

    // -------------------------------------------------------
    //  Persistencia XML: guardar
    // -------------------------------------------------------
    imprimir_separador("PERSISTENCIA XML - GUARDAR");

    guardar_xml(nodo1, "arbol.xml", int_a_string);

    // Mostrar el contenido del archivo
    printf("\nContenido de arbol.xml:\n\n");
    FILE *f = fopen("arbol.xml", "r");
    if (f) {
        char linea[256];
        while (fgets(linea, sizeof(linea), f)) {
            printf("  %s", linea);
        }
        fclose(f);
    }

    // -------------------------------------------------------
    //  Persistencia XML: cargar
    // -------------------------------------------------------
    imprimir_separador("PERSISTENCIA XML - CARGAR");

    arbol *arbol_cargado = cargar_xml("arbol.xml", int_desde_string);

    if (arbol_cargado != NULL) {
        printf("Preorden del arbol cargado: ");
        preorden(arbol_cargado, imprimir_int);
        printf("\n");

        liberar_arbol(arbol_cargado, free);
    }

    // -------------------------------------------------------
    //  Limpieza
    // -------------------------------------------------------
    imprimir_separador("LIMPIEZA");
    liberar_arbol(nodo1, free);
    printf("Memoria liberada correctamente.\n\n");

    return 0;
}