#include "binom.h"

int binom(int n,int k){ //returns binomial coeff n choose k
	// Base Cases
	if (k > n)
		return 0;
	if (k == 0 or k == n)
		return 1;

	// Recur
	return binom(n - 1, k - 1) + binom(n - 1, k);
}
