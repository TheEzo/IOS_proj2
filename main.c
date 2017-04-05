#include <stdio.h>
#include <stdlib.h>

#define OUTPUT "proj2.out"

int main(int argc, char **argv) {
    if(argc != 7){
        fprintf(stderr, "Nespravny pocet argumentu!\n");
        exit(1);
    }
    int A = 0, C = 0, AGT = 0, CGT = 0, AWT = 0, CWT = 0;

    /****************** Kontrola zadanych hodnot ********************/

    char *pEnd;
    // přiřazení hodnot proměnným a kontrola, jestli jsou čísla
    A = strtol(argv[1], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }
    C = strtol(argv[2], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }
    AGT = strtol(argv[3], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }
    CGT = strtol(argv[4], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }
    AWT = strtol(argv[5], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }
    CWT = strtol(argv[6], &pEnd, 10);
    if(*pEnd != 0){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }

    // kontrola rozsahu hodnot
    if(A <= 0 || C <= 0 || AGT < 0 || AGT >= 5001 || CGT < 0 || CGT >= 5001 || AWT < 0 || AWT >= 5001 || CWT < 0 || CWT >= 5001){
        fprintf(stderr, "Neocekavany format hodnot\n");
        exit(1);
    }

    /*************** Konec kontroly zadanych hodnot ********************/

    // Otevreni souboru
    FILE *output;
    if((output = fopen(OUTPUT, "w")) == NULL){
        fprintf(stderr, "Soubor %s se nepodarilo otevrit\n", OUTPUT);
        exit(2);
    }





    // zavrani souboru
    if(fclose(output) == EOF){
        fprintf(stderr, "Soubor %s se nepodarilo zavrit\n", OUTPUT);
        exit(2);
    }

    return 0;
}