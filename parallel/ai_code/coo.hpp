#include <iostream>
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
        cout << "rows: " << rows << endl;
        cout << "cols: " << cols << endl;
        cout << "nnz: " << nnz << endl;
        cout << "row_indices: ";
        for (int i = 0; i < nnz; i++)
        {
            cout << row_indices[i] << " ";
        }
        cout << endl;
        cout << "col_indices: ";
        for (int i = 0; i < nnz; i++)
        {
            cout << col_indices[i] << " ";
        }
        cout << endl;
        cout << "values: ";
        for (int i = 0; i < nnz; i++)
        {
            cout << values[i] << " ";
        }
        cout << endl;
    }
};

// A function to multiply two sparse matrices in COO format and return the result in COO format
SparseMatrix multiply(SparseMatrix A, SparseMatrix B)
{

    // Check if the matrices are compatible for multiplication
    if (A.cols != B.rows)
    {
        throw invalid_argument("Matrices cannot be multiplied");
    }

    // Initialize an empty result matrix with the same dimensions as the product of A and B
    SparseMatrix C(A.rows, B.cols, 0);

    // Loop through each non-zero element of A
    for (int i = 0; i < A.nnz; i++)
    {

        // Loop through each non-zero element of B
        for (int j = 0; j < B.nnz; j++)
        {

            // If the column index of A matches the row index of B
            if (A.col_indices[i] == B.row_indices[j])
            {

                // Compute the product of the corresponding values and add it to the corresponding position in C
                int prod = A.values[i] * B.values[j];
                int row = A.row_indices[i];
                int col = B.col_indices[j];

                // Check if C already has a non-zero element at that position
                bool found = false;
                for (int k = 0; k < C.nnz; k++)
                {
                    if (C.row_indices[k] == row && C.col_indices[k] == col)
                    {
                        found = true;

                        // If yes, then update its value by adding the product
                        C.values[k] += prod;
                        break;
                    }
                }

                // If not, then append a new non-zero element to C with the product value and the position indices
                if (!found)
                {
                    C.nnz++;
                    C.row_indices.push_back(row);
                    C.col_indices.push_back(col);
                    C.values.push_back(prod);
                }
            }
        }
    }

    return C;
}