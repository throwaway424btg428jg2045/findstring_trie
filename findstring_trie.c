#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define BUILD_BUF_SIZE 512
#define WORK_BUF_SIZE 512

struct linked_node {
  struct linked_node* next;

  /* String written in the edge */
  char* c;

  /* Endpoint of the edge */
  struct trie_node* trie_ptr;
};

struct trie_node {
  /* Linked list of edges */
  struct linked_node* list_next;

  /* Some line ends here */
  bool is_final;
};

/* Adds a new egde from a given node in a trie */
void add_new_list_node(struct linked_node** linked_ptr, char* str) {
  (*linked_ptr) = (struct linked_node*)calloc(1, sizeof(struct linked_node));
  (*linked_ptr)->c = (char *)malloc(sizeof(char)*(strlen(str)+1));
  strcpy((*linked_ptr)->c, str);
  (*linked_ptr)->trie_ptr = (struct trie_node*)calloc(1, sizeof(struct trie_node));
}

/* Makes a branch in a given edge */
struct trie_node* split_in_two(struct linked_node* ptr, size_t skip, char* str) {
  struct trie_node* node = ptr->trie_ptr;
  struct trie_node* new_node1;
  struct trie_node* new_node2;
  struct linked_node* new_linked;

  add_new_list_node(&new_linked, &ptr->c[skip]);

  new_node1 = new_linked->trie_ptr;
  memcpy(new_node1, node, sizeof(struct trie_node));
  node->list_next = new_linked;

  add_new_list_node(&node->list_next->next, &str[skip]);

  new_node2 = node->list_next->next->trie_ptr;

  ptr->c[skip] = '\0';

  node->is_final = false;

  return new_node2;
}

/* Adds a new node in the middle of a given edge */
struct trie_node* split_in_the_middle(struct linked_node* ptr, size_t skip) {
  struct trie_node new_node;
  memcpy(&new_node, ptr->trie_ptr, sizeof(struct trie_node));

  add_new_list_node(&ptr->trie_ptr->list_next, &ptr->c[skip]);

  memcpy(ptr->trie_ptr->list_next->trie_ptr, &new_node, sizeof(struct trie_node));

  ptr->c[skip] = '\0';
  ptr->trie_ptr->is_final = false;

  return ptr->trie_ptr;
}

/*  Moves from a given node in a trie to another connected by given character. */
/*  Creates a new node if no such node exists. */
struct trie_node* traverse(struct trie_node* from, char* str) {
  struct linked_node* ptr = from->list_next;
  size_t add_len = strlen(str);

  if (add_len == 0) return from;

  /* First edge from given node */
  if (from->list_next == NULL) {
    add_new_list_node(&from->list_next, str);
    return from->list_next->trie_ptr;
  }

  /* Search in linked list */
  while (true)
  {
    if (ptr->c[0] == str[0]) {
      size_t have_len = strlen(ptr->c);
      size_t idx = 1;

      while (idx < add_len && idx < have_len && ptr->c[idx] == str[idx]) idx++;

      if (idx < add_len && idx < have_len) {
        return split_in_two(ptr, idx, str);
      }

      if (idx < have_len) {
        return split_in_the_middle(ptr, idx);
      }

      if (idx < add_len) {
        return traverse(ptr->trie_ptr, &str[idx]);
      }

      return ptr->trie_ptr;
    }
    if (ptr->next == NULL) break;
    ptr = ptr->next;
  }

  /* Nothing was found, so new node should be created */
  add_new_list_node(&ptr->next, str);
  return ptr->next->trie_ptr;
}

/* Adds new string to a trie from given node. */
struct trie_node* add_str(struct trie_node* from, char* str) {
  struct trie_node* ptr = from;
  ptr = traverse(ptr, str);
  return ptr;
}

/* Builds new trie with lines from a file with given name. */
struct trie_node* build_trie(const char* filename)
{
  FILE* f = fopen(filename, "rt");
  if (f == NULL) {
    fprintf(stderr, "No such file: %s\n", filename);
    exit(0);
  }

  char buf[BUILD_BUF_SIZE] = {};
  struct trie_node* root = (struct trie_node*)calloc(1, sizeof(struct trie_node));
  struct trie_node* ptr = root;

  char* ret = fgets(buf, BUILD_BUF_SIZE, f);

  while (ret != NULL) {
    size_t len = strlen(buf);

    /* Last chunk of a line */
    bool last = false;

    /* Remove trailing newline character */
    if (buf[len-1] == '\n') {
      buf[len-1] = '\0';
      len--;
      last = true;
    }

    if (feof(f)) {
      last = true;
    }

    ptr = add_str(ptr, buf);

    if (last) {
      ptr->is_final = true;
      ptr = root;
    }

    ret = fgets(buf, BUILD_BUF_SIZE, f);
  }
  fclose(f);
  return root;
}

/* Finds a node connected to given by a given string. */
/* Returns NULL if no such node exists. */
struct trie_node* find_next(struct trie_node* from, char* str) {
  struct linked_node* ptr = from->list_next;

  /* Search in linked list */
  while (ptr != NULL)
  {
    if (ptr->c[0] == str[0])
    {
      size_t idx = 0;
      size_t add_len = strlen(str);
      size_t have_len = strlen(ptr->c);

      while (idx < add_len && idx < have_len && ptr->c[idx] == str[idx]) idx++;

      if (idx < have_len) return NULL;

      if (idx < add_len) return find_next(ptr->trie_ptr, &str[idx]);

      return ptr->trie_ptr;
    }

    ptr = ptr->next;
  }

  /* Nothing's found */
  return NULL;
}

/* Finds a node where ends given string in a trie starting from given node. */
/* Returns NULL if no such node exists. */
struct trie_node* find_line(struct trie_node* root, char* line)
{
  if (strlen(line) == 0) return root;

  struct trie_node* ptr = root;
  ptr = find_next(ptr, line);
  return ptr;
}

/* Deallocates memory of a subtree in a trie. */
void free_dfs(struct trie_node* node)
{
  struct linked_node* ptr = node->list_next;
  while (ptr != NULL)
  {
    struct linked_node* old_ptr = ptr;

    free_dfs(ptr->trie_ptr);

    ptr = ptr->next;

    free(old_ptr->c);
    free(old_ptr);
  }
  free(node);
}

void work(const char* dict_filename, FILE* input_file, FILE* output_file) {
  time_t st = clock();

  struct trie_node* root = build_trie(dict_filename);

  fprintf(stderr, "Building time: %lf sec.\n", (double)(clock()-st)/CLOCKS_PER_SEC);

  char buf[WORK_BUF_SIZE] = {};
  struct trie_node* ptr = root;

  /* First chunk of a line */
  bool first = true;

  st = clock();

  while (1) {
    char* ret = fgets(buf, WORK_BUF_SIZE, input_file);

    if (first && strcmp(ret, "exit\n") == 0) {
      break;
    }

    first = false;

    int len = (int)strlen(buf);

    /* Last chunk of a line */
    bool final = false;

    /* Remove trailing newline character */
    if (buf[len-1] == '\n') {
      buf[len-1] = '\0';
      final = 1;
      len--;
    }

    ptr = find_line(ptr, buf);

    if (ptr == NULL) {
      fputs("NO\n", output_file);

      /*  Read until the end of the file */
      while (!final &&
             (ret = fgets(buf, WORK_BUF_SIZE, input_file)) != NULL &&
             buf[strlen(buf)-1] != '\n') ;

      first = true;
      ptr = root;
      continue;
    }

    if (final) {
      if (ptr != NULL && ptr->is_final) {
        fputs("YES\n", output_file);
      } else {
        fputs("NO\n", output_file);
      }
      ptr = root;
      first = true;
    }
  }

  fprintf(stderr, "Quering time: %lf sec.\n", (double)(clock()-st)/CLOCKS_PER_SEC);

  st = clock();
  free_dfs(root);

  fprintf(stderr, "Freeing time: %lf sec.\n", (double)(clock()-st)/CLOCKS_PER_SEC);
}

int main(int argc, char const* argv[])
{
  if (argc < 2) {
    printf("Usage:\n%s [FILE]\n", argv[0]);
    exit(0);
  }
  work(argv[1], stdin, stdout);
  return 0;
}
