#define _XOPEN_SOURCE 600        // for (optarg, optind,..) declaration 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "cachelab.h"

typedef struct {
    
}cacheLine;

FILE* parser_construct(char* filename);
void parse(FILE* fileptr, int verbose);

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
                printf("tracefile:- %s\n", trace);
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

    FILE* tracefile = parser_construct(trace);
    parse(tracefile, verbose);;

    printSummary(0, 0, 0);
    return 0;
}

inline FILE* parser_construct(char* filename)
{
    FILE* fileptr = fopen(filename, "r");
    if (fileptr == NULL) 
    {
        printf("tracefile not found\n");
        exit(1);
    }

    return fileptr;
}

void parse(FILE* fileptr, int verbose)
{
    size_t buffsize = 30;
    char buffer[buffsize];
    while(fgets(buffer, buffsize, fileptr) != NULL)
    {
        printf("Line: %s", buffer);
    }

    fclose(fileptr);
}


