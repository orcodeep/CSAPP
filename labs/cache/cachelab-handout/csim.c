#define _XOPEN_SOURCE 600        // for (optarg, optind,..) declaration 
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cachelab.h"

typedef struct {
    int valid;
    unsigned int tag;
    uint8_t age;
}line;

typedef struct {
    line* lines;
}set;   

FILE* fileopen(char* filename);
set* make_cache(unsigned int sets, int E);
int* parse(FILE* fileptr, set* cache, int s, int b, int E);
void free_cache(set* cache, unsigned int sets);

int main(int argc, char* argv[])
{
    int s = -1; int E = -1; int b = -1; int t = 0;
    char* trace = NULL; // tracefile path

    int opt;
    while((opt = getopt(argc, argv, ":hvs:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 'h':
                printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
                exit(-1);

            case 'v':
                printf("verbose output was not required by the 'test-csim' "
                       "so didn't add that. Its easy to add but just makes the code "
                       "harder to read.\n");
                exit(-1);

            case 's':
                s = atoi(optarg); 
                break;
            
            case 'E':
                E = atoi(optarg);
                break;

            case 'b':
                b = atoi(optarg);
                break;

            case 't':
                t = 1;
                trace = optarg;
                // printf("tracefile:- %s\n", trace);
                break;

            case ':':
                printf("Missing argument for %c\n", optopt);
                exit(-1);

            case '?':
                printf("Unknown option character %c\n", optopt);
                exit(-1);
        }
    }

    if (s < 0 || E < 0 || b < 0)
    {
        printf("Set size or Associativity or Block size were not specified correctly\n");
        exit(-1);
    }
    if (!t){printf("-t flag missing\n"); exit(-1);}
    unsigned int sets = 1u << s;  // 2^s
    set* cache = make_cache(sets, E);

    FILE* tracefile = fileopen(trace);
    int* arr = parse(tracefile, cache, s, b, E); 

    printSummary(arr[0], arr[1], arr[2]);
    free_cache(cache, sets);
    free(arr);
    return 0;
}

inline FILE* fileopen(char* filename)
{
    FILE* fileptr = fopen(filename, "r");
    if (fileptr == NULL) 
    {
        printf("tracefile not found\n");
        exit(1);
    }

    return fileptr;
}

set* make_cache(unsigned int sets, int E)
{
    set* cache = malloc(sets * sizeof(set));
    if (!cache) return NULL;
    for(unsigned int i = 0; i < sets; i++)
    {
        cache[i].lines = malloc(E * sizeof(line));
        if (!cache[i].lines) return NULL;

        // initialize valid bits to 0
        for(int j = 0; j < E; j++)
        {
            cache[i].lines[j].valid = 0;
            cache[i].lines[j].age = 0;
            // tag can be left uninitialized as Only when valid = 1 do you store a meaningful tag
        }
    }

    return cache;
}

int* parse(FILE* fileptr, set* cache, int s, int b, int E)
{
    size_t buffsize = 30;
    char buffer[buffsize];
    char copy[buffsize]; 
    int hits = 0; int misses = 0; int evictions = 0; 
    while(fgets(buffer, buffsize, fileptr) != NULL)
    {
        strcpy(copy, buffer);

        // ** do not miss the \r 
        char* opstr = strtok(copy, " ,\r\n");
        char op = opstr[0];
        if (op == 'I') {continue;}
        char* addrstr = strtok(NULL, " ,\r\n");
        unsigned long addr = strtoul(addrstr, NULL, 16);
        unsigned int tag = addr >> (s+b);
        unsigned int setindex = (addr >> b) & ((1u << s) - 1); 

        // now do the hits misses and evictions
        int hit = 0;
        set* set = &cache[setindex]; // <--- It need to be a pointer for proper access to cache
        int evictLine; int invLine = -1; int LRU = -1;
        
        for (int i = 0; i < E; i++)
        {
            if (set->lines[i].valid)
            {
                // initialising LRU and evict victim
                if (LRU == -1){
                    LRU = set->lines[i].age;
                    evictLine = i;
                } 

                // handle hits 
                if (set->lines[i].tag == tag)
                {
                    hit = 1;
                    hits++;
                    
                    if (op == 'M')
                    hits++;
                    
                    // decrement the ages of all the other lines
                    for(int j = 0; j < E; j++)
                    {
                        if (set->lines[j].age > 0)
                        set->lines[j].age--;
                    }
                    // important
                    set->lines[i].age = 255;

                    // no need for LRU calculation if its a hit
                    break;
                }

                // evict victim 
                if (set->lines[i].age < LRU)
                {
                    LRU = set->lines[i].age;
                    evictLine = i;
                }
            }
            // the miss will access the first invalid line if there is one
            else
            {
                if (invLine == -1)
                    invLine = i;
            }
        }

        // handle miss
        if (!hit)
        {
            misses++;
            if (op == 'M')
            hits++;
            
            int line;
            // invalid line present
            if (invLine != -1)
            {
                set->lines[invLine].valid = 1;
                line = invLine;
            }
            else // if no invalid need to evict
            {
                line = evictLine;
                evictions++;
            }

            set->lines[line].tag = tag;
            for (int i = 0; i < E; i++)
            {
                if (set->lines[i].age > 0)
                    set->lines[i].age--;
            }
            set->lines[line].age = 255;
        }
    }
    fclose(fileptr);
    int* arr = malloc(3 * sizeof(int));
    arr[0] = hits;
    arr[1] = misses;
    arr[2] = evictions;
    return arr;
}

void free_cache(set* cache, unsigned int sets)
{
/*
free(cache[i].lines) frees the entire array of cache lines for set i.

There is no need to loop over each line, because they are not individually malloc’d.
*/
    for(int i = 0; i < sets; i++)
    {
        free(cache[i].lines);
    }
    free(cache);
}


