#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define T 2 // Grado mínimo. Máximo de claves = 3, Máximo de hijos = 4.

typedef struct BTreeNode {
    int keys[2 * T - 1];
    bool is_deleted[2 * T - 1]; // Soporte para eliminación perezosa (Lazy)
    struct BTreeNode *children[2 * T];
    int n; 
    bool leaf; 
} BTreeNode;

BTreeNode* createNode(bool is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->n = 0;
    node->leaf = is_leaf;
    for (int i = 0; i < 2 * T; i++) node->children[i] = NULL;
    for (int i = 0; i < 2 * T - 1; i++) node->is_deleted[i] = false;
    return node;
}

// 1. Acceso secuencial (In-Order) ignorando los eliminados perezosamente
void traverse(BTreeNode* root) {
    if (root != NULL) {
        int i;
        for (i = 0; i < root->n; i++) {
            if (!root->leaf) traverse(root->children[i]);
            if (!root->is_deleted[i]) {
                printf(" %d", root->keys[i]);
            }
        }
        if (!root->leaf) traverse(root->children[i]);
    }
}

// 2. Búsqueda de un elemento
BTreeNode* search(BTreeNode* root, int k, int* index) {
    int i = 0;
    while (i < root->n && k > root->keys[i]) i++;
    
    // Encontrado y no está marcado como eliminado
    if (i < root->n && root->keys[i] == k && !root->is_deleted[i]) {
        *index = i;
        return root; 
    }
    if (root->leaf) return NULL; 
    return search(root->children[i], k, index);
}

// -- Funciones de Inserción y Reestructuración (Split) --
void splitChild(BTreeNode* parent, int i, BTreeNode* full_child) {
    BTreeNode* new_child = createNode(full_child->leaf);
    new_child->n = T - 1;

    for (int j = 0; j < T - 1; j++) {
        new_child->keys[j] = full_child->keys[j + T];
        new_child->is_deleted[j] = full_child->is_deleted[j + T];
    }
    if (!full_child->leaf) {
        for (int j = 0; j < T; j++) new_child->children[j] = full_child->children[j + T];
    }
    full_child->n = T - 1;

    for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
    parent->children[i + 1] = new_child;

    for (int j = parent->n - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->is_deleted[j + 1] = parent->is_deleted[j];
    }
    parent->keys[i] = full_child->keys[T - 1];
    parent->is_deleted[i] = full_child->is_deleted[T - 1];
    parent->n++;
}

void insertNonFull(BTreeNode* node, int k) {
    int i = node->n - 1;
    if (node->leaf) {
        while (i >= 0 && node->keys[i] > k) {
            node->keys[i + 1] = node->keys[i];
            node->is_deleted[i + 1] = node->is_deleted[i];
            i--;
        }
        node->keys[i + 1] = k;
        node->is_deleted[i + 1] = false;
        node->n++;
    } else {
        while (i >= 0 && node->keys[i] > k) i--;
        i++;
        if (node->children[i]->n == 2 * T - 1) {
            splitChild(node, i, node->children[i]);
            if (node->keys[i] < k) i++;
        }
        insertNonFull(node->children[i], k);
    }
}

BTreeNode* insert(BTreeNode* root, int k) {
    if (root == NULL) {
        root = createNode(true);
        root->keys[0] = k;
        root->n = 1;
        return root;
    }
    if (root->n == 2 * T - 1) {
        BTreeNode* s = createNode(false);
        s->children[0] = root;
        splitChild(s, 0, root);
        int i = (s->keys[0] < k) ? 1 : 0;
        insertNonFull(s->children[i], k);
        return s;
    } else {
        insertNonFull(root, k);
        return root;
    }
}

// -- ELIMINACIÓN PEREZOSA (Lazy Deletion) --
// Simplemente marca el elemento como eliminado sin tocar la estructura.
void delete_lazy(BTreeNode* root, int k) {
    int idx;
    BTreeNode* node = search(root, k, &idx);
    if (node != NULL) {
        node->is_deleted[idx] = true;
        printf("Elemento %d eliminado (Lazy).\n", k);
    } else {
        printf("Elemento %d no encontrado para eliminar.\n", k);
    }
}

// -- ELIMINACIÓN ESTÁNDAR MÍNIMA (Solo en hojas) --
// Elimina físicamente desplazando el arreglo. 
// NOTA: Omite la reestructuración por underflow para no complicar el código.
void delete_standard_leaf(BTreeNode* root, int k) {
    int idx;
    BTreeNode* node = search(root, k, &idx);
    
    if (node != NULL && node->leaf) {
        // Desplazar elementos a la izquierda para borrarlo físicamente
        for (int i = idx; i < node->n - 1; i++) {
            node->keys[i] = node->keys[i + 1];
            node->is_deleted[i] = node->is_deleted[i + 1];
        }
        node->n--;
        printf("Elemento %d eliminado fisicamente (Standard Leaf).\n", k);
    } else if (node != NULL && !node->leaf) {
        printf("Elemento %d esta en nodo interno. Eliminacion estandar interna requiere reestructuracion compleja.\n", k);
    }
}

int main() {
    BTreeNode* root = NULL;

    // 3. Construcción inicial
    int datos[] = {10, 20, 30, 40, 50, 60, 70};
    int num_datos = sizeof(datos) / sizeof(datos[0]);

    for (int i = 0; i < num_datos; i++) root = insert(root, datos[i]);

    printf("Recorrido inicial:");
    traverse(root);
    printf("\n");

    // 4. Eliminaciones
    delete_lazy(root, 40); // Eliminación perezosa
    delete_standard_leaf(root, 70); // Eliminación estándar (en hoja)

    printf("Recorrido despues de eliminaciones:");
    traverse(root);
    printf("\n");

    // 2. Búsqueda de un elemento
    int idx;
    if (search(root, 40, &idx) == NULL) {
        printf("Busqueda de 40: Correctamente NO encontrado (fue borrado perezosamente).\n");
    }

    return 0;
}