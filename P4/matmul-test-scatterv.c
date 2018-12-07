// Compiling:
//    module load intel
//    mpicc matmul-test-scatterv.c -o scatter
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
  MPI_Request sendreq, rcvreq1, rcvreq2;
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
  MPI_Type_create_resized(block, 0, 3 * sizeof(int), &blocktype);
  MPI_Type_commit(&blocktype); // needed

  int dims[2] = {SQRP, SQRP};
  int periods[2] = {1, 1};
  MPI_Comm cart_comm;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  MPI_Scatterv(a, scount, disp, blocktype, mya, P, MPI_INT, 0, cart_comm);
  MPI_Scatterv(b, scount, disp, blocktype, myb, P, MPI_INT, 0, cart_comm);

  int a_to, a_from, b_to, b_from;
  int coords[2];
  MPI_Comm_rank(cart_comm, &rank);
  MPI_Cart_coords(cart_comm, rank, 2, coords);
  MPI_Cart_shift(cart_comm, 1, coords[0], &a_to, &a_from);
  MPI_Cart_shift(cart_comm, 0, coords[1], &b_to, &b_from);
  //printf("Rank %d: a to: %d, a from: %d, b to: %d, b from: %d\n", rank, a_to, a_from, b_to, b_from);

  // MPI_Sendrecv_replace(mya, P, MPI_INT, a_to, rank, a_from, MPI_ANY_TAG, cart_comm, &status);
  // MPI_Sendrecv_replace(myb, P, MPI_INT, b_to, rank, b_from, MPI_ANY_TAG, cart_comm, &status);

  MPI_Isend(mya, N, MPI_INT, a_to, a_from, cart_comm, &sendreq);
  MPI_Isend(myb, N, MPI_INT, b_to, b_from, cart_comm, &sendreq);

  MPI_Irecv(mya, N, MPI_INT, a_from, MPI_ANY_TAG, cart_comm, &rcvreq1);
  MPI_Irecv(myb, N, MPI_INT, b_from, MPI_ANY_TAG, cart_comm, &rcvreq2);

  MPI_Wait(&rcvreq1, &status);
  MPI_Wait(&rcvreq2, &status);

  MPI_Cart_shift(cart_comm, 1, 1, &a_to, &a_from);
  MPI_Cart_shift(cart_comm, 0, 1, &b_to, &b_from);

  MPI_Isend(mya, N, MPI_INT, a_to, a_from, cart_comm, &sendreq);
  MPI_Isend(myb, N, MPI_INT, b_to, b_from, cart_comm, &sendreq);

  MPI_Irecv(mya, N, MPI_INT, a_from, MPI_ANY_TAG, cart_comm, &rcvreq1);
  MPI_Irecv(myb, N, MPI_INT, b_from, MPI_ANY_TAG, cart_comm, &rcvreq2);

  MPI_Wait(&rcvreq1, &status);
  MPI_Wait(&rcvreq2, &status);

  for (i = 0; i < SQRP; i++)
  {
    for (j = 0; j < SQRP; j++)
    {
      printf("RANK: %d, a[%d][%d] = %d, b = %d\n", rank, i, j, mya[i][j], myb[i][j]);
    }
  }
  MPI_Finalize();
}