#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct nodo_arbol {
    void* dato; // usamos void para que el arbol sea generico
    struct nodo_arbol* padre;
    struct nodo_arbol* primer_hijo;
    struct nodo_arbol* hermano_der;
} arbol;

typedef void (*func_imprimir)(void*);
typedef int (*func_comparar)(void*, void*);
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

int arbol_vacio(arbol *raiz) {
    return (raiz == NULL);
}

int nodo_nulo(arbol *nodo) {
    return (nodo == NULL);
}

arbol* raiz(arbol *nodo) {
    if (nodo == NULL) return NULL;
    while (nodo->padre != NULL) {
        nodo = nodo->padre;
    }
    return nodo;
}

arbol* padre(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->padre;
}

arbol* primer_hijo(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->primer_hijo;
}

arbol* hermano_derecho(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->hermano_der;
}

void* contenido(arbol *nodo) {
    if (nodo == NULL) return NULL;
    return nodo->dato;
}

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

arbol* buscar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp);

int insertar_nodo(arbol *raiz_arbol, void* dato_padre, void* dato_nuevo, func_comparar cmp) {
    arbol *nodo_padre = buscar_nodo(raiz_arbol, dato_padre, cmp);
    if (nodo_padre == NULL) return 0;

    arbol *nuevo = crear_arbol(dato_nuevo);
    if (nuevo == NULL) return 0;

    agregar_hijo(nodo_padre, nuevo);
    return 1;
}

arbol* buscar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp) {
    if (raiz_arbol == NULL) return NULL;

    if (cmp(raiz_arbol->dato, dato_buscado) == 0) {
        return raiz_arbol;
    }

    arbol *resultado = buscar_nodo(raiz_arbol->primer_hijo, dato_buscado, cmp);
    if (resultado != NULL) return resultado;

    return buscar_nodo(raiz_arbol->hermano_der, dato_buscado, cmp);
}

int eliminar_nodo(arbol *raiz_arbol, void* dato_buscado, func_comparar cmp) {
    arbol *nodo = buscar_nodo(raiz_arbol, dato_buscado, cmp);
    if (nodo == NULL || nodo->padre == NULL) {
        return 0;
    }

    arbol *papa = nodo->padre;

    if (papa->primer_hijo == nodo) {
        papa->primer_hijo = nodo->hermano_der;
    } else {
        arbol *aux = papa->primer_hijo;
        while (aux != NULL && aux->hermano_der != nodo) {
            aux = aux->hermano_der;
        }
        if (aux != NULL) {
            aux->hermano_der = nodo->hermano_der;
        }
    }

    nodo->hermano_der = NULL;
    liberar_arbol(nodo, free);
    return 1;
}

void preorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    imprimir(nodo->dato);

    arbol *hijo = nodo->primer_hijo;
    while (hijo != NULL) {
        preorden(hijo, imprimir);
        hijo = hijo->hermano_der;
    }
}

void inorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    if (nodo->primer_hijo != NULL) {
        inorden(nodo->primer_hijo, imprimir);
    }

    imprimir(nodo->dato);

    if (nodo->primer_hijo != NULL) {
        arbol *hijo = nodo->primer_hijo->hermano_der;
        while (hijo != NULL) {
            inorden(hijo, imprimir);
            hijo = hijo->hermano_der;
        }
    }
}

void postorden(arbol *nodo, func_imprimir imprimir) {
    if (nodo == NULL) return;

    arbol *hijo = nodo->primer_hijo;
    while (hijo != NULL) {
        postorden(hijo, imprimir);
        hijo = hijo->hermano_der;
    }

    imprimir(nodo->dato);
}


static void escribir_xml_recursivo(FILE *archivo, arbol *nodo, func_a_string a_string, int nivel) {
    if (nodo == NULL) return;

    for (int i = 0; i < nivel; i++) fprintf(archivo, "  ");

    char *str_dato = a_string(nodo->dato);

    if (nodo->primer_hijo == NULL) {
        fprintf(archivo, "<nodo dato=\"%s\"/>\n", str_dato);
    } else {
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

int guardar_xml(arbol *raiz_arbol, const char *nombre_archivo, func_a_string a_string) {
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

typedef void* (*func_desde_string)(const char*);

arbol* cargar_xml(const char *nombre_archivo, func_desde_string desde_string) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo '%s' para lectura.\n",
               nombre_archivo);
        return NULL;
    }

    arbol *pila[256];
    int tope = -1;
    arbol *raiz_arbol = NULL;

    char linea[1024];
    while (fgets(linea, sizeof(linea), archivo) != NULL) {
        if (strstr(linea, "<?xml") || strstr(linea, "<arbol") ||
            strstr(linea, "</arbol")) {
            continue;
        }

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

int* nuevo_int(int valor) {
    int *p = (int*)malloc(sizeof(int));
    *p = valor;
    return p;
}


void imprimir_separador(const char *titulo) {
    printf("\n");
    printf("  %s\n", titulo);
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

    imprimir_separador("INSERCION");

    int val_padre = 3;
    printf("Insertando nodo 10 como hijo de nodo %d...\n", val_padre);
    insertar_nodo(nodo1, nuevo_int(val_padre), nuevo_int(10), comparar_int);

    printf("Preorden:   ");
    preorden(nodo1, imprimir_int);
    printf("\n");

    imprimir_separador("ELIMINACION");

    int val_eliminar = 4;
    printf("Eliminando nodo %d (y su subarbol)...\n", val_eliminar);
    eliminar_nodo(nodo1, &val_eliminar, comparar_int);

    printf("Preorden:   ");
    preorden(nodo1, imprimir_int);
    printf("\n");

    imprimir_separador("PERSISTENCIA XML - GUARDAR");

    guardar_xml(nodo1, "arbol.xml", int_a_string);

    printf("\nContenido de arbol.xml:\n\n");
    FILE *f = fopen("arbol.xml", "r");
    if (f) {
        char linea[256];
        while (fgets(linea, sizeof(linea), f)) {
            printf("  %s", linea);
        }
        fclose(f);
    }

    imprimir_separador("PERSISTENCIA XML - CARGAR");

    arbol *arbol_cargado = cargar_xml("arbol.xml", int_desde_string);

    if (arbol_cargado != NULL) {
        printf("Preorden del arbol cargado: ");
        preorden(arbol_cargado, imprimir_int);
        printf("\n");

        liberar_arbol(arbol_cargado, free);
    }

    imprimir_separador("LIMPIEZA");
    liberar_arbol(nodo1, free);
    printf("Memoria liberada correctamente.\n\n");

    return 0;
}