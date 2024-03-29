#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define OUTPUT "proj2.out"

void child_gen(int C, int A, int CGT, int CWT, FILE * output);
void parent_gen(int A, int C, int AGT, int AWT, FILE * output);
void prints(char *kind, int number, int type, int CA, int CC, FILE * out);
void init();
void destroy();


// semafory
sem_t *sem_mem, *sem_child_cap, *sem_finish, *sem_prints, *sem_child_queue, *sem_cap_changing;
sem_t *sem_adult_leaving, *sem_waiting_for_parent, *sem_child_stopped, *w_enter, *o_enter;

// sdílená paměť
int *count, *adults_in, *children_in, *finishing, *total_adults_finished, *children_waiting, *parent_waiting;
int *stopped_child, *waiting_for_parent;

int *val;

// vstup pro prints()
char *types[6];

int main(int argc, char **argv){
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

    // hodnoty výpisu
    types[0] = "started";
    types[1] = "enter";
    types[2] = "trying to leave";
    types[3] = "leave";
    types[4] = "waiting";
    types[5] = "finished";


    srand(time(NULL));

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    init();

    pid_t pid, a_pid, c_pid;

    if((pid = fork()) < 0){
        fprintf(stderr, "Fork failed!\n");
        exit(2);
    }

    if (pid == 0) { // child generator
        child_gen(C, A, CGT, CWT, output);
        exit(0);
    }
    else {
        a_pid = pid;
        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }
        if (pid == 0) { // adult generator
            parent_gen(A, C, AGT, AWT, output);
            exit(0);
        }
        else
            c_pid = pid;
    }

    waitpid(c_pid, NULL, 0);
    waitpid(a_pid, NULL, 0);

    // zavrani souboru
    if(fclose(output) == EOF){
        fprintf(stderr, "Soubor %s se nepodarilo zavrit\n", OUTPUT);
        exit(2);
    }

    destroy();

    exit(0);
}

/**
 * Generator deti
 * @param C Celkovy pocet deti
 * @param CGT Cas, po kterem se vygeneruje nove dite
 * @param CWT Cas, po kterem dite opusti kritickou zonu
 */
void child_gen(int C, int A, int CGT, int CWT, FILE * output){
    pid_t pid;
    for (int i = 1; i <= C; i++) {
        if(CGT > 0) {
            usleep((rand() % (CGT+1)) * 1000);
        }

        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }

        if(pid == 0){
            //started
            prints("C", i, 0, 0, 0, output);

            if(A != *total_adults_finished && *children_in+1 > *adults_in*3){
                if(*parent_waiting > 0) {
                    /*************dítě nebrání rodičovi v odchodu*********/
                    sem_wait(sem_mem);
                    (*waiting_for_parent)++;
                    sem_post(sem_mem);

                    sem_wait(sem_waiting_for_parent);
                    sem_wait(sem_child_cap);

                    sem_wait(sem_prints);
                    sem_wait(sem_mem);
                    (*children_in)++;
                    sem_post(sem_mem);
                    //enter
                    prints("C", i, 1, 0, 0, output);
                    sem_post(sem_prints);

                }
                else{
                    prints("C", i, 4, *adults_in, *children_in, output);

                    sem_wait(sem_mem);
                    (*children_waiting)++;
                    sem_post(sem_mem);

                    sem_wait(w_enter);
                    sem_wait(sem_child_cap);

                    sem_wait(sem_prints);
                    sem_wait(sem_mem);
                    (*children_in)++;
                    sem_post(sem_mem);
                    //enter
                    prints("C", i, 1, 0, 0, output);
                    sem_post(sem_prints);
                }
            }
            else{
                if(*parent_waiting > 0) {
                    /*************dítě nebrání rodičovi v odchodu*********/
                    sem_wait(sem_mem);
                    (*waiting_for_parent)++;
                    sem_post(sem_mem);

                    sem_wait(sem_waiting_for_parent);
                    sem_wait(sem_child_cap);

                    sem_wait(sem_prints);
                    sem_wait(sem_mem);
                    (*children_in)++;
                    sem_post(sem_mem);
                    //enter
                    prints("C", i, 1, 0, 0, output);
                    sem_post(sem_prints);
                }
                else{

                    sem_wait(o_enter);
                    sem_wait(sem_child_cap);
                    sem_wait(sem_prints);
                    sem_wait(sem_mem);
                    (*children_in)++;
                    sem_post(sem_mem);
                    //enter
                    prints("C", i, 1, 0, 0, output);
                    sem_post(sem_prints);
                }
            }


            usleep(CWT * 1000);

            //trying to leave + ošetření výpisu
            sem_wait(sem_prints);
            prints("C", i, 2, 0, 0, output);
            //leave
            (*children_in)--;
            prints("C", i, 3, 0, 0, output);
            // pokud pustí rodiče, tak ho zrovna on blokoval
            if(*parent_waiting && *adults_in*3-2 > *children_in){
                sem_post(sem_adult_leaving);
                sem_post(sem_child_cap);
                sem_post(sem_prints);
            }
            else{ //jinak může vpustit další dítě
                if(*children_waiting > 0)
                    sem_post(w_enter);
                else
                    sem_post(o_enter);
                sem_post(sem_child_cap);
                sem_post(sem_prints);
            }


            ////////////////////////////////////////////////////////
            //hromadný finish
            sem_wait(sem_mem);
            (*finishing)++;
            sem_post(sem_mem);

            if(*finishing == A + C)
                sem_post(sem_finish);
            sem_wait(sem_finish);
            sem_post(sem_finish);
            prints("C", i, 5, 0, 0, output);
            exit(0);
        }

    }
    waitpid(pid, NULL, 0);
}

/**
 * Generator rodicu
 * @param A Celkovy pocet rodicu
 * @param AGT Cas, po kterem se vygeneruje novy rodic
 * @param AWT Cas, po kterem se rodic pokousi opustit kritickou zonu
 */
void parent_gen(int A, int C, int AGT, int AWT, FILE *output){
    pid_t pid;
    for(int i = 1; i <= A; i++){
        if(AGT > 0) {
            usleep((rand() % (AGT+1)) * 1000);
        }
        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }

        if(pid == 0) {
            //omezování výpisů
            int boolean = 1;
            //started
            sem_wait(sem_prints);
            prints("A", i, 0, 0, 0, output);
            sem_post(sem_prints);

            //enter
            sem_wait(sem_prints);
            prints("A", i, 1, 0, 0, output);
            sem_wait(sem_mem);
            (*adults_in)++;
            sem_post(sem_mem);
            //vytvoření 3 míst
            printf("%d waiting children | %d no parent\n", *children_waiting, *waiting_for_parent);
            if(*waiting_for_parent > 0){
                if(*waiting_for_parent > 3){
                    sem_wait(sem_mem);
                    for(int j = 0; j < 3; j++){
                        sem_post(sem_waiting_for_parent);
                        sem_post(sem_child_cap);
                        (*waiting_for_parent)--;
                    }
                    sem_post(sem_mem);
                    sem_post(sem_prints);
                }
                else{
                    int j = 0;
                    for(; j < *waiting_for_parent; j++){
                        sem_post(sem_waiting_for_parent);
                        sem_post(sem_child_cap);
                    }
                    for(; j < 3; j++){
                        sem_post(sem_waiting_for_parent);
                        sem_post(sem_child_cap);
                    }
                    sem_wait(sem_mem);
                    (*waiting_for_parent) = 0;
                    sem_post(sem_mem);
                    sem_post(sem_prints);
                }
            }
            else if(*children_waiting > 0){
                if(*children_waiting > 3){
                    sem_wait(sem_mem);
                    for (int j = 0; j < 3; j++) {
                        sem_post(w_enter);
                        sem_post(sem_child_cap);
                        (*children_waiting)--;
                    }
                    sem_post(sem_mem);
                    sem_post(sem_prints);
                }
                else{
                    int j = 0;
                    for(; j < *parent_waiting; j++){
                        sem_post(w_enter);
                        sem_post(sem_child_cap);
                    }
                    for(; j < 3; j++){
                        sem_post(o_enter);
                        sem_post(sem_child_cap);
                    }
                    sem_wait(sem_mem);
                    (*children_waiting) = 0;
                    sem_post(sem_mem);
                    sem_post(sem_prints);
                }
            }
            else{
                for(int j = 0; j < 3; j++){
                    sem_post(o_enter);
                    sem_post(sem_child_cap);
                }
                sem_post(sem_prints);
            }


            usleep(AWT * 1000);


            sem_wait(sem_prints);
            prints("A", i, 2, 0, 0, output);
            //printf("%d <= %d\n", *adults_in*3-2, *children_in);
            if(*children_in && *adults_in*3-2 <= *children_in){
                prints("A", i, 4, *adults_in, *children_in, output);
                boolean = 0;
                sem_post(sem_prints);

                sem_wait(sem_mem);
                (*parent_waiting)++;
                sem_post(sem_mem);

                sem_wait(sem_adult_leaving);

                sem_wait(sem_mem);
                (*parent_waiting)--;
                sem_post(sem_mem);
            }

            // zrušení 3 míst
            for(int j = 0; j < 3; j++) {
                sem_wait(sem_child_cap);
            }
            (*adults_in)--;
            (*total_adults_finished)++;
            prints("A", i, 3, 0, 0, output);

            if(boolean)
                sem_post(sem_prints);

            if(A == *total_adults_finished){
                for (int j = 0; j < C; j++) {
                    sem_post(w_enter);
                    sem_post(o_enter);
                    sem_post(sem_waiting_for_parent);
                    sem_post(sem_child_cap);
                }
            }





            ///////////////////////////////////////////
            // hromadný finish
            sem_wait(sem_mem);
            (*finishing)++;
            sem_post(sem_mem);
            if(*finishing == A + C) {
                sem_post(sem_finish);
            }
            sem_wait(sem_finish);
            sem_post(sem_finish);
            prints("A", i, 5, 0, 0, output);
            exit(0);
        }

    }
    waitpid(pid, NULL, 0);
}

/**
 * Funkce pro vypisy
 * @param type Index vypisu [started, enter, trying to leave, leave, waiting, finished]
 * @param number Cislo procesu
 * @param CA Parametr pro waiting
 * @param CC Parametr pro waiting
 */
void prints(char *kind, int number, int type, int CA, int CC, FILE * out){
    sem_wait(sem_mem);
    (void)out;
    if(type == 4)
        fprintf(stdout, "%d\t: %s %d\t: %s : %d : %d\n", (*count)++, kind, number, types[type], CA, CC);
    else
        fprintf(stdout, "%d\t: %s %d\t: %s\n", (*count)++, kind, number, types[type]);
    fflush(stdout);
    sem_post(sem_mem);
}

/**
 * Vytvoreni pameti a semaforu + prirazeni hodnot
 */
void init(){
    // vytvoření sdílené paměti a semaforu
    count = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    children_in = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    adults_in = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    finishing = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    total_adults_finished = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    children_waiting = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    parent_waiting = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    stopped_child = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    waiting_for_parent = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem_child_cap = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_mem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_finish = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_prints = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_child_queue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_adult_leaving = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_waiting_for_parent = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_child_stopped = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_cap_changing = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    w_enter = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    o_enter = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    val = malloc(sizeof(int));

    // inicializace paměti a semaforů
    if(sem_init(sem_finish, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(sem_prints, 1, 1) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(sem_child_cap, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(sem_mem, 1, 1) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(o_enter, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(w_enter, 1, 1) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(sem_waiting_for_parent, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }



    if(sem_init(sem_adult_leaving, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }
    if(sem_init(sem_child_queue, 1, 0) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }

    if(sem_init(sem_cap_changing, 1, 1) == -1){
        fprintf(stderr, "Semafor selhal\n");
        exit(2);
    }

    *finishing = 0;
    *children_in = 0;
    *adults_in = 0;
    *count = 1;
    *total_adults_finished = 0;
    *children_waiting = 0;
    *parent_waiting = 0;
    *stopped_child = 0;
    *waiting_for_parent = 0;
}

void destroy(){
    // zničení semaforů a uvolnění paměti
    sem_destroy(sem_child_cap);
    sem_destroy(sem_mem);
    sem_destroy(sem_finish);
    sem_destroy(sem_prints);
    sem_destroy(sem_child_queue);
    sem_destroy(sem_adult_leaving);
    sem_destroy(sem_waiting_for_parent);
    sem_destroy(sem_child_stopped);
    sem_destroy(sem_cap_changing);
    sem_destroy(w_enter);
    sem_destroy(o_enter);

    munmap(sem_child_cap, sizeof(sem_t));
    munmap(sem_mem, sizeof(sem_t));
    munmap(sem_finish, sizeof(sem_t));
    munmap(sem_prints, sizeof(sem_t));
    munmap(sem_child_queue, sizeof(sem_t));
    munmap(sem_adult_leaving, sizeof(sem_t));
    munmap(sem_waiting_for_parent, sizeof(sem_t));
    munmap(sem_child_stopped, sizeof(sem_t));
    munmap(sem_cap_changing, sizeof(sem_t));
    munmap(w_enter, sizeof(sem_t));
    munmap(o_enter, sizeof(sem_t));

    free(val);
}