#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MASTER 0
MPI_Datatype arr;

double *monomial_value(int m, int n, int e[], double x[]);
double determinant(int n, double a_lu[], int pivot[]);
int fa(int n, double a[], int pivot[]);
double vec_sum(int n, double a[]);
double simplex_volume(int m, double t[]);
double *randuniform_vec(int n, int *seed);
double *simplex_unit_sample(int m, int n, int *seed);
void simplex_unit_to_general(int m, int n, double t[], double ref[], double phy[]);
double *simplex_sample(int m, int n, double t[], int *seed);

double *monomial_value(int m, int n, int e[], double x[]) {
    int i;
    int j;
    double *v;

    v = (double *)malloc(n * sizeof(double));

    for (j = 0; j < n; j++) {
        v[j] = 1.0;
    }

    for (i = 0; i < m; i++) {
        if (0 != e[i]) {
            for (j = 0; j < n; j++) {
                v[j] = v[j] * pow(x[i + j * m], e[i]);
            }
        }
    }

    return v;
}

double determinant(int n, double a_lu[], int pivot[]) {
    double det;
    int i;

    det = 1.0;

    for (i = 1; i <= n; i++) {
        det = det * a_lu[i - 1 + (i - 1) * n];
        if (pivot[i - 1] != i) {
            det = -det;
        }
    }

    return det;
}

int fa(int n, double a[], int pivot[]) {
    int i;
    int j;
    int k;
    int l;
    double t;

    for (k = 1; k <= n - 1; k++) {
        /*
          Find L, the index of the pivot row.
        */
        l = k;

        for (i = k + 1; i <= n; i++) {
            if (fabs(a[l - 1 + (k - 1) * n]) < fabs(a[i - 1 + (k - 1) * n])) {
                l = i;
            }
        }

        pivot[k - 1] = l;
        /*
          If the pivot index is zero, the algorithm has failed.
        */
        if (a[l - 1 + (k - 1) * n] == 0.0) {
            fprintf(stderr, "\n");
            fprintf(stderr, "FA - Fatal error!\n");
            fprintf(stderr, "  Zero pivot on step %d\n", k);
            exit(1);
        }
        /*
          Interchange rows L and K if necessary.
        */
        if (l != k) {
            t = a[l - 1 + (k - 1) * n];
            a[l - 1 + (k - 1) * n] = a[k - 1 + (k - 1) * n];
            a[k - 1 + (k - 1) * n] = t;
        }
        /*
          Normalize the values that lie below the pivot entry A(K,K).
        */
        for (i = k + 1; i <= n; i++) {
            a[i - 1 + (k - 1) * n] = -a[i - 1 + (k - 1) * n] / a[k - 1 + (k - 1) * n];
        }
        /*
          Row elimination with column indexing.
        */
        for (j = k + 1; j <= n; j++) {
            if (l != k) {
                t = a[l - 1 + (j - 1) * n];
                a[l - 1 + (j - 1) * n] = a[k - 1 + (j - 1) * n];
                a[k - 1 + (j - 1) * n] = t;
            }

            for (i = k + 1; i <= n; i++) {
                a[i - 1 + (j - 1) * n] = a[i - 1 + (j - 1) * n] + a[i - 1 + (k - 1) * n] * a[k - 1 + (j - 1) * n];
            }
        }
    }

    pivot[n - 1] = n;

    if (a[n - 1 + (n - 1) * n] == 0.0) {
        fprintf(stderr, "\n");
        fprintf(stderr, "FA - Fatal error!\n");
        fprintf(stderr, "  Zero pivot on step %d\n", n);
        exit(1);
    }

    return 0;
}

double vec_sum(int n, double a[]) {
    int i;
    double value;

    value = 0.0;
    for (i = 0; i < n; i++) {
        value = value + a[i];
    }

    return value;
}

double *randuniform_vec(int n, int *seed) {
    int i;
    int i4_huge = 2147483647;
    int k;
    double *r;

    if (*seed == 0) {
        fprintf(stderr, "\n");
        fprintf(stderr, "R8VEC_randuniform - Fatal error!\n");
        fprintf(stderr, "  Input value of SEED = 0.\n");
        exit(1);
    }

    r = (double *)malloc(n * sizeof(double));

    for (i = 0; i < n; i++) {
        k = *seed / 127773;

        *seed = 16807 * (*seed - k * 127773) - k * 2836;

        if (*seed < 0) {
            *seed = *seed + i4_huge;
        }

        r[i] = (double)(*seed) * 4.656612875E-10;
    }

    return r;
}

double *simplex_sample(int m, int n, double t[], int *seed) {
    double *x;
    double *x1;

    x1 = simplex_unit_sample(m, n, seed);

    x = (double *)malloc(m * n * sizeof(double));
    simplex_unit_to_general(m, n, t, x1, x);

    free(x1);

    return x;
}

double simplex_volume(int m, double t[]) {
    double *b;
    double det;
    int i;
    int j;
    int *pivot;
    double volume;

    pivot = (int *)malloc(m * sizeof(int));
    b = (double *)malloc(m * m * sizeof(double));

    for (j = 0; j < m; j++) {
        for (i = 0; i < m; i++) {
            b[i + j * m] = t[i + j * m] - t[i + m * m];
        }
    }

    fa(m, b, pivot);

    det = determinant(m, b, pivot);

    volume = fabs(det);
    for (i = 1; i <= m; i++) {
        volume = volume / (double)(i);
    }

    free(b);
    free(pivot);

    return volume;
}

double *simplex_unit_sample(int m, int n, int *seed) {
    double *e;
    double e_sum;
    double *x;

    x = (double *)malloc(m * n * sizeof(double));

    for (int j = 0; j < n; j++) {
        e = randuniform_vec(m + 1, seed);

        for (int i = 0; i < m + 1; i++) {
            e[i] = -log(e[i]);
        }
        e_sum = vec_sum(m + 1, e);

        for (int i = 0; i < m; i++) {
            x[i + j * m] = e[i] / e_sum;
        }
        free(e);
    }

    return x;
}

void simplex_unit_to_general(int m, int n, double t[], double ref[], double phy[]) {
    for (int point = 0; point < n; point++) {
        for (int dim = 0; dim < m; dim++) {
            phy[dim + point * m] = t[dim + 0 * m];
            for (int vertex = 1; vertex < m + 1; vertex++) {
                phy[dim + point * m] = phy[dim + point * m] + (t[dim + vertex * m] - t[dim + 0 * m]) * ref[vertex - 1 + point * m];
            }
        }
    }

    return;
}

void run(int iter, int rank, int size) {
    const int m = 20;
    const int expsz = 40;
    int chunk, start, end, result_chunk, result_start, result_end, seed;
    int e[20];
    double buff[10] = {0};
    MPI_Status status;

    int exps[20 * 40] = {0, 6, 0, 1, 6, 5, 0, 0, 5, 0, 2, 0, 0, 3, 0, 0, 0, 0, 3, 5, 0, 0, 5, 0, 5, 3, 0, 0, 3, 0, 0, 2, 0, 6, 0, 5, 0, 5, 0, 0, 4, 1, 0, 0, 0, 4, 0, 4, 0, 4,
                         0, 0, 0, 0, 0, 0, 0, 8, 0, 5, 7, 0, 8, 0, 0, 3, 0, 6, 0, 7, 0, 0, 6, 0, 3, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, 3, 0, 6, 5, 0, 4, 4, 0, 4, 0, 7, 6, 0,
                         0, 0, 0, 0, 0, 4, 4, 0, 8, 0, 4, 3, 0, 3, 0, 0, 0, 0, 5, 1, 8, 7, 0, 0, 0, 0, 7, 0, 4, 8, 0, 1, 0, 0, 0, 0, 0, 0, 4, 4, 0, 5, 1, 0, 0, 4, 7, 4, 0, 2,
                         4, 3, 1, 7, 4, 1, 0, 0, 0, 0, 7, 0, 8, 4, 0, 0, 0, 0, 0, 0, 3, 4, 0, 1, 0, 0, 2, 0, 0, 3, 1, 7, 4, 0, 4, 0, 0, 0, 0, 0, 2, 0, 6, 0, 8, 8, 0, 0, 2, 0,
                         0, 0, 0, 0, 8, 3, 7, 4, 5, 2, 0, 2, 0, 0, 0, 4, 6, 0, 0, 0, 0, 2, 0, 0, 1, 0, 7, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 8, 0, 5, 0, 7, 0, 0, 8, 0, 0, 4, 6, 1,
                         3, 2, 0, 0, 2, 0, 0, 0, 5, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 5, 0, 0, 0, 8, 0, 0, 0, 0, 4, 0, 0, 8, 0, 1, 3, 4, 0, 0, 5, 0, 7, 3, 0, 0, 2,
                         0, 0, 0, 0, 7, 0, 4, 8, 0, 0, 7, 0, 0, 5, 4, 0, 3, 0, 0, 0, 5, 5, 6, 0, 5, 2, 0, 0, 0, 5, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 4, 1, 0, 0, 0,
                         0, 8, 0, 0, 4, 0, 0, 7, 0, 1, 8, 8, 0, 0, 0, 3, 5, 0, 8, 3, 0, 0, 0, 6, 6, 0, 7, 1, 8, 8, 0, 1, 0, 0, 0, 0, 3, 8, 0, 0, 4, 6, 5, 4, 0, 0, 8, 0, 0, 0,
                         2, 0, 5, 0, 0, 8, 0, 5, 0, 0, 8, 0, 3, 0, 0, 5, 7, 1, 0, 0, 0, 0, 0, 4, 0, 4, 2, 0, 5, 4, 0, 0, 0, 3, 0, 1, 1, 2, 4, 7, 7, 0, 2, 0, 5, 2, 8, 7, 6, 0,
                         0, 0, 4, 0, 0, 1, 1, 0, 0, 8, 0, 1, 5, 1, 0, 6, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 4, 0, 4, 4, 0, 0, 3, 0, 0, 5, 7, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 3, 0,
                         3, 0, 6, 8, 0, 0, 0, 7, 5, 5, 4, 6, 4, 0, 5, 3, 0, 2, 6, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 5, 0, 4, 8, 0, 0, 2, 6, 0, 8, 0, 0, 0, 6, 8, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 7, 0, 0, 0, 0, 7, 0, 1, 0, 2, 2, 5, 2, 0, 0, 0, 2, 2, 0, 0, 3, 4, 0, 7, 3, 0, 0, 4, 0, 8, 0, 0, 0, 4, 0, 0, 7, 0, 0, 4, 2, 4, 0, 0, 0, 0,
                         7, 0, 0, 0, 0, 5, 6, 0, 0, 3, 0, 4, 4, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 6, 0, 0, 0, 8, 0, 0, 0, 0, 5, 0, 0, 0, 0, 1, 1, 2,
                         1, 0, 0, 0, 2, 4, 3, 0, 0, 0, 0, 8, 3, 3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 8, 7, 0, 3, 0, 7, 0, 0, 1, 5, 0, 0, 0, 0, 3, 0, 0, 2, 2, 0, 5, 4, 2, 0,
                         8, 0, 0, 5, 0, 2, 0, 0, 5, 2, 4, 8, 8, 8, 2, 6, 5, 0, 2, 0, 0, 5, 7, 0, 4, 7, 0, 7, 0, 2, 5, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 4, 0, 0, 3, 0, 4, 0, 7,
                         0, 0, 4, 3, 0, 8, 4, 0, 0, 0, 0, 0, 5, 8, 0, 0, 1, 1, 7, 0, 6, 6, 3, 0, 7, 0, 0, 0, 0, 3, 7, 5, 1, 0, 0, 0, 7, 6, 4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 3, 0};

    double t[20 * 21] = {4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 5.0, 0.0, 5.0, 0.0, 0.0, 1.0, 4.0, 0.0, 0.0, 0.0, 2.0, 4.0, 0.0, 0.0, 4.0, 8.0, 0.0, 7.0, 0.0, 8.0,
                         5.0, 1.0, 5.0, 0.0, 0.0, 0.0, 4.0, 6.0, 0.0, 4.0, 0.0, 2.0, 0.0, 0.0, 8.0, 0.0, 2.0, 5.0, 0.0, 0.0, 2.0, 0.0, 0.0, 2.0, 6.0, 0.0, 0.0, 4.0, 0.0, 1.0,
                         0.0, 0.0, 2.0, 8.0, 0.0, 0.0, 7.0, 0.0, 8.0, 0.0, 6.0, 8.0, 7.0, 0.0, 0.0, 0.0, 0.0, 0.0, 3.0, 1.0, 0.0, 3.0, 4.0, 2.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                         5.0, 0.0, 8.0, 0.0, 8.0, 6.0, 0.0, 6.0, 2.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 5.0, 4.0, 2.0, 4.0, 8.0, 6.0, 4.0, 1.0, 0.0, 7.0, 2.0, 0.0,
                         6.0, 4.0, 5.0, 0.0, 4.0, 0.0, 0.0, 0.0, 6.0, 0.0, 2.0, 6.0, 0.0, 0.0, 7.0, 4.0, 2.0, 0.0, 1.0, 0.0, 0.0, 2.0, 8.0, 2.0, 0.0, 0.0, 0.0, 5.0, 6.0, 8.0,
                         0.0, 5.0, 1.0, 5.0, 0.0, 6.0, 5.0, 4.0, 0.0, 1.0, 5.0, 0.0, 2.0, 6.0, 3.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0, 8.0, 0.0, 0.0, 2.0, 0.0, 3.0, 7.0, 0.0, 0.0,
                         1.0, 6.0, 4.0, 3.0, 3.0, 0.0, 0.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 6.0, 8.0, 4.0, 0.0, 5.0, 0.0, 2.0, 4.0, 0.0, 2.0, 3.0, 7.0, 3.0, 0.0, 7.0, 0.0,
                         0.0, 0.0, 5.0, 0.0, 0.0, 5.0, 8.0, 5.0, 5.0, 7.0, 0.0, 0.0, 6.0, 3.0, 1.0, 2.0, 7.0, 0.0, 1.0, 0.0, 0.0, 7.0, 0.0, 6.0, 8.0, 0.0, 7.0, 7.0, 5.0, 7.0,
                         0.0, 5.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 3.0, 0.0, 0.0, 3.0, 0.0, 0.0, 4.0, 3.0, 6.0, 0.0,
                         4.0, 6.0, 1.0, 4.0, 0.0, 3.0, 1.0, 0.0, 1.0, 2.0, 4.0, 6.0, 0.0, 0.0, 0.0, 8.0, 7.0, 0.0, 0.0, 6.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0,
                         7.0, 5.0, 4.0, 0.0, 0.0, 0.0, 4.0, 5.0, 0.0, 4.0, 2.0, 4.0, 8.0, 1.0, 4.0, 5.0, 5.0, 0.0, 3.0, 0.0, 0.0, 0.0, 2.0, 0.0, 4.0, 0.0, 0.0, 3.0, 0.0, 3.0,
                         5.0, 0.0, 0.0, 2.0, 3.0, 7.0, 0.0, 3.0, 0.0, 0.0, 4.0, 5.0, 4.0, 8.0, 0.0, 0.0, 8.0, 7.0, 1.0, 0.0, 7.0, 7.0, 0.0, 0.0, 8.0, 0.0, 7.0, 0.0, 0.0, 0.0,
                         3.0, 8.0, 0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 6.0, 6.0, 6.0, 0.0, 3.0, 8.0, 0.0, 0.0, 3.0, 0.0, 0.0, 5.0, 0.0, 3.0, 1.0, 0.0, 0.0, 1.0, 2.0, 2.0, 0.0, 4.0,
                         3.0, 2.0, 3.0, 0.0, 0.0, 0.0, 7.0, 8.0, 0.0, 0.0, 6.0, 1.0, 6.0, 0.0, 0.0, 6.0, 0.0, 0.0, 0.0, 1.0, 4.0, 4.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0};
    double *value;
    double *x;

    seed = 123;

    int n = 1;

    double result[40] = {0};

    result_chunk = (expsz + size - 1) / size;
    result_start = result_chunk * rank;
    result_end = result_start + result_chunk;
    while (n <= iter) {
        x = simplex_sample(m, n, t, &seed);
        for (int j = result_start; j < result_end; j++) {
            for (int i = 0; i < m; i++) {
                e[i] = exps[i + j * m];
            }
            value = monomial_value(m, n, e, x);
            result[j] = simplex_volume(m, t) * vec_sum(n, value) / (double)(n);

            free(value);
        }

        free(x);
        n *= 2;
    }
    MPI_Type_contiguous(result_chunk, MPI_DOUBLE, &arr);
    if(rank != MASTER){
        for(int i = result_start; i < result_end; i++){
            MPI_Send(&result[i], 1, MPI_DOUBLE, MASTER, 100, MPI_COMM_WORLD);
        }
    }else {
        for(int it = 1; it < size; it++){
            for(int i = 0; i < result_chunk; i++){
                MPI_Recv(&buff[i], 1, MPI_DOUBLE, it, 100,MPI_COMM_WORLD, &status);
            }
            for(int i = 0;i < result_chunk; i++){
                result[it * result_chunk + i] = buff[i];
            }
        }
    }

    if(rank == MASTER){
        for (int j = 0; j < expsz; j++) {
            printf("\t%g", result[j]);
        }
    }

    return;
}

int main(int argc, char *argv[]) {
    int iter = 0, rank, size;
    clock_t start, stop;
    unsigned long elapsed;

    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == MASTER){
        if (argc == 2) {
            iter = atoi(argv[1]);
            if (!iter) return 1;
        } else {
            iter = 1 << 16;
        }
        start = clock();
    }

    MPI_Bcast(&iter, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    run(iter, rank, size);

    if(rank == MASTER){
        printf("\nSimplex %d\n", iter);

        stop = clock();
        elapsed = (unsigned long)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
        printf("Parallel time elapsed in ms: %lu\n", elapsed);
        clock_t start_sec = clock();
        run(iter,rank,1);
        clock_t end_sec = clock();
        unsigned long elapsed_sec = (unsigned long)(end_sec - start_sec) * 1000.0 / CLOCKS_PER_SEC;
        printf("\nSequential  time elapsed in ms: %lu\n", elapsed_sec);
        elapsed_sec > elapsed ? printf("Test PASSED\n") : printf("Test FAILD\n");
        printf("Normal end of execution.\n");
    }

    MPI_Finalize();
    return 0;
}
