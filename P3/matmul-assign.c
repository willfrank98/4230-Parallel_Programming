// Compiling: 
//    module load intel
//    mpicc matmul-assign.c -o matmul
// Executing:
//    mpiexec -n 2 ./matmul
// Sbatch execution:
//    sbatch script.matmul

#include "stdio.h"
#include "mpi.h"

#define N 16
// Can adjust N through #define
//#define N 16
#define P 2

void
main(int argc, char *argv[]) {
  FILE *f;
  int i, j, k, error, rank, size, valid;
  float a[N][N], b[N][N], c[N][N], myc[N/P][N], tmpb[N/P][N], mya[N/P][N], myb[N/P][N], tmpdata;
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
	   //printf("Doing: c[%d][%d] += %f * %f\n", i, j, a[i][k], b[k][j]);
         }
         printf("SEQ: c[%d][%d] = %f\n", i,j,c[i][j]);
       }
     }
   }

   // Parallel Portion.  Distribute a and b into local copies 
   // mya and myb.  Initialize myc to 0.  Initiate exchange of myb into tmpb.

   MPI_Scatter(a, N*N/P, MPI_FLOAT, mya, N*N/P, MPI_FLOAT, 0, MPI_COMM_WORLD);
   MPI_Scatter(b, N*N/P, MPI_FLOAT, myb, N*N/P, MPI_FLOAT, 0, MPI_COMM_WORLD);

   MPI_Isend(myb, N*N/P, MPI_FLOAT, (rank+1)%P, 0, MPI_COMM_WORLD, &sendreq);
   MPI_Irecv(tmpb, N*N/P, MPI_FLOAT, (rank+P-1)%P, MPI_ANY_TAG, MPI_COMM_WORLD, &rcvreq);

   for (i = 0; i < N/P; i++)
     for (j = 0; j < N; j++)
         myc[i][j] = 0.0;

   // Implement Calc1.  Should be like sequential code, but use
   // myc, mya, and myb.  Adjust bounds for loops corresponding to rows 
   // due to partitioning (i and k).  Index mya appropriately based on 
   // portion of tmpb to be computed.

   for (i = 0; i < N/P; i++) {
     for (j = 0; j < N; j++) {
       //printf("Rank %d: mya[%d][%d] = %f\n", rank, i, j, mya[i][j]);
       //printf("Rank %d: myb[%d][%d] = %f\n", rank, i, j, myb[i][j]);
     }
   }
   
   for (i = 0; i < N/P; i++) {
     for (j = 0; j < N; j++) {
       for (k = 0; k < N/P; k++) {
	 myc[i][j] += mya[i][k+(rank*N/P)] * myb[k][j];
	 //printf("Rank %d doing: myc[%d][%d] += %f * %f\n", rank, i+rank, j, mya[i][k], myb[k-rank][j]);
       }
     }
   }

   // Make sure you have received tmpb.

   MPI_Wait(&rcvreq, &status);

   // Implement Calc2.  Should be like sequential code, but use
   // myc, mya, and tmpb.  Adjust bounds for loops corresponding to rows 
   // due to partitioning (i and k).  Index mya appropriately based on 
   // portion of tmpb to be computed.
   
   for (i = 0; i < N/P; i++) {
     for (j = 0; j < N; j++) {
	 //printf("Rank %d: tmpb[%d][%d] = %f\n", rank, i, j, tmpb[i][j]);
     }
   }
   
   int off = ((rank + 1) % P) * N/P;
   for (i = 0; i < N/P; i++) {
     for (j = 0; j < N; j++) {
       for (k = 0; k < N/P; k++) {
	 myc[i][j] += mya[i][k+off] * tmpb[k][j];
	 //printf("Rank %d doing: myc[%d][%d] += %f * %f\n", rank, i+rank, j, mya[i][k-(rank*N/P)], tmpb[k-N/2][j]);
       }
     }
   }

   // Output local results to compare against sequential
   valid = 1;
   for (i = 0; i<N/P; i++) {   
     for (j = 0; j<N; j++) {   
       printf("PAR, RANK %d: c[%d][%d] = %f\n", rank, i+rank*N/P,j,myc[i][j]);
     }
   }

   MPI_Finalize();
}

