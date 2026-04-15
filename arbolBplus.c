#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define T 2 

typedef struct BPlusNode {
    int keys[2 * T - 1];
    bool is_deleted[2 * T - 1]; 
    struct BPlusNode *children[2 * T];
    int n; 
    bool leaf; 
    struct BPlusNode *next; // puntero al hermano derecho
} BPlusNode;

BPlusNode* createNode(bool is_leaf) {
    BPlusNode* node = (BPlusNode*)malloc(sizeof(BPlusNode));
    node->n = 0;
    node->leaf = is_leaf;
    node->next = NULL; 
    for (int i = 0; i < 2 * T; i++) node->children[i] = NULL;
    for (int i = 0; i < 2 * T - 1; i++) node->is_deleted[i] = false;
    return node;
}

void traverse_fast(BPlusNode* root) {
    if (root == NULL) return;

    BPlusNode* actual = root;
    while (!actual->leaf) {
        actual = actual->children[0];
    }

    while (actual != NULL) {
        for (int i = 0; i < actual->n; i++) {
            if (!actual->is_deleted[i]) {
                printf("%d ", actual->keys[i]);
            }
        }
        actual = actual->next; 
    }
    printf("\n");
}

BPlusNode* search(BPlusNode* root, int k, int* index) {
    if (root == NULL) return NULL;
    
    int i = 0;
    while (i < root->n && k > root->keys[i]) i++;
    
    if (root->leaf) {
        if (i < root->n && root->keys[i] == k && !root->is_deleted[i]) {
            *index = i;
            return root;
        }
        return NULL; 
    }
    
    return search(root->children[i], k, index);
}

void splitChild(BPlusNode* parent, int i, BPlusNode* full_child) {
    BPlusNode* new_child = createNode(full_child->leaf);

    if (full_child->leaf) {
        new_child->n = T; 
        for (int j = 0; j < T; j++) {
            new_child->keys[j] = full_child->keys[j + T - 1];
            new_child->is_deleted[j] = full_child->is_deleted[j + T - 1];
        }
        full_child->n = T - 1; 

        new_child->next = full_child->next;
        full_child->next = new_child;

        for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
        parent->children[i + 1] = new_child;

        for (int j = parent->n - 1; j >= i; j--) parent->keys[j + 1] = parent->keys[j];
        parent->keys[i] = new_child->keys[0]; 
        parent->n++;

    } else {
        new_child->n = T - 1;
        for (int j = 0; j < T - 1; j++) new_child->keys[j] = full_child->keys[j + T];
        for (int j = 0; j < T; j++) new_child->children[j] = full_child->children[j + T];
        full_child->n = T - 1;

        for (int j = parent->n; j >= i + 1; j--) parent->children[j + 1] = parent->children[j];
        parent->children[i + 1] = new_child;

        for (int j = parent->n - 1; j >= i; j--) parent->keys[j + 1] = parent->keys[j];
        parent->keys[i] = full_child->keys[T - 1]; 
        parent->n++;
    }
}

void insertNonFull(BPlusNode* node, int k) {
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
        }
        insertNonFull(node->children[i], k);
    }
}

BPlusNode* insert(BPlusNode* root, int k) {
    if (root == NULL) {
        root = createNode(true);
        root->keys[0] = k;
        root->n = 1;
        return root;
    }
    if (root->n == 2 * T - 1) {
        BPlusNode* s = createNode(false);
        s->children[0] = root;
        splitChild(s, 0, root);
        int i = (s->keys[0] <= k) ? 1 : 0;
        insertNonFull(s->children[i], k);
        return s;
    } else {
        insertNonFull(root, k);
        return root;
    }
}

void delete_lazy(BPlusNode* root, int k) {
    int idx;
    BPlusNode* node = search(root, k, &idx);
    
    if (node != NULL) {
        node->is_deleted[idx] = true;
        printf("Elemento %d borrado perezosamente en la hoja.\n", k);
    } else {
        printf("Elemento %d no encontrado para eliminar.\n", k);
    }
}

int main() {
    BPlusNode* root = NULL;

    int datos[] = {10, 20, 30, 40, 50, 60, 70, 80};
    int num_datos = sizeof(datos) / sizeof(datos[0]);

    for (int i = 0; i < num_datos; i++) {
        root = insert(root, datos[i]);
    }

    printf("\nRecorrido ultrarrapido (via Lista Enlazada):\n");
    traverse_fast(root);

    printf("\n--- BORRADO PEREZOSO ---\n");
    delete_lazy(root, 30);
    delete_lazy(root, 50);

    printf("\nRecorrido despues de borrar 30 y 50:\n");
    traverse_fast(root);

    return 0;
}