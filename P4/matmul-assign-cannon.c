// Compiling:
//    module load intel
//    mpicc matmul-assign.c -o matmul
// Executing:
//    mpiexec -n 9 matmul
// Sbatch execution:
//    sbatch script.matmul

#include "stdio.h"
#include "mpi.h"

#define N 9
#define P 9
#define SQRP 3

void
main(int argc, char *argv[]) {
  FILE *f;
  int i, j, k, error, rank, size;
  float a[N][N], b[N][N], c[N][N], myc[SQRP][SQRP], mya[N/P][N], myb[SQRP][SQRP], tmpdata;
  MPI_Request sendreq, rcvreq;
  MPI_Status status;


  MPI_Init(&argc, &argv);
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );


  // sequential MM
  if (rank == 0) {

    // read in matrix
    f = fopen("matrixA.dat","r");
    for (i = 0; i<N; i++) {
      for (j = 0; j<N; j++) {
	error = fscanf(f,"%f",&tmpdata);
	a[i][j] = tmpdata;
	b[i][j] = tmpdata+1.;
      }
    }
    fclose(f);

    // After computing each point, output sequential results.
    for (i = 0; i< N; i++) {
      for (j = 0; j<N; j++) {
	c[i][j] = 0.;
	for (k=0; k<N; k++) {
	  c[i][j] += a[i][k] * b[k][j];
	}
	printf("SEQ: c[%d][%d] = %f\n", i,j,c[i][j]);
      }
    }
  }

  // TODO: Parallel Portion.  Distribute a and b into local copies
  // mya and myb using Scatterv as in website pointed to by Lecture 21.
  // Initialize myc to 0.

  MPI_Datatype block, blocktype;
  int disp[9] = {0, 1, 2, 9, 10, 11, 18, 19, 20};
  int scount[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
  MPI_Type_vector(3, 3, 9, MPI_INT, &block);
  MPI_Type_commit(&block); // not necessary
  MPI_Type_create_resized(block, 0, 3*sizeof(int), &blocktype);
  MPI_Type_commit(&blocktype); // needed
  MPI_Scatterv(a, scount, disp, blocktype, mya, P, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Scatterv(b, scount, disp, blocktype, myb, P, MPI_INT, 0, MPI_COMM_WORLD);

  for (i = 0; i<SQRP; i++) {
    for (j = 0; j<SQRP; j++) {
      myc[i][j] = 0;
    }
  }

  // TODO: Now create Cartesian grid communicator (see website pointed to
  // by Lecture 21 and Sundar-communicators.pdf on Canvas)
  
  int dims[2] = {SQRP, SQRP};
  int periods[2] = {1, 1};
  MPI Comm cart_comm;
  MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

  // TODO: Move a and b data within Cartesian Grid using initial skew
  // operations (see p. 10 of Lecture 20.)

  int /* rank, */ source;
  MPI_Cart_shift(cart_comm, 0, 1, &rank, &source);

  // TODO: Add following loop:
  // for (k=0; k<=SQRP-1; k++} {
  //    CALC: Should be like sequential code, but use
  //          myc, mya, and myb.  Adjust bounds for all loops to SQRP.
  //          (More generally, (N/P/SQRP)).
  //    SHIFT: Shift A leftward and B upward by 1 in Cartesian grid.
  // }



  // Output local results to compare against sequential
  for (i = 0; i<SQRP; i++) {
    for (j = 0; j<SQRP; j++) {
      printf("PAR, RANK %d: c[%d][%d] = %f\n", rank, (rank/SQRP)+i,(rank % SQRP)+j,myc[i][j]);
    }
  }

  MPI_Finalize();
}

