#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

#define MIN_CHAR 32
#define MAX_CHAR 126
#define CHAR_CNT (MAX_CHAR - MIN_CHAR + 1)

#define BUILD_BUF_SIZE 512
#define WORK_BUF_SIZE 512

#define LINES_CNT 117
#define CHARS_BY_LINE 117

struct linked_node {
  struct linked_node* next;

  /* Character written in the edge */
  char c;

  /* Endpoint of the edge */
  struct trie_node* trie_ptr;
};

struct trie_node {
  /* Linked list of edges */
  struct linked_node* list_next;

  /* Some line ends here */
  bool is_final;
};

/*  Moves from given node in a trie to another connected by given character. */
/*  Creates new node if no such node exists. */
struct trie_node* traverse(struct trie_node* from, char c) {
  struct linked_node* ptr = from->list_next;

  /* First edge from given node */
  if (from->list_next == NULL) {
    from->list_next = (struct linked_node*)calloc(1, sizeof(struct linked_node));
    from->list_next->c = c;
    from->list_next->trie_ptr = (struct trie_node*)calloc(1, sizeof(struct trie_node));
    return from->list_next->trie_ptr;
  }

  /* Search in linked list */
  while (true)
  {
    if (ptr->c == c) return ptr->trie_ptr;
    if (ptr->next == NULL) break;
    ptr = ptr->next;
  }

  /* Nothing was found, so new node should be created */
  ptr->next = (struct linked_node*)calloc(1, sizeof(struct linked_node));
  ptr->next->c = c;
  ptr->next->trie_ptr = (struct trie_node*)calloc(1, sizeof(struct trie_node));
  return ptr->next->trie_ptr;
}

/* Adds new string to a trie from given node. */
struct trie_node* add_str(struct trie_node* from, char* str, int len) {
  struct trie_node* ptr = from;
  for (int i = 0; i < len; i++) {
    ptr = traverse(ptr, str[i]);
  }
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
    int len = (int)strlen(buf);

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

    ptr = add_str(ptr, buf, len);

    if (last) {
      ptr->is_final = true;
      ptr = root;
    }

    ret = fgets(buf, BUILD_BUF_SIZE, f);
  }
  fclose(f);
  return root;
}

/* Finds a node connected to given by given character. */
/* Returns NULL if no such node exists. */
struct trie_node* find_next(struct trie_node* from, char c) {
  struct linked_node* ptr = from->list_next;

  /* Search in linked list */
  while (ptr != NULL)
  {
    if (ptr->c == c) return ptr->trie_ptr;
    ptr = ptr->next;
  }

  /* Nothing's found */
  return NULL;
}

/* Finds a node where ends given string in a trie starting from given node. */
/* Returns NULL if no such node exists. */
struct trie_node* find_line(struct trie_node* root, char* line)
{
  size_t len = strlen(line);
  struct trie_node* ptr = root;
  for (size_t i = 0; i < len && ptr != NULL; i++) {
    ptr = find_next(ptr, line[i]);
  }
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

    free(old_ptr);
  }
  free(node);
}

void work(const char* dict_filename, FILE* input_file, FILE* output_file) {
  time_t st = clock();

  struct trie_node* root = build_trie(dict_filename);

  fprintf(stderr, "Trie is built\n");
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

    if (buf[len-1] == '\n') {
      buf[len-1] = '\0';
      final = 1;
      len--;
    }

    ptr = find_line(ptr, buf);

    if (ptr == NULL) {
      fputs("NO\n", output_file);

      /*  Read until the end of the file */
      while (!final && (ret = fgets(buf, WORK_BUF_SIZE, input_file)) != NULL && buf[strlen(buf)-1] != '\n') ;

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

void stress()
{
  int cnt = 0;
  while (1) {
    size_t dic_len = 0;
    fprintf(stderr, "\r%d\r", ++cnt);
    FILE* f = fopen("dictionary.txt", "wt");
    char* lines[LINES_CNT];
    for (int i = 0; i < LINES_CNT; i++) {
      int chars = rand()%CHARS_BY_LINE + 1;
      /* int chars = CHARS_BY_LINE; */
      /* int chars = CHARS_BY_LINE-i+1; */
      lines[i] = (char*)malloc(sizeof(char)*((unsigned int)chars+1));
      lines[i][chars] = '\0';
      for (int j = 0; j < chars; j++) {
        char c = (char)(rand()%CHAR_CNT + MIN_CHAR);
        if (j == 3 && c == 't' && chars == 4) c = 'o';
        lines[i][j] = c;
        fputc(c,f);
        dic_len++;
      }
      fputc('\n', f);
      dic_len++;
    }
    fclose(f);

    fprintf(stderr, "Dictionary size: %lf MB\n", (double)dic_len/1024.0/1024.0);

    bool res[LINES_CNT];
    f = fopen("input.txt", "wt");
    for (int i = 0; i < LINES_CNT; i++) {
      int chars = rand()%CHARS_BY_LINE + 1;
      char line[chars+1];
      line[chars] = '\0';
      for (int j = 0; j < chars; j++) {
        char c = (char)(rand()%CHAR_CNT + MIN_CHAR);
        line[j] = c;
        fputc(c,f);
      }
      res[i] = false;
      for (int j = 0; j < LINES_CNT; j++) {
        if (strcmp(line, lines[j]) == 0) {
          res[i] = true;
          break;
        }
      }
      fputc('\n', f);
    }
    fputs("exit\n", f);
    fclose(f);

    for (int i = 0; i < LINES_CNT; i++) {
      free(lines[i]);
    }

    FILE* input_file = fopen("input.txt", "rt");
    FILE* output_file = fopen("output.txt", "wt");
    work("dictionary.txt", input_file, output_file);
    fclose(input_file);
    fclose(output_file);

    output_file = fopen("output.txt", "rt");

    for (int i = 0; i < LINES_CNT; i++) {
      char line[8];
      fgets(line, 8, output_file);

      if (line[2] == '\n') line[2] = '\0';
      if (line[3] == '\n') line[3] = '\0';

      if (strcmp(line, "YES") != 0 && strcmp(line, "NO") != 0) {
        fprintf(stderr, "Some strange shit is happening right here\n");
        exit(0);
      }

      if (strcmp(line, "YES") == 0) {
        if (res[i] == false) {
          fprintf(stderr, "It should be NO, but it's YES\n");
          exit(0);
        }
      }
      if (strcmp(line, "NO") == 0) {
        if (res[i] == true) {
          fprintf(stderr, "It should be YES, but it's NO\n");
          exit(0);
        }
      }
    }
    fclose(output_file);
  }
}

int main(int argc, char const* argv[])
{
  stress();
  if (argc < 2) {
    printf("Usage:\n%s [FILE]\n", argv[0]);
    exit(0);
  }
  work(argv[1], stdin, stdout);
  return 0;
}
