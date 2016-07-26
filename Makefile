all:
	gcc -Wall -Wextra -Wconversion -Wshadow -Wno-unused-result -Wsequence-point -Wlogical-op -O2 --std=c99 -o findstring_trie findstring_trie.c
