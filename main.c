#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define OUTPUT "proj2.out"

void child_gen(int C, int CGT, int CWT, FILE * output);
void parent_gen(int A, int AGT, int AWT, int C, FILE * output);
void prints(char *kind, int number, int type, int CA, int CC, FILE * out);
void init();
void destroy();


// semafory
sem_t *sem_adults, *sem_mem, *sem, *sem_finish, *sem_finish2, *sem_child_enter, *sem_leaving, *sem_queue;
sem_t *sem_vars, *sem_child_enter2, *sem_adults_ended;

// sdílená paměť
int *count, *adults_in, *children_in;

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

    pid_t pid;

    if((pid = fork()) < 0){
        fprintf(stderr, "Fork failed!\n");
        exit(2);
    }

    if (pid == 0) { // child generator
        child_gen(C, CGT, CWT, output);
        exit(0);
    }
    else {
        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }
        if (pid == 0) { // adult generator
            parent_gen(A, AGT, AWT, C, output);
            exit(0);
        }
    }


    while(*val != A+C)
        sem_getvalue(sem_finish2, val);

    for(int i = 0; i < (A + C); i++)
        sem_post(sem_finish);

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
void child_gen(int C, int CGT, int CWT, FILE * output){
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
            prints("C", i, 0, 0, 0, output);
            sem_getvalue(sem_adults_ended, val);
            if(*adults_in*3 <= *children_in && *val == 0){
                prints("C", i, 4, *adults_in, *children_in, output);
                sem_post(sem_child_enter2);
                sem_wait(sem_child_enter);
            }

            sem_wait(sem_leaving);
            sem_wait(sem);
            prints("C", i, 1, 0, 0, output);
            sem_post(sem_leaving);

            sem_wait(sem_vars);
            (*children_in)++;
            sem_post(sem_vars);
            usleep(CWT*1000);

            // trying to leave - leave (waiting) jdoucí za sebou
            sem_wait(sem_leaving);
            prints("C", i, 2, 0, 0, output);
            prints("C", i, 3, 0, 0, output);
            sem_wait(sem_vars);
            (*children_in)--;
            sem_post(sem_vars);
            sem_post(sem_leaving);
            if(*adults_in > 0)
                if(*adults_in*3-2 > *children_in)
                    sem_post(sem_adults);

            sem_post(sem);

            sem_post(sem_finish2);
            sem_wait(sem_finish);
            prints("C", i, 5, 0, 0, output);
            exit(0);
        }
    }
}

/**
 * Generator rodicu
 * @param A Celkovy pocet rodicu
 * @param AGT Cas, po kterem se vygeneruje novy rodic
 * @param AWT Cas, po kterem se rodic pokousi opustit kritickou zonu
 */
void parent_gen(int A, int AGT, int AWT, int C, FILE *output){
    for(int i = 1; i <= A; i++){
        pid_t pid;
        if(AGT > 0) {
            usleep((rand() % (AGT+1)) * 1000);
        }
        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }

        if(pid == 0) {
            int boolean = 0;
            prints("A", i, 0, 0, 0, output);

            sem_wait(sem_leaving);
            prints("A", i, 1, 0, 0, output);
            sem_post(sem_leaving);

            // uvolnění 3 míst
            for (int j = 0; j < 3; j++)
                sem_post(sem);

            sem_wait(sem_vars);
            (*adults_in)++;
            sem_post(sem_vars);


            // val - aktuální počet dětí čekajících na vstup
            sem_getvalue(sem_child_enter2, val);
            if (*val > 0) {
                // vpuštění čekajících děti (max 3), pokud jich je méně než 3, uvolní další místa
                if(*val <= 3){
                    int j;
                    for (j = 0; j < *val; j++){
                        sem_post(sem_child_enter);
                        sem_wait(sem_child_enter2);
                    }
                    for (; j < 3; j++) {
                        sem_post(sem);
                    }
                }
                else{
                    for (int j = 0; j < 3; j++) {
                        sem_post(sem_child_enter);
                        sem_wait(sem_child_enter2);
                    }
                }
            } else {
                // uvolnění 3 míst pokud nečeká žádné dítě
                for (int j = 0; j < 3; j++) {
                    sem_post(sem);
                }
            }
            usleep(AWT * 1000);

            // trying to leave - leave (waiting) jdoucí za sebou
            sem_wait(sem_leaving);
            prints("A", i, 2, 0, 0, output);
            if (*adults_in * 3 <= *children_in + 2) {
                prints("A", i, 4, *adults_in, *children_in, output);
                boolean = 1;
                sem_post(sem_leaving);
                sem_wait(sem_adults);
            }

            // smazání 3 míst
            for (int j = 0; j < 3; j++) {
                sem_wait(sem);
            }
            prints("A", i, 3, 0, 0, output);
            sem_wait(sem_vars);
            (*adults_in)--;
            sem_post(sem_vars);
            if (boolean == 0)
                sem_post(sem_leaving);

            sem_post(sem_finish2);
            sem_wait(sem_finish);
            prints("A", i, 5, 0, 0, output);
            exit(0);
        }

        sleep(1);

        sem_post(sem_adults_ended);
        for (int j = 0; j < C; j++){
            sem_post(sem);
            sem_post(sem_child_enter);
        }
    }
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
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_adults = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_mem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_finish = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_finish2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_child_enter = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_child_enter2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_leaving = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_queue = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_vars = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_adults_ended = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    val = malloc(sizeof(int));

    // inicializace paměti a semaforů
    sem_init(sem, 1, 0);
    sem_init(sem_adults, 1, 0);
    sem_init(sem_mem, 1, 1);
    sem_init(sem_finish, 1, 0);
    sem_init(sem_finish2, 1, 0);
    sem_init(sem_child_enter, 1, 0);
    sem_init(sem_child_enter2, 1, 0);
    sem_init(sem_leaving, 1, 1);
    sem_init(sem_queue, 1, 1);
    sem_init(sem_vars, 1, 1);
    sem_init(sem_adults_ended, 1, 0);

    *children_in = 0;
    *adults_in = 0;
    *count = 1;
}

void destroy(){
    // zničení semaforů a uvolnění paměti
    sem_destroy(sem);
    sem_destroy(sem_mem);
    sem_destroy(sem_adults);
    sem_destroy(sem_finish2);
    sem_destroy(sem_finish);
    sem_destroy(sem_child_enter);
    sem_destroy(sem_child_enter2);
    sem_destroy(sem_leaving);
    sem_destroy(sem_queue);
    sem_destroy(sem_vars);
    sem_destroy(sem_adults_ended);

    munmap(sem, sizeof(sem_t));
    munmap(sem_mem, sizeof(sem_t));
    munmap(sem_adults, sizeof(sem_t));
    munmap(sem_finish, sizeof(sem_t));
    munmap(sem_finish2, sizeof(sem_t));
    munmap(sem_child_enter, sizeof(sem_t));
    munmap(sem_child_enter2, sizeof(sem_t));
    munmap(sem_leaving, sizeof(sem_t));
    munmap(sem_queue, sizeof(sem_t));
    munmap(sem_vars, sizeof(sem_t));
    munmap(sem_adults_ended, sizeof(sem_t));

    free(val);
}