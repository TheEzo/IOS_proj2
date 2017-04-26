#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define OUTPUT "proj2.out"

void child_gen(int C, int CGT, int CWT);
void parent_gen(int A, int AGT, int AWT);

sem_t *sem;
sem_t *sem2;
int *mem;

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


    srand(time(NULL));

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    mem = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 0);
    sem_init(sem2, 1, 0);



    pid_t pid;

    if((pid = fork()) < 0){
        fprintf(stderr, "Fork failed!\n");
        exit(2);
    }

    if (pid == 0) { // child generator
        child_gen(C, CGT, CWT);

        exit(0);
    }
    else {
        if((pid = fork()) < 0){
            fprintf(stderr, "Fork failed!\n");
            exit(2);
        }
        if (pid == 0) { // adult generator
            parent_gen(A, AGT, AWT);


        }
    }

    for(int i = 0; i < (A + C); i++) {
        sem_post(sem2);
    }

/*for (int i = 0; i < 5; i++) {
                sem_wait(sem);

                printf("Parent: %d\n", *mem);

                sem_post(sem);
                usleep(10000);
            }*/


    // zavrani souboru
    if(fclose(output) == EOF){
        fprintf(stderr, "Soubor %s se nepodarilo zavrit\n", OUTPUT);
        exit(2);
    }

    sem_destroy(sem);
    munmap(mem, sizeof(int));
    munmap(sem, sizeof(sem_t));



    exit(0);
}

/**
 * Generator deti
 * @param C Celkovy pocet deti
 * @param CGT Cas, po kterem se vygeneruje nove dite
 * @param CWT Cas, po kterem dite opusti kritickou zonu
 * @param mem Sdilena pamet
 */
void child_gen(int C, int CGT, int CWT){
    for (int i = 0; i < C; i++) {
        if(CGT > 0) {
            usleep((rand() % (CGT+1)) * 1000);
        }
        printf("%d\t: C %d\t: started\n", (*mem)++, i);
        sem_wait(sem);
        printf("%d\t: C %d\t: enter\n", (*mem)++, i);
        usleep(CWT*100);
        printf("%d\t: C %d\t: leave\n", (*mem)++, i);
        sem_post(sem);
    }
    return;
}

/**
 * Generator rodicu
 * @param A Celkovy pocet rodicu
 * @param AGT Cas, po kterem se vygeneruje novy rodic
 * @param AWT Cas, po kterem se rodic pokousi opustit kritickou zonu
 * @param mem Sdilena pamet
 */
void parent_gen(int A, int AGT, int AWT){
    if(AGT > 0) {
        usleep((rand() % (AGT+1)) * 1000);
    }
    for(int i = 0; i < A; i++){
        printf("%d\t: A %d\t: started\n", (*mem)++, i);
        printf("%d\t: A %d\t: enter\n", (*mem)++, i);
        for(int j = 0; j < 3; j++) {
            sem_post(sem);
        }
        usleep(AWT);
        printf("%d\t: A %d\t: trying to leave\n", (*mem)++, i);
        int *val = (int *)malloc(sizeof(int));
        sem_getvalue(sem,val);
        if(*val < 3)
            printf("%d\t: A %d\t: waiting\n", (*mem)++, i);
            while(*val >=3)
                sem_getvalue(sem, val);
        free(val);
        for(int j = 0; j < 3; j++)
            sem_wait(sem);

        printf("%d\t: A %d\t: leave\n", (*mem)++, i);


        usleep(50000);
    }
    return;
}


/*
    int pid;
    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    pid_t consPID;
    pid_t prodPID;

    sem_t *sem;
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 0);

    // systemove volani - vzdy je vhodne overit uspesnost!
    if ((pid = fork()) < 0) {
        perror("fork");
        exit(2);
    }

    if (pid == 0) { // child
        // cekame, az nam druhy proces dovoli pokracovat
        sem_wait(sem);
        for (int i=0; i<10; i++) {
            fprintf(stdout, "first: %d\n", i);
            usleep(500);
        }
        sem_post(sem);
        fprintf(stdout, "first: end\n");
        exit(0);
    } else { // parent.
        consPID = pid;
        //--
        pid = fork();
        //--
        if (pid == 0) { // child
            for (int i=0; i<10; i++) {
                fprintf(stdout, "second: %d\n", i);
                // po splneni podminky dovolime dalsimu procesu pokracovat
                if (i==5) sem_post(sem);
                usleep(500);
            }
            sem_wait(sem);
            fprintf(stdout, "second: end\n");
            exit(0);
        } else { // parent
            prodPID = pid;
        }
    }

    // pockame az vsichni skonci
    waitpid(consPID, NULL, 0);
    waitpid(prodPID, NULL, 0);

    // zrusime zdroje
    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));
    */