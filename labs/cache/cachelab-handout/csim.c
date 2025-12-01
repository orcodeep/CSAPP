#define _XOPEN_SOURCE 600        // for (optarg, optind,..) declaration 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cachelab.h"

typedef struct {
    int valid;
    unsigned int setindex;
}cacheLine;

FILE* fileopen(char* filename);
cacheLine** make_cache(sets, lines, blocksize);
void parse(FILE* fileptr, int verbose);
void free_cache(cacheLine** cache, int sets);

int main(int argc, char* argv[])
{
    int s = -1; int E = -1; int b = -1; int t = 0; int verbose = 0;
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
                verbose = 1;
                break;

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
    int sets = 1 << s;
    int lines = 1 << E;
    //  int blocksize = 1 << b; 
    cacheLine** cache = (sets, lines);

    FILE* tracefile = fileopen(trace);
    parse(tracefile, verbose); // it should return the total hits, misses and evictions 

    printSummary(0, 0, 0);
    free_cache(cache, sets);
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

cacheLine** make_cache(sets, lines)
{
    cacheLine** cache = malloc(sets * sizeof(cacheLine*));
    for(int i = 0; i < sets; i++)
    {
        cache[i] = malloc(lines * sizeof(cacheLine));
        // initialize valid bits to 0
        for(int j = 0; j < lines; j++)
        {
            cache[i][j].valid = 0;
        }
    }
    
    return cache;
}

void parse(FILE* fileptr, int verbose)
{
    size_t buffsize = 30;
    char buffer[buffsize];
    char copy[buffsize];  
    while(fgets(buffer, buffsize, fileptr) != NULL)
    {
        strcpy(copy, buffer);

        char* op = strtok(copy, " ,  \n");
        char* addrstr = strtok(NULL, " ,  \n");
        char* datasize = strtok(NULL, " ,  \n"); 
        // printf("op: %s, addr: %s, datasize: %s\n", op, addr, datasize);

        unsigned long addr = strtoul(addrstr, NULL, 16);
        // now make the checking and mapping and cache logic

        
    }

    fclose(fileptr);
}


void free_cache(cacheLine** cache, int sets)
{
/*
free(cache[i]) frees the entire array of cache lines for set i.

There is no need to loop over each line, because they are not individually malloc’d.
*/
    for(int i = 0; i < sets; i++)
    {
        free(cache[i]);
    }
    free(cache);
}


