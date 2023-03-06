#include <iostream>
// 定义csr格式的稀疏矩阵结构体
struct csr_matrix
{
    int rows;  // 矩阵行数
    int cols;  // 矩阵列数
    int nnz;   // 非零元素个数
    double *A; // 非零元素值数组
    int *IA;   // 每行第一个非零元素索引数组
    int *JA;   // 非零元素列号数组

    // 构造函数，根据给定参数初始化结构体成员
    csr_matrix(int r, int c, int n, double *a, int *ia, int *ja)
    {
        rows = r;
        cols = c;
        nnz = n;
        A = a;
        IA = ia;
        JA = ja;
    }
};

// 定义csr格式稀疏矩阵乘法函数，输入两个csr格式稀疏矩阵m1和m2，输出结果m3
void csr_mult(csr_matrix &m1, csr_matrix &m2, csr_matrix &m3)
{
    // 检查两个输入矩阵是否可以相乘，即m1的列数是否等于m2的行数
    if (m1.cols != m2.rows)
    {
        std::cout << "Error: incompatible matrix dimensions." << std::endl;
        return;
    }

    // 初始化结果矩阵m3的行数、列数和非零元素个数为0
    m3.rows = m1.rows;
    m3.cols = m2.cols;
    m3.nnz = 0;

    // 动态分配结果矩阵m3所需空间大小（最大可能为m1.nnz * m2.nnz）
    m3.A = new double[m1.nnz * m2.nnz];
    m3.IA = new int[m1.rows + 1];
    m3.JA = new int[m1.nnz * m2.nnz];
    // 遍历m1的每一行
    for (int i = 0; i < m1.rows; i++)
    {
        // 记录m3中当前行的第一个非零元素索引
        m3.IA[i] = m3.nnz;
        // 遍历m2的每一列
        for (int j = 0; j < m2.cols; j++)
        {
            // 初始化m3中当前位置(i,j)的元素值为0
            double val = 0;
            // 遍历m1和m2中当前行和列的非零元素
            for (int k = m1.IA[i]; k < m1.IA[i + 1]; k++)
            {
                for (int l = m2.IA[j]; l < m2.IA[j + 1]; l++)
                {
                    // 如果两个非零元素的列号和行号相等，则计算它们的乘积并累加到val上
                    if (m1.JA[k] == m2.JA[l])
                    {
                        val += m1.A[k] * m2.A[l];
                        break;
                    }
                }
            }
            // 如果val不为0，则将其存储到m3中，并更新非零元素个数和列号数组
            if (val != 0)
            {
                m3.A[m3.nnz] = val;
                m3.JA[m3.nnz] = j;
                m3.nnz++;
            }
        }
    }
    // 记录m3中最后一行的第一个非零元素索引
    m3.IA[m1.rows] = m3.nnz;
}