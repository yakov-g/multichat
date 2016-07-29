#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "bst_string.h"

/* BST node struct */
struct _Bst_Node
{
   char *key;
   void *data;
   Bst_Node *left, *right;
};

/* Node create function */
static Bst_Node *
_bst_string_node_create(const char *key, void *data)
{
   if (!key) return NULL;
   Bst_Node *n =  (Bst_Node *) calloc(1, sizeof(Bst_Node));
   n->key = strdup(key);
   n->data = data;
   return n;
}

/* Node delete function */
static void
_bst_string_node_delete(Bst_Node *node)
{
   if (!node) return;
   free(node->key);
   free(node);
}

/* Get data from node */
void *
bst_string_node_data_get(Bst_Node *node)
{
   if (!node) return NULL;
   return node->data;
}

/* Set data to node */
void *
bst_string_node_data_set(Bst_Node *node, void *data)
{
   if (!node) return NULL;
   return node->data = data;
}

/* Fetch key from node */
const char *
bst_string_node_key_get(Bst_Node *node)
{
   if (!node) return NULL;
   return node->key;
}

/* Helper to insert node */
static Bst_Node *
_bst_string_node_insert_helper(Bst_Node *node, const char *key, void *data)
{
   if (node == NULL) return _bst_string_node_create(key, data);
   if (strcmp(key, node->key) < 0)
      node->left = _bst_string_node_insert_helper(node->left, key, data);
   else if (strcmp(key, node->key) > 0)
      node->right = _bst_string_node_insert_helper(node->right, key, data);
   return node;
}

/* Insert node into BST */
Bst_Node*
bst_string_node_insert(Bst_Node *root, const char *key, void *data)
{
   if (!key) return NULL;
   if (root == NULL) return _bst_string_node_create(key, data);
   _bst_string_node_insert_helper(root, key, data);
   return root;
}

/* Find node by key in BST */
Bst_Node*
bst_string_node_find(Bst_Node *node, const char *key)
{
   if (!key) return NULL;
   if (node == NULL) return NULL;

   if (strcmp(key, node->key) < 0)
      return bst_string_node_find(node->left, key);
   else if (strcmp(key, node->key) > 0)
      return bst_string_node_find(node->right, key);

   return node;
}

/* Find minimum node in BST */
Bst_Node *
bst_string_min_value_node_get(Bst_Node *node)
{
   Bst_Node *current = node;
   while (current->left != NULL)
      current = current->left;
   return current;
}

/* Remove node from BST */
Bst_Node* bst_string_node_remove(Bst_Node* root, const char *key)
{
   if (root == NULL) return root;
   if (strcmp(key, root->key) < 0)
     {
        root->left = bst_string_node_remove(root->left, key);
     }
   else if (strcmp(key, root->key) > 0)
     {
        root->right = bst_string_node_remove(root->right, key);
     }
   else
     {
        if (root->left == NULL)
          {
             Bst_Node *temp = root->right;
             _bst_string_node_delete(root);
             return temp;
          }
        else if (root->right == NULL)
          {
             Bst_Node *temp = root->left;
             _bst_string_node_delete(root);
             return temp;
          }

        Bst_Node* temp = bst_string_min_value_node_get(root->right);

        root->key = temp->key;
        root->right = bst_string_node_remove(root->right, temp->key);
     }
   return root;
}

/* Traverse BST inorder */
void
bst_string_inorder(Bst_Node *root)
{
   if (root != NULL)
     {
        bst_string_inorder(root->left);
        printf("%s \n", root->key);
        bst_string_inorder(root->right);
     }
}
