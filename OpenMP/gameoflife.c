#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define for_x for (int x = 0; x < w; x++)
#define for_y for (int y = 0; y < h; y++)
#define for_xy for_x for_y

unsigned** curr_generation;
unsigned** next_generation;

void init(void *u, int w, int h) {

    int(*univ)[w] = u;
    for_xy univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
}

void initParallel(int w, int h) {

    w += 2;
    h += 2;

    curr_generation = malloc(h * sizeof(unsigned*));
    next_generation = malloc(h * sizeof(unsigned*));

    for(int i = 0; i < h; i++){
        curr_generation[i] = malloc(w * sizeof(unsigned));
        next_generation[i] = malloc(w * sizeof(unsigned));
    }

    for(int i = 1; i < h - 1; i++) {
        for(int j = 1; j < w - 1; j++) {
            curr_generation[i][j] = rand() < RAND_MAX / 10 ? 1 : 0;
        }
    }
}

void show(void *u, int w, int h) {
    int(*univ)[w] = u;
    printf("\033[H");
    for_y {
        for_x printf(univ[y][x] ? "\033[07m  \033[m" : "  ");
        printf("\033[E");
    }
    fflush(stdout);
}

void add_border(int w, int h){
    int i;

// #pragma omp parallel for private(i)
//     for(i = 1; i < w - 1; i++){
//         curr_generation[0][i] = curr_generation[h - 2][i];
//     }

// #pragma omp parallel for private(i)
//     for(i = 1; i < w - 1; i++){
//         curr_generation[h - 1][i] = curr_generation[1][i];
//     }

// #pragma omp parallel for private(i)
//     for(i = 1; i < h - 1; i++){
//         curr_generation[i][0] = curr_generation[i][w - 2];
//     }

// #pragma omp parallel for private(i)
//     for(i = 1; i < h - 1; i++) {
//         curr_generation[i][w - 1] = curr_generation[i][1];
//     }

#pragma omp parallel for private(i) schedule(static, 5)
    for(i = 1; i < h - 1; i++) {
        curr_generation[i][w - 1] = curr_generation[i][1];
        curr_generation[i][0] = curr_generation[i][w - 2];
        curr_generation[h - 1][i] = curr_generation[1][i];
        curr_generation[0][i] = curr_generation[h - 2][i];
    }

    curr_generation[0][0] = curr_generation[h - 2][w - 2];
    curr_generation[0][w - 1] = curr_generation[h - 2][1];
    curr_generation[h - 1][0] = curr_generation[1][w - 2];
    curr_generation[h - 1][w - 1] = curr_generation[1][1];
}

unsigned evolve_cell(int i, int j){
    int alive_neighbours = 0;
    if(curr_generation[i - 1][j - 1] == 1) alive_neighbours++;
    if(curr_generation[i - 1][j] == 1) alive_neighbours++;
    if(curr_generation[i - 1][j + 1] == 1) alive_neighbours++;
    if(curr_generation[i][j - 1] == 1) alive_neighbours++;
    if(curr_generation[i][j + 1] == 1) alive_neighbours++;
    if(curr_generation[i + 1][j - 1] == 1) alive_neighbours++;
    if(curr_generation[i + 1][j] == 1) alive_neighbours++;
    if(curr_generation[i + 1][j + 1] == 1) alive_neighbours++;

    if(curr_generation[i][j] == 0){
        if(alive_neighbours == 3) return 1; 
    }else{
        if(alive_neighbours <= 1) return 0;
        else if(alive_neighbours >= 4 && alive_neighbours <= 8) return 0;
    }
}

void evolveParallel(int w, int h) {

    add_border(w, h);
    int i, j = 0;

#pragma omp parallel for private(i, j)
    for(i = 1; i < h - 1;i++){
        for(j = 1; j < w - 1; j++){
            next_generation[i][j] = evolve_cell(i , j); 
        }
    }

#pragma omp parallel for private(i,j)
    for(i = 1; i < h - 1; i++){
        for(j = 1; j < w - 1; j++){
            curr_generation[i][j] = next_generation[i][j];
        }
    }
}

void gameParallel(int w, int h, int iter) {
    for (int i = 0; i < iter; i++) {
#ifdef LIFE_VISUAL
        show(curr_generation, w, h);
#endif
        evolveParallel(w, h);
#ifdef LIFE_VISUAL
        usleep(200000);
#endif
    }
}

void evolve(void *u, int w, int h) {
    unsigned(*univ)[w] = u;
    unsigned new[h][w];

    for_y for_x {
        int n = 0;
        for (int y1 = y - 1; y1 <= y + 1; y1++)
            for (int x1 = x - 1; x1 <= x + 1; x1++)
                if (univ[(y1 + h) % h][(x1 + w) % w]) n++;

        if (univ[y][x]) n--;
        new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
    }
    for_y for_x univ[y][x] = new[y][x];
}

void game(unsigned *u, int w, int h, int iter) {
    for (int i = 0; i < iter; i++) {
#ifdef LIFE_VISUAL
        show(u, w, h);
#endif
        evolve(u, w, h);
#ifdef LIFE_VISUAL
        usleep(200000);
#endif
    }
}

void read_inputs(int c, char* v[], int* w, int* h, int* iter){
    if (c > 1) *w = atoi(v[1]);
    if (c > 2) *h = atoi(v[2]);
    if (c > 3) *iter = atoi(v[3]);
    if (*w <= 0) *w = 30;
    if (*h <= 0) *h = 30;
    if (*iter <= 0) *iter = 1000;
}

unsigned long sequential_execution(int c, char *v[]){
    int w = 0, h = 0, iter = 0;
    unsigned *u;

    if (c > 1) w = atoi(v[1]);
    if (c > 2) h = atoi(v[2]);
    if (c > 3) iter = atoi(v[3]);
    if (w <= 0) w = 30;
    if (h <= 0) h = 30;
    if (iter <= 0) iter = 1000;

    u = (unsigned *)malloc(w * h * sizeof(unsigned));
    if (!u) exit(1);

    init(u, w, h);

    clock_t start = clock();
    game(u, w, h, iter);
    clock_t stop = clock();
    unsigned long elapsed = (unsigned long)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    // printf("\nTime elapsed in ms(sequential execution ): %lu", elapsed);

    free(u);
    return elapsed;
}

unsigned long parallel_execution(int c, char *v[]){
    int w = 0, h = 0, iter = 0;
    read_inputs(c, v, &w, &h, &iter);

    initParallel(w, h);

    clock_t startParallel = clock();
    gameParallel(w, h, iter);
    clock_t stopParallel = clock();
    unsigned long elapsedParallel = (unsigned long)(stopParallel - startParallel) * 1000.0 / CLOCKS_PER_SEC;


    for(int i = 0; i < h; i++){
        free(curr_generation[i]);
        free(next_generation[i]);
    }

    free(curr_generation);
    free(next_generation);

    return elapsedParallel;
}

int main(int c, char *v[]) {
    int w = 0, h = 0, iter = 0;
    unsigned long elapsedSequential = sequential_execution(c, v);
    unsigned long elapsedParallel = parallel_execution(c, v);

    printf("\nTime elapsed in ms(sequential execution ): %lu\n", elapsedSequential);
    printf("\nTime elapsed in ms(parallel execution ): %lu\n", elapsedParallel);

    if(elapsedSequential >= elapsedParallel){
        printf("\nTEST PASSED\n");
    }else{
        printf("\nTEST FAILD\n");
    }

    return 0;
}
