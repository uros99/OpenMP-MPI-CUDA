#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define for_x for (int x = 0; x < w; x++)
#define for_y for (int y = 0; y < h; y++)
#define for_xy for_x for_y
#define BLOCK_SIZE 4
#define inBounds(x, y, size) ((x > 0 && x < size && y > 0 && y < size) ? true : false)


void init(unsigned *u, int w, int h) {
    for_xy u[x*w + y] = rand() < RAND_MAX / 10 ? 1 : 0;
}

void show(unsigned *u, int w, int h) {
    printf("\033[H");
    for_y {
        for_x printf(u[x*w + y] ? "\033[07m  \033[m" : "  ");
        printf("\033[E");
    }
    fflush(stdout);
}

__global__ void evolve(unsigned* curr_grid, unsigned* next_grid, int size){
    int x = blockIdx.x * BLOCK_SIZE + threadIdx.x;
    int y = blockIdx.y * BLOCK_SIZE + threadIdx.y;
    int index = x * size + y;

    int alive_neighbours = 0;
    if(inBounds(x - 1, y, size) && curr_grid[(x-1) * size + y] == 1) alive_neighbours++;
    if(inBounds(x + 1, y, size) &&  curr_grid[(x+1) * size + y] == 1) alive_neighbours++;
    if(inBounds(x, y - 1, size) &&  curr_grid[x * size + (y - 1)] == 1) alive_neighbours++;
    if(inBounds(x, y + 1, size) &&  curr_grid[x * size + (y + 1)] == 1) alive_neighbours++;
    if(inBounds(x - 1, y - 1, size) &&  curr_grid[(x - 1) * size + (y - 1)] == 1) alive_neighbours++;
    if(inBounds(x - 1, y + 1, size) &&  curr_grid[(x - 1) * size + (y + 1)] == 1) alive_neighbours++;
    if(inBounds(x + 1, y - 1, size) &&  curr_grid[(x + 1) * size + (y - 1)] == 1) alive_neighbours++;
    if(inBounds(x + 1, y + 1, size) &&  curr_grid[(x + 1) * size + (y + 1)] == 1) alive_neighbours++;
    
    if(curr_grid[index] == 0){
        if(alive_neighbours == 3) next_grid[index] = 1;
    }else{
        if(alive_neighbours <= 1) next_grid[index] = 0; // usamljenost
        else if(alive_neighbours >= 4 && alive_neighbours <= 8) next_grid[index] = 0; // prenaseljenost
    }
    
}

// void evolve(unsigned *u, int w, int h) {
//     unsigned(*univ)[w] = u;
//     unsigned new[h][w];

//     for_y for_x {
//         int n = 0;
//         for (int y1 = y - 1; y1 <= y + 1; y1++)
//             for (int x1 = x - 1; x1 <= x + 1; x1++)
//                 if (univ[(y1 + h) % h][(x1 + w) % w]) n++;

//         if (univ[y][x]) n--;
//         new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
//     }
//     for_y for_x univ[y][x] = new[y][x];
// }

// void game(unsigned *u, int w, int h, int iter) {
//     for (int i = 0; i < iter; i++) {
// #ifdef LIFE_VISUAL
//         show(u, w, h);
// #endif
//         evolve(u, w, h);
// #ifdef LIFE_VISUAL
//         usleep(200000);
// #endif
//     }
// }


void read_input(int c, char *v[], int* w, int* h, int* iter){
    if (c > 1) (*w) = atoi(v[1]);
    if (c > 2) (*h) = atoi(v[2]);
    if (c > 3) (*iter) = atoi(v[3]);
    if (*w <= 0) (*w) = 30;
    if (*h <= 0) (*h) = 30;
    if (*iter <= 0) (*iter) = 1000;
}

float parallel_execution(int c, char *v[]){
    int w = 0, h = 0, iter = 0;

    unsigned* host_grid;
    unsigned* device_grid; 
    unsigned* device_next_gen_grid;
    unsigned* device_tmp_grid;

    read_input(c, v, &w,&h,&iter);

    size_t grid_size_bytes = w * h * sizeof(unsigned);

    host_grid = (unsigned *)malloc(grid_size_bytes);
    if (!host_grid) exit(1);

    cudaMalloc(&device_grid, grid_size_bytes);
    cudaMalloc(&device_next_gen_grid, grid_size_bytes);

    init(host_grid, w, h);

    cudaMemcpy(device_grid, host_grid, grid_size_bytes, cudaMemcpyHostToDevice);

    dim3 block_size(BLOCK_SIZE, BLOCK_SIZE);
    dim3 grid_size((w + BLOCK_SIZE - 1) / BLOCK_SIZE, (h + BLOCK_SIZE - 1) / BLOCK_SIZE);

	cudaEvent_t start = cudaEvent_t();
	cudaEvent_t stop = cudaEvent_t();
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);
    // game(host_grid, w, h, iter);

    for (int i = 0; i < iter; i++) {
#ifdef LIFE_VISUAL
        show(host_grid, w, h);
#endif
        evolve<<<grid_size, block_size>>>(device_grid, device_next_gen_grid, w);
        device_tmp_grid = device_grid;
        device_grid = device_next_gen_grid;
        device_next_gen_grid = device_tmp_grid;
        cudaMemcpy(host_grid, device_next_gen_grid, grid_size_bytes, cudaMemcpyHostToDevice);
#ifdef LIFE_VISUAL
        usleep(200000);
#endif
    }

    cudaMemcpy(host_grid, device_grid, grid_size_bytes, cudaMemcpyDeviceToHost);

    float elapsed = 0.f;
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed, start, stop);

    cudaFree(device_grid);
    cudaFree(device_next_gen_grid);
    free(host_grid);

    return elapsed;
}

// float sequential_execution(int c, char *v[]){
//     int w = 0, h = 0, iter = 0;
//     unsigned *u;

//     if (c > 1) w = atoi(v[1]);
//     if (c > 2) h = atoi(v[2]);
//     if (c > 3) iter = atoi(v[3]);
//     if (w <= 0) w = 30;
//     if (h <= 0) h = 30;
//     if (iter <= 0) iter = 1000;

//     u = (unsigned *)malloc(w * h * sizeof(unsigned));
//     if (!u) exit(1);

//     init(u, w, h);
//     game(u, w, h, iter);

//     free(u);
// }


int main(int c, char *v[]) {
    float elapsed = parallel_execution(c, v);
    printf("\nTime elapsed: %lf\n", elapsed);
    // sequential_execution(c, v);
    return 0;
}
