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
    uint8_t increasing; 
}set;   

FILE* fileopen(char* filename);
set* make_cache(unsigned int sets, int E);
void parse(FILE* fileptr, int verbose, set* cache, int s, int b, int E);
void free_cache(set* cache, unsigned int sets);

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
    unsigned int sets = 1u << s;  // 2^s
    set* cache = make_cache(sets, E);

    FILE* tracefile = fileopen(trace);
    parse(tracefile, verbose, cache, s, b, E); // it should return the total hits, misses and evictions 

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

set* make_cache(unsigned int sets, int E)
{
    set* cache = malloc(sets * sizeof(set));
    if (!cache) return NULL;
    for(unsigned int i = 0; i < sets; i++)
    {
        cache[i].lines = malloc(E * sizeof(line));
        if (!cache[i].lines) return NULL;
        cache[i].increasing = 1;   // initially every line's will increase. 
                                   // so line with min age is oldest accessed and should be evicted next

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

void parse(FILE* fileptr, int verbose, set* cache, int s, int b, int E)
{
    size_t buffsize = 30;
    char buffer[buffsize];
    char copy[buffsize];  
    while(fgets(buffer, buffsize, fileptr) != NULL)
    {
        strcpy(copy, buffer);

        char* op = strtok(copy, " ,\n");
        if (op[0] == 'I')
            continue;
        char* addrstr = strtok(NULL, " ,\n");
        int datasize = atoi(strtok(NULL, " ,\n")); 
        // printf("op: %s, addr: %s, datasize: %s\n", op, addrstr, datasize);

        unsigned long addr = strtoul(addrstr, NULL, 16);
        
        unsigned int tag = addr >>  (s+b);
        unsigned int setindex = (addr >> b) & ((1u << s) - 1); 

        // now do the hits misses and evictions
        int hits = 0; int misses = 0; int evictions = 0;

        set* set = &cache[setindex]; // set is a pointer to our memory block's set
        int evictLine = 0; int invLine = -1;
        int hit = 0; int miss = 0;
        uint8_t LRU = set->lines[0].age;
        int edgeCounder = 0;
        for(int i = 0; i < E; i++)
        {
            // if invalid line available, track the first as a invLine 
            if (!set->lines[i].valid)
            {
                if (invLine == -1)
                    invLine = i;

                continue;
            }
            else
            {
                // Count how many lines are at the extreme age
                if (set->increasing && set->lines[i].age == 255)
                    edgeCounder++;
                else if (!set->increasing && set->lines[i].age == 0)
                    edgeCounder++;

                /* flip the increasing flag if needed
                For a hit, if you flip the flag before updating the age, 
                the hit line will age in the new direction, not the old one.*/
                if (edgeCounder == E)
                    set->increasing = !set->increasing;                     

                // if hit
                if (tag == set->lines[i].tag)
                {
                    hit = 1;
                    hits++;

                    if (set->increasing)
                    {
                        if (set->lines[i].age < 255)
                            set->lines[i].age++;

                        if (op[0] == 'M')
                        {
                            hits++;

                            if (set->lines[i].age < 255)
                                set->lines[i].age++;
                        }
                    }
                    else
                    {
                        if (set->lines[i].age > 0)
                            set->lines[i].age--;

                        if (op[0] == 'M')
                        {
                            hits++;

                            if (set->lines[i].age > 0)
                                set->lines[i].age--;
                        }
                    }
                    
                    break;
                }
                       
                // track line with max/min LRU whichever 'increasing' flag demands
                if (set->increasing)
                {
                    if (set->lines[i].age < LRU)
                    {
                        LRU = set->lines[i].age;
                        evictLine = i;
                    }
                }
                else
                {
                    if (set->lines[i].age > LRU)
                    {
                        LRU = set->lines[i].age;
                        evictLine = i;
                    }
                }

            }

        }
                
    }

    fclose(fileptr);
}


void free_cache(set* cache, unsigned int sets)
{
/*
free(cache[i]) frees the entire array of cache lines for set i.

There is no need to loop over each line, because they are not individually malloc’d.
*/
    for(int i = 0; i < sets; i++)
    {
        free(cache[i].lines);
    }
    free(cache);
}


