#include <pthread.h>
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

using namespace std;

pthread_mutex_t mutex;

typedef struct _thread_arg
{
    int matrix_size;
    int blocking_factor;
    int **x_matrix;
    int **y_matrix;
    int **z_matrix; // matrix multiplication

    int blocks_num;
    int *block_id; // workload: which blocks the thread should handle

    // for core binding:
    char tag[10];             // thread name(tag)
    pthread_t pid;            // thread id
    int core_id;              // bind thread to core id
    void *(*run)(void *args); // thread routine(non-binding part)

    _thread_arg()
    {
        matrix_size = blocking_factor = blocks_num = 0;
        x_matrix = y_matrix = z_matrix = NULL;
        block_id = NULL;

        // for core binding:
        run = NULL;
        pid = core_id = 0;
    };
} thread_arg;

int ** gen_matrix(int matrix_size, float sparse_factor)
{
    bool flag = false;  // assert that at least one non-zero element can be found
    int ** matrix = new int * [matrix_size];
    for (int i = 0; i < matrix_size; ++i)
        matrix[i] = new int [matrix_size];
    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j) {
            float possibility = rand() % 100 / (float) (100);
            matrix[i][j] = (possibility < sparse_factor);
            if (possibility < sparse_factor)
                flag = true;
        }
    assert(flag);  // assert that at least one non-zero element can be found
    return matrix;
}

void destructor(int **matrix, int matrix_size)
{
    for (int i = 0; i < matrix_size; ++i)
        delete[] matrix[i];
    delete[] matrix;
}

void print_matrix(int **matrix, int matrix_size)
{
    for (int i = 0; i < matrix_size; ++i)
    {
        for (int j = 0; j < matrix_size; ++j)
            cout << matrix[i][j] << " ";
        cout << endl;
    }
}

// for correctness
bool benchmark(int matrix_size, int **x_matrix, int **y_matrix, int **z_matrix)
{
    int **benchmark_matrix = gen_matrix(matrix_size, 0.5);
    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
        {
            int r = 0;
            for (int k = 0; k < matrix_size; ++k)
                r += y_matrix[i][k] * z_matrix[k][j];
            benchmark_matrix[i][j] = r;
        }

    cout << "=== BENCHMARK_MATRIX ===" << endl;
    print_matrix(benchmark_matrix, matrix_size);
    cout << endl;

    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
            if (benchmark_matrix[i][j] != x_matrix[i][j])
            {
                destructor(benchmark_matrix, matrix_size);
                return false;
            }

    destructor(benchmark_matrix, matrix_size);

    return true;
}

// wrapper in each thread for core binding
static void *wrapper(void *args)
{
    thread_arg *obj = (thread_arg *)args;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (obj->core_id == sysconf(_SC_NPROCESSORS_CONF))  // no binding
        for (int i = 1; i < sysconf(_SC_NPROCESSORS_CONF); ++i)
            CPU_SET(i, &cpuset);
    else  // binding
        CPU_SET(obj->core_id, &cpuset);

    pthread_setaffinity_np(obj->pid, sizeof(cpu_set_t), &cpuset);
    cout << obj->tag << " starts running..." << endl;
    obj->run(args);
    return NULL;
}

static void *thread_routine(void *arg)
{
    thread_arg *argu = (thread_arg *)arg; // parse the arguments
    int matrix_size = argu->matrix_size;
    int blocking_factor = argu->blocking_factor;
    int **x_matrix = argu->x_matrix;
    int **y_matrix = argu->y_matrix;
    int **z_matrix = argu->z_matrix;
    int blocks_num = argu->blocks_num;
    int *block_id = argu->block_id;

    for (int i = 0; i < blocks_num; ++i)
    { // each iteration handles one block
        // calculate the starting row and column: jj, kk
        int id = block_id[i];
        int blocks_per_row = matrix_size / blocking_factor;
        int jj = floor(id / blocks_per_row) * blocking_factor;
        int kk = (id % blocks_per_row) * blocking_factor;

        for (int i = 0; i < matrix_size; ++i)
            // all rows in x_matrix are affected
            for (int k = kk; k < kk + blocking_factor; k++)
            { // update elements in certain columns
                int r = 0;
                for (int j = 0; j < blocking_factor; j++)
                    // number of elements to be added
                    r += y_matrix[i][jj + j] * z_matrix[jj + j][k];
                pthread_mutex_lock(&mutex);
                x_matrix[i][k] += r;
                pthread_mutex_unlock(&mutex);
                // fine-grained locks for refinement
            }
    }

    // to check the correctness of core binding (use top)
    // for (;;)
    //     sleep(1);

    return NULL;
}

int stoi(char *a)
{
    stringstream b(a);
    int x;
    b >> x;
    return x;
}

int main(int argc, char **argv)
{
    // bind main thread to core0 anyway
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    // usage: ./dense.out [matrix size] [blocking factor] [threads number] [binding] [sparse factor]
    assert(argc == 6); // then, parse the parameters
    int matrix_size = stoi(argv[1]);
    int blocking_factor = stoi(argv[2]);
    int threads_number = stoi(argv[3]);
    int binding = stoi(argv[4]);
    assert(matrix_size % blocking_factor == 0);

    float sparse_factor = stod(argv[5]);

    cout << "=== INPUT ===" << endl;
    cout << "matrix size: " << matrix_size << " blocking factor: " << blocking_factor << " threads number: " << threads_number
         << " binding: " << binding << " sparse factor: " << sparse_factor << endl << endl;

    cout << "=== CPU_NUM ===" << endl;
    cout << "system cpu number: " << sysconf(_SC_NPROCESSORS_CONF) << endl << endl;

    srand(time(NULL)); // matrix elements are random

    pthread_mutex_init(&mutex, NULL);

    int **y_matrix = gen_matrix(matrix_size, sparse_factor);
    cout << "=== Y_MATRIX ===" << endl;
    print_matrix(y_matrix, matrix_size);
    cout << endl;

    int **z_matrix = gen_matrix(matrix_size, sparse_factor);
    cout << "=== Z_MATRIX ===" << endl;
    print_matrix(z_matrix, matrix_size);
    cout << endl;

    int **x_matrix = gen_matrix(matrix_size, sparse_factor); // multiplication: X = Y * Z

    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
            x_matrix[i][j] = 0;

    // sequential implementation:

    // for (int jj = 0; jj < matrix_size; jj += blocking_factor)  // jj: starting row number in blocks of z_matrix
    //     for (int kk = 0; kk < matrix_size; kk += blocking_factor)  // kk: starting column number in blocks of z_matrix
    //         for (int i = 0; i < matrix_size; ++i)  // all rows in x_matrix are affected
    //             for (int k = kk; k < kk + blocking_factor; k++) {  // update elements in certain columns
    //                 int r = 0;
    //                 for (int j = 0; j < blocking_factor; j++)  // number of elements to be added
    //                     r += y_matrix[i][jj + j] * z_matrix[jj + j][k];
    //                 x_matrix[i][k] += r;
    //             }

    // parallel implementation:

    int num_of_blocks = (matrix_size / blocking_factor) * (matrix_size / blocking_factor);
    int blocks_per_thread = floor(num_of_blocks / threads_number);
    thread_arg *args = new thread_arg[threads_number]; // thread argument

    // prepare arguments
    for (int i = 0; i < threads_number; ++i)
    {
        args[i].matrix_size = matrix_size;
        args[i].blocking_factor = blocking_factor;
        args[i].x_matrix = x_matrix;
        args[i].y_matrix = y_matrix;
        args[i].z_matrix = z_matrix;

        if (i != threads_number - 1)
            args[i].blocks_num = blocks_per_thread;
        else
            args[i].blocks_num = num_of_blocks - blocks_per_thread * (threads_number - 1);

        args[i].block_id = new int[args[i].blocks_num];
        // then, assign block id to thread
        for (int j = 0; j < args[i].blocks_num; ++j)
        {
            args[i].block_id[j] = i * blocks_per_thread + j;
        }

        // for core binding:
        args[i].run = thread_routine;
        string x;
        stringstream t(x);
        t << i;
        strcpy(args[i].tag, ("id-" + x).c_str());
        if (binding)
            args[i].core_id = i % (sysconf(_SC_NPROCESSORS_CONF) - 1) + 1;
        else
            args[i].core_id = sysconf(_SC_NPROCESSORS_CONF);
    }

    for (int i = 0; i < threads_number; ++i)
        pthread_create(&args[i].pid, NULL, wrapper, (void *)&args[i]);

    for (int i = 0; i < threads_number; ++i)
        pthread_join(args[i].pid, NULL);

    for (int i = 0; i < threads_number; ++i)
        delete[] args[i].block_id;

    delete[] args;

    pthread_mutex_destroy(&mutex);

    cout << "=== X_MATRIX ===" << endl;
    print_matrix(x_matrix, matrix_size);
    cout << endl;

    assert(benchmark(matrix_size, x_matrix, y_matrix, z_matrix));

    destructor(x_matrix, matrix_size);
    destructor(y_matrix, matrix_size);
    destructor(z_matrix, matrix_size);

    return 0;
}
