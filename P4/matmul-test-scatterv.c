// Compiling:
//    module load intel
//    mpicc matmul-test-scatterv.c -o matmul
// Executing:
//    mpiexec -n 9 matmul
// Sbatch execution:
//    sbatch script.matmul

#include "stdio.h"
#include "mpi.h"

#define N 9
#define P 9
#define SQRP 3

void main(int argc, char *argv[])
{
  FILE *f;
  int i, j, k, error, rank, size;
  int a[N][N], b[N][N], c[N][N], myc[SQRP][SQRP], mya[SQRP][SQRP], myb[SQRP][SQRP];
  MPI_Request sendreq, rcvreq;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // sequential MM
  if (rank == 0)
  {

    // initialize a and b
    int val = 0;
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        a[i][j] = val++;
        b[i][j] = val++;
      }
    }
  }

  // TODO: Scatter 3x3 tiles to mya and myb
  MPI_Datatype block, blocktype;
  int disp[9] = {0, 1, 2, 9, 10, 11, 18, 19, 20};
  int scount[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
  MPI_Type_vector(3, 3, 9, MPI_INT, &block);
  MPI_Type_commit(&block); // not necessary
  MPI_Type_create_resized(block, 0, 3*sizeof(int), &blocktype);
  MPI_Type_commit(&blocktype); // needed
  MPI_Scatterv(a, scount, disp, blocktype, mya, P, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv(b, scount, disp, blocktype, myb, P, MPI_INT, 0, MPI_COMM_WORLD);

  int dims[2] = {SQRP, SQRP};
  int periods[2] = {1, 1};
  MPI_Comm cart_comm;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  MPI_Comm col_comm, row_comm;
  int belongs[2] = { 0, 1 };
  MPI_Cart_sub(cart_comm, belongs, &row_comm);
  belongs = { 1, 0 };
  MPI_Cart_sub(cart_comm, belongs, &col_comm);

  MPI_Cart_shift()

  // int /* rank, */ source;
  // MPI_Cart_shift(cart_comm, 0, 1, &rank, &source);
  // MPI_Cart_shift(cart_comm, 1, 1, &rank, &source);

  for (i = 0; i < SQRP; i++)
  {
    for (j = 0; j < SQRP; j++)
    {
      printf("RANK: %d, a[%d][%d] = %d, b = %d\n", rank, i, j, mya[i][j], myb[i][j]);
    }
  }
  MPI_Finalize();
}