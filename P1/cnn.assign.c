#include <stdio.h>
#include <omp.h>

/*
  Simple CNN code.  This code will work for all layers.  Read in arguments in specific order.  Only works for correct number of arguments.

  Compile with
    module load intel
    icc -O3 -fopenmp -o cnn_par cnn.assign.c

  Submit to queue using sbatch

  Vary parallelization strategies, loop order (when safe), different scheduling strategies, with and without reductions, number of threads
*/

int main(int argc, char *argv[]) {
  int n,k,c,p,q,r,s,ii,ij;

  // READ PROBLEM SIZES
  if (argc != 11) exit(1);
  int N = atoi(argv[1]);
  int C = atoi(argv[2]);
  int K = atoi(argv[3]);
  int H = atoi(argv[4]);
  int W = atoi(argv[5]);
  int R = atoi(argv[6]);
  int S = atoi(argv[7]);
  int u = atoi(argv[8]);
  int v = atoi(argv[9]);
  int P = (H-R)/u + 1;
  int Q = (W-S)/v + 1;
  int version = atoi(argv[10]);

  // ALLOCATE MEMORY
  float output_seq[128][128][55][55];
  memset(output_seq,0.0,128*128*55*55*sizeof(float));
  float output_par[128][128][55][55];
  memset(output_par,0.0,128*128*55*55*sizeof(float));
  float input[128][832][112][112];
  float weight[128][832][3][3];

  // ASSIGN INITIAL VALUES FOR INPUT AND WEIGHT
  for (n=0; n<N; n++) {
    for (c=0; c<C; c++) {
      for (p=0; p<(P-1)*u+R; p++) {
	for (q=0; q<(Q-1)*v+S; q ++) {
	  input [n][c][p][q] = ((float) (n+c+p+q)/7);
	} } } }

  for (k=0; k<K; k++) {
    for (c=0; c<C; c++) {
      for (r =0; r<R; r++) {
	for (s =0; s<S; s++) {
	  weight[k][c][r][s] = ((float) (k+c+r+s)/11);
	}}}}

  // SEQUENTIAL CALCULATION
  double seq_start = omp_get_wtime();
  for (n=0; n<N; n++) { // minibatch size
    for (k=0; k<K; k ++) { // output feature map
      for (c=0; c<C; c ++) { // input feature map
	for (p=0; p<P; p ++) { // output height
	  ij = p * u; // input height
	  for (q =0; q<Q; q ++) { // output width
	    ii = q * v; // input width
	    for (r=0; r<R; r ++) { // filter height
	      for (s =0; s< S; s ++) {// filter width
		output_seq[n][k][p][q] += input[n][c][ij+r][ii+s] * weight[k][c][r][s];
	      } } } } } } }
  double seq_end = omp_get_wtime();
  double seq_time = seq_end - seq_start;
  
  double par_time = 999.999, par_start = 0, par_end = 0;;
  
  switch(version) {
  // PARALLEL CALCULATION 1
  case 1:
    par_start = omp_get_wtime();
#pragma omp parallel for
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input[n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;

  // PARALLEL CALCULATION 2
  case 2:
    par_start = omp_get_wtime();
#pragma omp parallel for private(n, k, c, p, ij, q, ii, r, s)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;
 
  // PARALLEL CALCULATION 3
  case 3:
    par_start = omp_get_wtime();
#pragma omp parallel for collapse(2) private(n, k, c, p, ij, q, ii, r, s)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		//#pragma omp parallel for private(n, k, c, p, ij, q, ii, r, s)
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;

    // PARALLEL CALCULATION 4
  case 4:
    par_start = omp_get_wtime();
#pragma omp parallel for num_threads(32) collapse(2) private(n, k, c, p, ij, q, ii, r, s)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;
  
  // PARALLEL CALCULATION 5
  case 5:
    par_start = omp_get_wtime();
#pragma omp parallel for num_threads(64) collapse(2) private(n, k, c, p, ij, q, ii, r, s)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;
  
// PARALLEL CALCULATION 6
  case 6:
    par_start = omp_get_wtime();
#pragma omp parallel for collapse(2) private(n, k, c, p, ij, q, ii, r, s) schedule(static)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;
 
 // PARALLEL CALCULATION 7
  case 7: 
    par_start = omp_get_wtime();
#pragma omp parallel for collapse(2) private(n, k, c, p, ij, q, ii, r, s) schedule(dynamic)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;

 // PARALLEL CALCULATION 8
  case 8:
    par_start = omp_get_wtime();
#pragma omp parallel for collapse(2) private(n, k, c, p, ij, q, ii, r, s) schedule(dynamic, 4)
    for (n=0; n<N; n++) { // minibatch size
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } } }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;

  // PARALLEL CALCULATION 9
  case 9:
    par_start = omp_get_wtime();
#pragma omp parallel for private(n, k, c, p, ij, q, ii, r, s)
    for (n=0; n<N; n += 2) { // minibatch size
#pragma omp parallel for collapse(2)
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } }
#pragma omp parallel for collapse(2)
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n+1][k][p][q] += input [n+1][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } }
 }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;

  // PARALLEL CALCULATION 10
  case 10:
   par_start = omp_get_wtime();
#pragma omp parallel for private(n, k, c, p, ij, q, ii, r, s) schedule(dynamic)
    for (n=0; n<N; n += 2) { // minibatch size
#pragma omp parallel for collapse(2) schedule(dynamic) num_threads(64)
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n][k][p][q] += input [n][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } }
#pragma omp parallel for collapse(2) schedule(dynamic) num_threads(64)
      for (k=0; k<K; k ++) { // output feature map
	for (c=0; c<C; c ++) { // input feature map
	  for (p=0; p<P; p ++) { // output height
	    ij = p * u; // input height
	    for (q =0; q<Q; q ++) { // output width
	      ii = q * v; // input width
	      for (r=0; r<R; r ++) { // filter height
		for (s =0; s< S; s ++) {// filter width
		  output_par[n+1][k][p][q] += input [n+1][c][ij+r][ii+s] * weight[k][c][r][s];
		} } } } } }
    }
    par_end = omp_get_wtime();
    par_time = par_end - par_start;
    break;
  }

  // VERIFY CORRECTNESS BY COMPARING OUTPUTS
  for (n=0; n<N; n++) { // minibatch size
    for (k=0; k<K; k ++) { // output feature map
      for (c=0; c<C; c ++) { // input feature map
	for (p=0; p<P; p ++) { // output height
	  for (q =0; q<Q; q ++) { // output width
	    if(abs(output_seq[n][k][p][q]-output_par[n][k][p][q])> .0001) {
	      printf("Outputs do not match!!!\n");
	      exit(2);
	    }}}}}}

  // PRINT OUT SPEEDUP
  printf (">Sequential time = %f, Parallel time = %f, Speedup = %f, Version = %d\n",seq_time, par_time, seq_time/par_time, version);
}
