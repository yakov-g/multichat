#ifndef _STRING_BST_
#define _STRING_BST_

typedef struct _Bst_Node Bst_Node;

/* Insert node into BST */
Bst_Node*
bst_string_node_insert(Bst_Node *node, const char *key, void *data);

/* Remove node from BST */
Bst_Node*
bst_string_node_remove(Bst_Node* root, const char *key);

/* Find node by key in BST */
Bst_Node*
bst_string_node_find(Bst_Node *node, const char *key);

/* Traverse BST inorder */
void
bst_string_inorder(Bst_Node *root);

/* Get data from node */
void *
bst_string_node_data_get(Bst_Node *node);

/* Set data to node */
void *
bst_string_node_data_set(Bst_Node *node, void *data);

/* Fetch key from node */
const char *
bst_string_node_key_get(Bst_Node *node);

#endif

