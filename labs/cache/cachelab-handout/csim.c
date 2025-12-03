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
int* parse(FILE* fileptr, int verbose, set* cache, int s, int b, int E);
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

    // printf("s = %i, E = %i, b = %i\n", s, E, b);

    FILE* tracefile = fileopen(trace);
    int* arr = parse(tracefile, verbose, cache, s, b, E); 

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

int* parse(FILE* fileptr, int verbose, set* cache, int s, int b, int E)
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
        if (op == 'I')
            continue;
        char* addrstr = strtok(NULL, " ,\r\n");
        char* datastr = strtok(NULL, " ,\r\n"); 
        if (verbose)
            printf("%s %s,%s ", opstr, addrstr, datastr);
        // printf("op: %s, addr: %s, datasize: %s\n", op, addrstr, datastr);

        unsigned long addr = strtoul(addrstr, NULL, 16);
        // printf("addr: %lu ", addr);
        
        unsigned int tag = addr >> (s+b);
        unsigned int setindex = (addr >> b) & ((1u << s) - 1); 

        // printf("tag: %u, setindex: %u ", tag, setindex);

        // now do the hits misses and evictions
        int hit = 0;

        set* set = &cache[setindex];
        int evictLine = 0; int invLine = -1;
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
                    if (verbose){printf("hit ");}

                    if (set->increasing)
                    {
                        if (set->lines[i].age < 255)
                            set->lines[i].age++;

                        if (op == 'M')
                        {
                            hits++;
                            if (verbose){printf("hit\n");}

                            if (set->lines[i].age < 255)
                                set->lines[i].age++;
                        }
                        else 
                        {
                            if (verbose)
                            printf("\n");
                        }
                    }
                    else
                    {
                        if (set->lines[i].age > 0)
                            set->lines[i].age--;

                        if (op == 'M')
                        {
                            hits++;
                            if (verbose){printf("hit\n");}

                            if (set->lines[i].age > 0)
                                set->lines[i].age--;
                        }
                        else 
                        {
                            if (verbose)
                                printf("\n");
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
        
        // handle the miss 
        if (!hit)
        {
            misses++;
            int line = 0;
            
            // invalid line available
            if (invLine != -1)
            {
                line = invLine;

                if (op == 'M')
                {
                    hits++;
                    if (verbose){printf("miss hit\n");}
                }
                else
                {
                    if (verbose){printf("miss\n");}
                }
            }
            else
            {
                evictions++;
                line = evictLine;

                if (op == 'M')
                {
                    hits++;
                    if (verbose){printf("miss eviction hit\n");}
                }
                else
                {
                    if (verbose){printf("miss eviction\n");}
                }
            }

            if (set->increasing)
                set->lines[line].age = 0;
            else
                set->lines[line].age = 255;
            
            set->lines[line].tag = tag;
            set->lines[line].valid = 1;   // <--- THIS very important
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
free(cache[i]) frees the entire array of cache lines for set i.

There is no need to loop over each line, because they are not individually malloc’d.
*/
    for(int i = 0; i < sets; i++)
    {
        free(cache[i].lines);
    }
    free(cache);
}


