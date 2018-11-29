// Compiling:
//    module load intel
//    mpicc matmul-assign.c -o matmul
// Executing:
//    mpiexec -n 2 matmul
// Sbatch execution:
//    sbatch script.matmul

#include "stdio.h"
#include "mpi.h"

#define N 81
#define P 9
#define SQRP 3

void
main(int argc, char *argv[]) {
  FILE *f;
  int i, j, k, error, rank, size;
  int a[N][N], b[N][N], c[N][N], myc[SQRP][SQRP], mya[N/P][N], myb[SQRP][SQRP];
  MPI_Request sendreq, rcvreq;
  MPI_Status status;


  MPI_Init(&argc, &argv);
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );


  // sequential MM
  if (rank == 0) {

    // initialize a and b
    int val = 0;
    for (i = 0; i<N; i++) {
      for (j = 0; j<N; j++) {
	a[i][j] = val++;
	b[i][j] = val++;
      }
    }
  }

  // TODO: Scatter 3x3 tiles to mya and myb

  for (i=0; i<SQRP; i++) {
    for (j=0; j<SQRP; j++) {
      printf("RANK: %d, a[%d][%d] = %d, b = %d\n", rank, i,j,mya[i][j],myb[i][j]);
    }
  }
  MPI_Finalize();
}

