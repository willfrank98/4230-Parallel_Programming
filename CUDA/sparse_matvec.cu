/*
 * This file was created automatically from SUIF
 *   on Fri Nov  4 15:08:58 2011.
 */
#include <stdio.h>
//#include <cutil.h>

#define __suif_min(x,y) ((x)<(y)?(x):(y))

;
#define N 4096
extern void MV_GPU_wrapper(float (*)[4096], float *, float *);

extern int cudaMemcpy();
extern int cudaFree();
extern void __syncthreads();
extern int cudaMemcpyToSymbol();
extern __global__ void mv_GPU(float *, float (*)[4096], float *);

int compare(float *a, float *b, int size, double threshold) {
  int i;
  int valid = 1;
  for (i=0; i<size; i++) {
    if (abs(a[i]-b[i]) > threshold) {
      printf("Got %.4f but expected %.4f at pos %d\n", b[i], a[i], i);
      valid = 0;
    }
  }
  return valid;
}

void normalMV(float *t, float* b, float *data, int* ptr, int* indices, int nr){
  int i, j;
  for (i=0; i<nr; i++) {                                                      
    for (j = ptr[i]; j<ptr[i+1]; j++) {
      //printf("Doing: t[%d] = t[%d] + data[%d] * b[indices[%d]]\n", i, i, j, j);
      t[i] = t[i] + data[j] * b[indices[j]];
    }
  }
}

extern void MV_GPU_wrapper(float *a, float *c, float *b) {
  return;
}

extern __global__ void mv_GPU(float *t, float* b, float *data, int* ptr, int* indices) {
  int bx = blockIdx.x;
  //int tx = threadIdx.x;
  //printf("bx: %d, tx: %d\n", bx, tx);
  
  int j;
  for (j = ptr[bx]; j<ptr[bx+1]; j++) {
    //printf("Doing: t[%d] = t[%d] + data[%d] * b[indices[%d]]\n", tx, tx, j, j);   
    t[bx] = t[bx] + data[j] * b[indices[j]];
  }

  return;
}

main (int argc, char **argv) {
  FILE *fp;
  char line[1024]; 
  int *ptr, *indices;
  float *data, *b, *t;
  int i;
  int n; // number of nonzero elements in data
  int nr; // number of rows in matrix
  int nc; // number of columns in matrix

  // Open input file and read to end of comments
  if (argc !=2) abort(); 

  if ((fp = fopen(argv[1], "r")) == NULL) {
    abort();
  }

  fgets(line, 128, fp);
  while (line[0] == '%') {
    fgets(line, 128, fp); 
  }

  // Read number of rows (nr), number of columns (nc) and
  // number of elements and allocate memory for ptr, indices, data, b and t.
  sscanf(line,"%d %d %d\n", &nr, &nc, &n);
  ptr = (int *) malloc ((nr+1)*sizeof(int));
  indices = (int *) malloc(n*sizeof(int));
  data = (float *) malloc(n*sizeof(float));
  b = (float *) malloc(nc*sizeof(float));
  t = (float *) malloc(nr*sizeof(float));

  // Read data in coordinate format and initialize sparse matrix
  int lastr=0;
  for (i=0; i<n; i++) {
    int r;
    fscanf(fp,"%d %d %f\n", &r, &(indices[i]), &(data[i]));  
    indices[i]--;  // start numbering at 0
    if (r!=lastr) { 
      ptr[r-1] = i; 
      lastr = r; 
    }
  }
  ptr[nr] = n;

  // initialize t to 0 and b with random data  
  for (i=0; i<nr; i++) {
    t[i] = 0.0;
  }
  for (i=0; i<nc; i++) {
    b[i] = (float) rand()/1111111111;
  }        

  // create CUDA event handles for timing purposes
  cudaEvent_t start_event, stop_event;
  float elapsed_time_seq, elapsed_time_gpu;

  cudaEventCreate(&start_event);
  cudaEventCreate(&stop_event);
  cudaEventRecord(start_event, 0);   
  normalMV(t, b, data, ptr, indices, nr);
  cudaEventRecord(stop_event, 0);
  cudaEventSynchronize(stop_event);
  cudaEventElapsedTime(&elapsed_time_seq,start_event, stop_event);

  printf("Seq time: %.4f\n", elapsed_time_seq);


  float *tp, *bp, *datap, *result;
  int *ptrp, *indicesp;
  result = (float *) malloc(nr * sizeof(float));

  cudaMalloc((void **)&tp, nr * 4);
  cudaMalloc((void **)&bp, nc * 4);
  cudaMemcpy(bp, b, nc * 4, cudaMemcpyHostToDevice);
  cudaMalloc((void **)&datap, n * 4);
  cudaMemcpy(datap, data, n * 4, cudaMemcpyHostToDevice);
  cudaMalloc((void **)&ptrp, (nr+1) * 4);
  cudaMemcpy(ptrp, ptr, (nr+1) * 4, cudaMemcpyHostToDevice);
  cudaMalloc((void **)&indicesp, n * 4);
  cudaMemcpy(indicesp, indices, n * 4, cudaMemcpyHostToDevice);

  dim3 dimGrid(nr, 1);
  dim3 dimBlock(1, 1);

  cudaEventCreate(&start_event);
  cudaEventCreate(&stop_event);
  cudaEventRecord(start_event, 0);   
  mv_GPU<<<dimGrid,dimBlock>>>(tp, bp, datap, ptrp, indicesp);
  cudaEventRecord(stop_event, 0);
  cudaEventSynchronize(stop_event);
  cudaMemcpy(result, tp, nr * 4, cudaMemcpyDeviceToHost);
  cudaEventElapsedTime(&elapsed_time_gpu,start_event, stop_event);

  cudaFree(tp);
  cudaFree(bp);
  cudaFree(datap);
  cudaFree(ptrp);
  cudaFree(indicesp);

  printf("Par time: %.4f\n", elapsed_time_gpu);
  printf("Speedup: %.4f\n", elapsed_time_seq/elapsed_time_gpu);

  int res = compare(t, result, nr, 0.01); //lower threshold than before to account for some rare errors
  if (res == 1) {
    printf("VALID!\n");
  }
  else printf("INVALID...\n");
}
