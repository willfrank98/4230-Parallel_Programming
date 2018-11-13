// Compiling: 
//    module load intel
//    mpicc matmul-assign.c -o matmul
// Executing:
//    mpiexec -n 2 ./matmul
// Sbatch execution:
//    sbatch script.matmul

#include "stdio.h"
#include "mpi.h"

#define N 2
// Can adjust N through #define
//#define N 16
#define P 2

void
main(int argc, char *argv[]) {
  FILE *f;
  int i, j, k, error, rank, size;
  float a[N][N], b[N][N], c[N][N], myc[N][N/P], tmpb[N/P][N], mya[N/P][N], myb[N/P][N], tmpdata;
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
   // mya and myb.  Initialize myc to 0.  Initiate exchange of myb into tmpb.


   // TODO: Implement Calc1.  Should be like sequential code, but use
   // myc, mya, and myb.  Adjust bounds for loops corresponding to rows 
   // due to partitioning (i and k).  Index mya appropriately based on 
   // portion of tmpb to be computed.

   // TODO: Make sure you have received tmpb.

   // TODO: Implement Calc2.  Should be like sequential code, but use
   // myc, mya, and tmpb.  Adjust bounds for loops corresponding to rows 
   // due to partitioning (i and k).  Index mya appropriately based on 
   // portion of tmpb to be computed.
   
   // Output local results to compare against sequential
   for (i = 0; i<N/P; i++) {   
      for (j = 0; j<N; j++) {   
	printf("PAR, RANK %d: c[%d][%d] = %f\n", rank, i+rank*N/P,j,myc[i][j]);
      }
   }

   MPI_Finalize();
}

