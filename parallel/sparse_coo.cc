#include <iostream>
#include <assert.h>
#include <ctime>
#include <cmath>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>

#include <vector>

using namespace std;

// A structure to represent a sparse matrix in COO format
struct SparseMatrix
{
    int rows;                // number of rows
    int cols;                // number of columns
    int nnz;                 // number of non-zero elements
    vector<int> row_indices; // row indices of non-zero elements
    vector<int> col_indices; // column indices of non-zero elements
    vector<int> values;      // values of non-zero elements

    // Constructor to initialize a sparse matrix with given dimensions and nnz
    SparseMatrix(int r, int c, int n)
    {
        rows = r;
        cols = c;
        nnz = n;
        row_indices.resize(nnz);
        col_indices.resize(nnz);
        values.resize(nnz);
    }

    // A function to print the sparse matrix in COO format
    void print()
    {
        cout << "=== COO_SPARSE_MATRIX ===" << endl;

        cout << "row_indices: ";
        for (int i = 0; i < nnz; i++)
            cout << row_indices[i] << " ";
        cout << endl;

        cout << "col_indices: ";
        for (int i = 0; i < nnz; i++)
            cout << col_indices[i] << " ";
        cout << endl;

        cout << "values: ";
        for (int i = 0; i < nnz; i++)
            cout << values[i] << " ";
        cout << endl << endl;
    }
};

SparseMatrix * to_COO(int ** matrix, int matrix_size) {
    int nnz = 0;
    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
            if (matrix[i][j])
                nnz++;
    SparseMatrix * res = new SparseMatrix(matrix_size, matrix_size, nnz);  // should be deleted later
    nnz = 0;
    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
            if (matrix[i][j]) {
                res->row_indices[nnz] = i;
                res->col_indices[nnz] = j;
                res->values[nnz] = matrix[i][j];
                nnz++;
            }
    res->print();  // check
    return res;
}

void from_COO(int ** matrix, int matrix_size, SparseMatrix * mat_coo) {
    for (int i = 0; i < mat_coo->nnz; ++i)
        matrix[mat_coo->row_indices[i]][mat_coo->col_indices[i]] = mat_coo->values[i];
}

pthread_mutex_t mutex;

typedef struct _thread_arg {
    SparseMatrix * x_coo;
    SparseMatrix * y_coo;
    SparseMatrix * z_coo;
    int nnz_num_per_thread, thread_id, last;

    // for core binding:
    char tag[10];  // thread name(tag)
    pthread_t pid;  // thread id
    int core_id;  // bind thread to core id
    void * (* run)(void * args);  // thread routine(non-binding part)

    _thread_arg() {
        x_coo = y_coo = z_coo = nullptr;
        nnz_num_per_thread = thread_id = last = 0;

        // for core binding:
        run = nullptr;
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

void destructor(int ** matrix, int matrix_size)
{
    for (int i = 0; i < matrix_size; ++i)
        delete [] matrix[i];
    delete [] matrix;
}

void print_matrix(int ** matrix, int matrix_size)
{
    for (int i = 0; i < matrix_size; ++i) {
        for (int j = 0; j < matrix_size; ++j)
            cout << matrix[i][j] << " ";
        cout << endl;
    }
}

// for correctness
bool benchmark(int matrix_size, int ** x_matrix, int ** y_matrix, int ** z_matrix)
{
    int ** benchmark_matrix = gen_matrix(matrix_size, 0.5);
    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j) {
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
            if (benchmark_matrix[i][j] != x_matrix[i][j]) {
                destructor(benchmark_matrix, matrix_size);
                return false;
            }

    destructor(benchmark_matrix, matrix_size);

    return true;
}

// wrapper in each thread for core binding
static void * wrapper(void * args) {
    thread_arg * obj = (thread_arg *) args;
    pthread_setname_np(obj->pid, obj->tag);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(obj->core_id, &cpuset);
    pthread_setaffinity_np(obj->pid, sizeof(cpu_set_t), &cpuset);
    cout << obj->tag << " starts running..." << endl;
    obj->run(args);
    return nullptr;
}

static void * thread_routine(void * arg) {
    thread_arg * argu = (thread_arg *) arg;  // parse the arguments
    SparseMatrix * x_coo = argu->x_coo;
    SparseMatrix * y_coo = argu->y_coo;
    SparseMatrix * z_coo = argu->z_coo;
    int nnz_num_per_thread = argu->nnz_num_per_thread;
    int thread_id = argu->thread_id;
    int last = argu->last;

    int start = nnz_num_per_thread * thread_id;
    int end = (last == 1) ? (y_coo->nnz - 1) : (nnz_num_per_thread * (thread_id + 1) - 1);

    for (int i = start; i <= end; ++i)
        for (int j = 0; j < z_coo->nnz; ++j)
            if (y_coo->col_indices[i] == z_coo->row_indices[j]) {
                int prod = y_coo->values[i] * z_coo->values[j];
                int row = y_coo->row_indices[i], col = z_coo->col_indices[j];

                pthread_mutex_lock(&mutex);

                bool found = false;
                for (int k = 0; k < x_coo->nnz; ++k)
                    if (x_coo->row_indices[k] == row && x_coo->col_indices[k] == col) {
                        found = true;
                        x_coo->values[k] += prod;
                        break;
                    }
                
                if (!found) {
                    x_coo->nnz++;
                    x_coo->row_indices.push_back(row);
                    x_coo->col_indices.push_back(col);
                    x_coo->values.push_back(prod);
                }

                pthread_mutex_unlock(&mutex);
            }

    // to check the correctness of core binding (use top)
    // for (;;)
    //     sleep(1);

    return nullptr;
}

int main(int argc, char ** argv)
{
    // usage: ./dense.out [matrix size] [blocking factor] [threads number] [binding] [sparse factor]
    assert(argc == 6);  // then, parse the parameters
    int matrix_size = stoi(argv[1]);
    int blocking_factor = stoi(argv[2]);
    int threads_number = stoi(argv[3]);
    int binding = stoi(argv[4]);
    assert(matrix_size % blocking_factor == 0);

    // sparse factor input range: 0.01~0.49; if this factor equals 0.50, no sparse effect can be applied
    float sparse_factor = stod(argv[5]);

    cout << "=== INPUT ===" << endl;
    cout << "matrix size: " << matrix_size << " blocking factor: " << blocking_factor << " threads number: " << threads_number
         << " binding: " << binding << " sparse factor: " << sparse_factor << endl << endl;

    cout << "=== CPU_NUM ===" << endl;
    cout << "system cpu number: " << sysconf(_SC_NPROCESSORS_CONF) << endl << endl;

    srand(time(nullptr));  // matrix elements are random

    pthread_mutex_init(&mutex, nullptr);

    int ** y_matrix = gen_matrix(matrix_size, sparse_factor);
    cout << "=== Y_MATRIX ===" << endl;
    print_matrix(y_matrix, matrix_size);
    cout << endl;

    int ** z_matrix = gen_matrix(matrix_size, sparse_factor);
    cout << "=== Z_MATRIX ===" << endl;
    print_matrix(z_matrix, matrix_size);
    cout << endl;

    // switch two-dimensional storage to COO
    SparseMatrix * y_coo = to_COO(y_matrix, matrix_size);
    SparseMatrix * z_coo = to_COO(z_matrix, matrix_size);

    SparseMatrix * x_coo = new SparseMatrix(matrix_size, matrix_size, 0);

    int nnz_per_thread = floor(y_coo->nnz / threads_number);
    thread_arg * args = new thread_arg [threads_number];  // thread argument

    // prepare arguments
    for (int i = 0; i < threads_number; ++i) {
        args[i].x_coo = x_coo;
        args[i].y_coo = y_coo;
        args[i].z_coo = z_coo;
        args[i].thread_id = i;
        args[i].nnz_num_per_thread = nnz_per_thread;
        if (i == threads_number - 1)
            args[i].last = 1;  // marks last thread in the group

        // for core binding:
        args[i].run = thread_routine;
        strcpy(args[i].tag, ("id-" + to_string(i)).c_str());
        if (binding)
            args[i].core_id = i % sysconf(_SC_NPROCESSORS_CONF);
        else
            args[i].core_id = sysconf(_SC_NPROCESSORS_CONF);  // policy to assign threads to different cores
    }

    for (int i = 0; i < threads_number; ++i)
        pthread_create(&args[i].pid, nullptr, wrapper, (void *) & args[i]);

    for (int i = 0; i < threads_number; ++i)
        pthread_join(args[i].pid, nullptr);

    delete [] args;

    pthread_mutex_destroy(&mutex);

    // convert back into the original format
    int ** x_matrix = gen_matrix(matrix_size, 0.5);  // multiplication: X = Y * Z

    for (int i = 0; i < matrix_size; ++i)
        for (int j = 0; j < matrix_size; ++j)
            x_matrix[i][j] = 0;

    from_COO(x_matrix, matrix_size, x_coo);

    cout << "=== X_MATRIX ===" << endl;
    print_matrix(x_matrix, matrix_size);
    cout << endl;

    assert(benchmark(matrix_size, x_matrix, y_matrix, z_matrix));

    destructor(x_matrix, matrix_size);
    destructor(y_matrix, matrix_size);
    destructor(z_matrix, matrix_size);

    delete x_coo;
    delete y_coo;
    delete z_coo;

    return 0;
}
