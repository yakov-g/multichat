#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#include "bst_string.h"

#define CONNS 3000000

/* Hash: client_id -> chatroom_name */
static const char *_client_chatroom_hash[CONNS];

/* Hash(BST): chatroom_name -> Chatroom */
static Bst_Node *_chatrooms_hash = NULL;

/* FIXME: Array of connections together
 * with poll.Need to implement better structure. */
static struct pollfd _fds[CONNS];
static size_t _fds_num = 1, _fds_num_cur;
static int _compress_array = 0;

/* Linked list to keep clients per group */
typedef struct _Clients_List Clients_List;
struct _Clients_List
{
   int client_id;
   Clients_List *next;
   Clients_List *prev;
};

/* Struct to keep info about chatroom */
typedef struct
{
   char *name;
   Clients_List *head;
   Clients_List *tail;
} Chatroom;

/* Get client's chatroom by client_id */
const char *
client_chatroom_name_get(int client_id)
{
   if (client_id < 1) return NULL;
   return _client_chatroom_hash[client_id];
}

/* Insert client into chatroom.
 * Fills in all structures: hashes, lists */
void
chatroom_client_insert(Chatroom *cr, int client_id)
{
   if (!cr) return;
   Clients_List *node = calloc (1, sizeof(Clients_List));
   node->client_id = client_id;
   if (cr->head == NULL)
     {
        cr->head = node;
        cr->tail = node;
     }
   else
     {
        node->prev = cr->tail;
        cr->tail->next = node;
        cr->tail = node;
     }
   _client_chatroom_hash[client_id] = cr->name;
}

/* API to get Chatroom desc by name.
 * If such group doesn't exist, create it. */
Chatroom *
chatroom_get(const char *chatroom_name)
{
   Bst_Node *chatroom_node = bst_string_node_find(_chatrooms_hash, chatroom_name);
   if (!chatroom_node)
     {
        Chatroom *cr = calloc(1, sizeof(Chatroom));
        cr->name = strdup(chatroom_name);
        _chatrooms_hash = bst_string_node_insert(_chatrooms_hash, chatroom_name, cr);
        return cr;
     }
   return bst_string_node_data_get(chatroom_node);
}

/* Delete Chatroom desc */
void
chatroom_delete(Chatroom *cr)
{
   free(cr->name);
   free(cr);
}

/* Remove client from Chatroom desc.
 * And free all structures/lists/hashes. */
void
chatroom_client_remove(int client_id)
{
   const char *chatroom_name = _client_chatroom_hash[client_id];
   _client_chatroom_hash[client_id] = NULL;

   if (chatroom_name)
     {
        Chatroom *cr = chatroom_get(chatroom_name);
        Clients_List *p = cr->head;
        while (p)
          {
             if (p->client_id == client_id)
               {
                  Clients_List *prev = p->prev;
                  Clients_List *next = p->next;

                  if (!prev)
                    {
                       cr->head = next;
                    }
                  else
                    {
                       prev->next = next;
                    }

                  if (!next)
                    {
                       cr->tail = prev;
                    }
                  else
                    {
                       next->prev = prev;
                    }
                  break;
               }
             p = p->next;
          }
        free(p);

        // If list of clients is empty, delete chatroom
        if (!cr->head)
          {
             _chatrooms_hash = bst_string_node_remove(_chatrooms_hash, chatroom_name);
             chatroom_delete(cr);
          }
     }
}

/* Parse message.
 * if "#name" pattern is found, use it as "name" chatroom. */
static char *
_chatroom_name_parse(const char *str)
{
   if (!str) return NULL;
   if (strlen(str) < 2) return NULL;
   char *ret = NULL;

   if (str[0] == '#' && str[1] != ' ')
     {
        char *pc = strpbrk(str, " \r\n");
        size_t n = pc - str;
        /* Create chatroom */
        if (n > 1)
          {
             ret = strndup(str, n);
          }
     }
   return ret;
}

/* Handler to catch Ctrl + C
 * and clean up. */
static void
_sigint_handler(int signal)
{
   (void) signal;
   size_t i = 0;
   close(_fds[0].fd);
   for (i = 1; i < _fds_num; i++)
     {
        chatroom_client_remove(_fds[i].fd);
        close(_fds[i].fd);
     }
   printf("\nCaught signal. Cleaning up. Good bye.\n");
   exit(0);
}

/* Helper to init signal handler. */
static void
_redefine_sigint_handler()
{
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = _sigint_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, NULL);
}

int
main(int argc, char **argv)
{
   int listener, newfd;
   struct sockaddr_storage remoteaddr;
   socklen_t addrlen;

   /* Message buffer. */
   char buf[1024];
   int nbytes;

   int yes = 1;        /* For setsockopt() SO_REUSEADDR */
   size_t i =0, j = 0;
   int rv;

   struct addrinfo hints, *ai, *p;
   const char *port = NULL;

   if (argc != 2)
     {
        printf("Usage: ./server <port>\n");
        exit(1);
     }
   else
     {
        port = argv[1];
     }

   _redefine_sigint_handler();
   memset(_fds, 0, sizeof(_fds));
   memset(_client_chatroom_hash, 0, sizeof(_client_chatroom_hash));
   memset(&hints, 0, sizeof(hints));

   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0)
     {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
     }

   for(p = ai; p != NULL; p = p->ai_next)
     {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
          {
             continue;
          }

        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
          {
             close(listener);
             continue;
          }
        break;
     }

   if (p == NULL)
     {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(1);
     }

   freeaddrinfo(ai);

   if (listen(listener, 1000) == -1)
     {
        perror("listen");
        exit(1);
     }

   /* Add the listener to poll array */
   _fds[0].fd = listener;
   _fds[0].events = POLLIN;

   for(;;)
     {
        /* Polling infinitevily */
        if (poll(_fds, _fds_num, -1) == -1)
          {
             perror("poll");
             exit(1);
          }

        _fds_num_cur = _fds_num;
        for (i = 0; i < _fds_num_cur; i++)
          {
             if (_fds[i].revents == 0)
                continue;

             if (_fds[i].fd == listener)
               {
                  // Handle new connections
                  addrlen = sizeof(remoteaddr);
                  newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                  if (newfd == -1)
                    {
                       perror("accept");
                    }
                  else
                    {
                       _fds[_fds_num].fd = newfd;
                       _fds[_fds_num].events = POLLIN;
                       _fds_num++;
                       printf("selectserver: new connection, socket: %d\n", newfd);
                    }
               }
             else
               {
                  /* Handle data from a client */
                  if ((nbytes = recv(_fds[i].fd, buf, sizeof(buf), 0)) <= 0)
                    {
                       /* Error or connection closed by client */
                       if (nbytes == 0)
                         {
                            printf("selectserver: socket %d hung up\n", _fds[i].fd);
                         }
                       else
                         {
                            perror("recv");
                         }
                       chatroom_client_remove(_fds[i].fd);
                       close(_fds[i].fd);
                       _fds[i].fd = -1;
                       _compress_array = 1;
                    }
                  else
                    {
                       /* Process data from a client */
                       char *chatroom_name = _chatroom_name_parse(buf);
                       if (chatroom_name)
                         {
                            const char *current_chatroom = client_chatroom_name_get(_fds[i].fd);
                            if (current_chatroom)
                              {
                                 chatroom_client_remove(_fds[i].fd);;
                              }
                            Chatroom *cr = chatroom_get(chatroom_name);
                            chatroom_client_insert(cr, _fds[i].fd);
                         }
                       else
                         {
                            const char *current_chatroom = client_chatroom_name_get(_fds[i].fd);
                            /* If client is not in chatroom, ask him to join. */
                            if (!current_chatroom)
                              {
                                 sprintf(buf, " >Join chatroom first. (type: #chatroom)\n");
                                 if (send(_fds[i].fd, buf, strlen(buf), 0) == -1)
                                   {
                                      perror("send");
                                   }
                                 continue;
                              }

                            Chatroom *cr = chatroom_get(current_chatroom);
                            if (!cr)
                              {
                                 printf("Can not find chat room: %s", current_chatroom);
                              }
                            else
                              {
                                 Clients_List *pl = cr->head;
                                 while (pl)
                                   {
                                      /* Send message to everyone, except sender */
                                      if (pl->client_id != _fds[i].fd)
                                        {
                                           if (send(pl->client_id, buf, nbytes, 0) == -1)
                                             {
                                                perror("send");
                                             }
                                        }
                                      pl = pl->next;
                                   }
                              }
                         }
                       if (chatroom_name) free(chatroom_name);
                    }
               }
          }

        /* FIXME: Compress array of connections.
         * Need to implement better structure. */
        if (_compress_array)
          {
             _compress_array = 0;
             for (i = 0; i < _fds_num; i++)
               {
                  if (_fds[i].fd == -1)
                    {
                       for(j = i; j < _fds_num; j++)
                         {
                            _fds[j].fd = _fds[j + 1].fd;
                         }
                       _fds_num--;
                    }
               }
          }
     }
   return 0;
}

