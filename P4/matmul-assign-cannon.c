// Compiling:
//    module load intel
//    mpicc matmul-assign-cannon.c -o matmul-cannon
// Executing:
//    mpiexec -n 9 ./matmul-cannon
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
  float a[N][N], b[N][N], c[N][N], myc[SQRP][SQRP], mya[SQRP][SQRP], myb[SQRP][SQRP], tmpdata;
  MPI_Request sendreq, rcvreq1, rcvreq2;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // sequential MM
  if (rank == 0)
  {

    // read in matrix
    f = fopen("matrixA.dat", "r");
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        error = fscanf(f, "%f", &tmpdata);
        a[i][j] = tmpdata;
        b[i][j] = tmpdata + 1.;
      }
    }
    fclose(f);

    // After computing each point, output sequential results.
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        c[i][j] = 0.;
        for (k = 0; k < N; k++)
        {
          c[i][j] += a[i][k] * b[k][j];
        }
        printf("SEQ: c[%d][%d] = %f\n", i, j, c[i][j]);
      }
    }
  }

  // TODO: Parallel Portion.  Distribute a and b into local copies
  // mya and myb using Scatterv as in website pointed to by Lecture 21.
  // Initialize myc to 0.

  MPI_Datatype block, blocktype;
  int disp[9] = {0, 1, 2, 9, 10, 11, 18, 19, 20};
  int scount[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
  MPI_Type_vector(3, 3, 9, MPI_FLOAT, &block);
  MPI_Type_commit(&block); // not necessary
  MPI_Type_create_resized(block, 0, 3 * sizeof(int), &blocktype);
  MPI_Type_commit(&blocktype); // needed

  int dims[2] = {SQRP, SQRP};
  int periods[2] = {1, 1};
  MPI_Comm cart_comm;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  MPI_Scatterv(a, scount, disp, blocktype, mya, P, MPI_FLOAT, 0, cart_comm);
  MPI_Scatterv(b, scount, disp, blocktype, myb, P, MPI_FLOAT, 0, cart_comm);

  for (i = 0; i < SQRP; i++)
  {
    for (j = 0; j < SQRP; j++)
    {
      myc[i][j] = 0;
    }
  }

  // TODO: Now create Cartesian grid communicator (see website pointed to
  // by Lecture 21 and Sundar-communicators.pdf on Canvas)

  // above

  // TODO: Move a and b data within Cartesian Grid using initial skew
  // operations (see p. 10 of Lecture 20.)

  int a_to, a_from, b_to, b_from;
  int coords[2];
  MPI_Comm_rank(cart_comm, &rank);
  MPI_Cart_coords(cart_comm, rank, 2, coords);
  MPI_Cart_shift(cart_comm, 1, coords[0], &a_to, &a_from);
  MPI_Cart_shift(cart_comm, 0, coords[1], &b_to, &b_from);
  //printf("Rank %d: a to: %d, a from: %d, b to: %d, b from: %d\n", rank, a_to, a_from, b_to, b_from);

  MPI_Sendrecv_replace(mya, P, MPI_FLOAT, a_to, rank, a_from, MPI_ANY_TAG, cart_comm, &status);
  MPI_Sendrecv_replace(myb, P, MPI_FLOAT, b_to, rank, b_from, MPI_ANY_TAG, cart_comm, &status);

  // TODO: Add following loop:
  // for (k=0; k<=SQRP-1; k++} {
  //    CALC: Should be like sequential code, but use
  //          myc, mya, and myb.  Adjust bounds for all loops to SQRP.
  //          (More generally, (N/P/SQRP)).
  //    SHIFT: Shift A leftward and B upward by 1 in Cartesian grid.
  // }

  int iter;
  for (iter = 0; iter < SQRP; iter++)
  {
    //calc
    for (i = 0; i < SQRP; i++)
    {
      for (j = 0; j < SQRP; j++)
      {
        for (k = 0; k < SQRP; k++)
        {
          myc[i][j] += mya[i][k] * myb[k][j];
        }
      }
    }

    //shift
    MPI_Cart_shift(cart_comm, 1, 1, &a_to, &a_from);
    MPI_Cart_shift(cart_comm, 0, 1, &b_to, &b_from);

    MPI_Sendrecv_replace(mya, P, MPI_FLOAT, a_to, rank, a_from, MPI_ANY_TAG, cart_comm, &status);
    MPI_Sendrecv_replace(myb, P, MPI_FLOAT, b_to, rank, b_from, MPI_ANY_TAG, cart_comm, &status);
  }

  // Output local results to compare against sequential
  MPI_Cart_coords(cart_comm, rank, 2, coords);
  for (i = 0; i < SQRP; i++)
  {
    for (j = 0; j < SQRP; j++)
    {
      printf("PAR, RANK %d: c[%d][%d] = %f\n", rank, coords[0]*3 + i, coords[1]*3 + j, myc[i][j]);
    }
  }

  MPI_Finalize();
}
