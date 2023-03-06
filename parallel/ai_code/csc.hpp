#include <assert.h>

// 定义csc格式的稀疏矩阵结构体
struct csc_matrix
{
    int n;        // 矩阵的维度
    int nnz;      // 非零元素的个数
    double *val;  // 非零元素的值数组，按列存储
    int *row_ind; // 非零元素的行索引数组，按列存储
    int *col_ptr; // 每一列的第一个非零元素在val和row_ind中的位置数组，长度为n+1
};

// csc格式的稀疏矩阵乘法函数，返回C = A * B
csc_matrix csc_multiply(csc_matrix A, csc_matrix B)
{
    assert(A.n == B.n); // 确保A和B是同维度的方阵

    // 初始化C为零矩阵
    csc_matrix C;
    C.n = A.n;
    C.nnz = 0;
    C.val = new double[C.n * C.n];
    C.row_ind = new int[C.n * C.n];
    C.col_ptr = new int[C.n + 1];

    // 遍历B的每一列
    for (int j = 0; j < B.n; j++)
    {
        C.col_ptr[j] = C.nnz; // 记录C的第j列的第一个非零元素在val和row_ind中的位置

        // 遍历B的第j列中的每一个非零元素
        for (int k = B.col_ptr[j]; k < B.col_ptr[j + 1]; k++)
        {
            double bjk = B.val[k]; // 获取B(j,k)的值
            int i = B.row_ind[k];  // 获取B(j,k)对应A中第i行

            // 遍历A的第i行中的每一个非零元素
            for (int l = A.col_ptr[i]; l < A.col_ptr[i + 1]; l++)
            {
                double ail = A.val[l]; // 获取A(i,l)的值
                int m = A.row_ind[l];  // 获取A(i,l)对应C中第m行

                // 在C(m,j)处累加A(i,l) * B(j,k)
                bool found = false;
                for (int n = C.col_ptr[j]; n < C.nnz; n++)
                {
                    if (C.row_ind[n] == m)
                    {
                        found = true;
                        C.val[n] += ail * bjk;
                        break;
                    }
                }
                if (!found)
                {
                    C.val[C.nnz] = ail * bjk;
                    C.row_ind[C.nnz] = m;
                    C.nnz++;
                }
            }
        }

        if (j == B.n - 1)
        {
            C.col_ptr[j + 1] = C.nnz;
        }
    }

    // 调整C的val和row_ind的大小，去掉多余的空间
    double *new_val = new double[C.nnz];
    int *new_row_ind = new int[C.nnz];
    for (int i = 0; i < C.nnz; i++)
    {
        new_val[i] = C.val[i];
        new_row_ind[i] = C.row_ind[i];
    }
    delete[] C.val;
    delete[] C.row_ind;
    C.val = new_val;
    C.row_ind = new_row_ind;

    return C; // 返回C
}