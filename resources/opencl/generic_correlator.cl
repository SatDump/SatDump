
void dot_prod_32f(float *result, global float *input, global float *taps,
                  int num_points) {
  float dotProduct = 0;
  const float *aPtr = input;
  const float *bPtr = taps;
  unsigned int number = 0;

  for (number = 0; number < num_points; number++) {
    dotProduct += ((*aPtr++) * (*bPtr++));
  }

  *result = dotProduct;
}

void kernel correlate(global float *syncs, global float *input,
                      global float *corrs, global int *matches,
                      global int *nsync, global int *syncsize) {

  int id = get_global_id(0);
  int nthreads = get_global_size(0);

  int best_match = 0;
  float best_corr = 0;
  float corr = 0;

  for (int s = 0; s < *nsync; s++) {
    dot_prod_32f(&corr, &input[id], &syncs[s * *syncsize], *syncsize);

    if (corr > best_corr) {
      best_corr = corr;
      best_match = s;
    }
  }

  corrs[id] = best_corr;
  matches[id] = best_match;
}
