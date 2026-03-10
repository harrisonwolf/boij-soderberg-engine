#include "algorithm_helpers.h"

#include <cstdlib>
#include <numeric>

using namespace std;

long long count_degree_sequences(int c, int d, int lowbound) {
	if(lowbound < 1) lowbound = 1;
	int start_max = max(lowbound, c);
	if(d < start_max) return 0;

	auto choose = [](int n, int k) -> long long {
		if(k < 0 || k > n) return 0;
		if(k == 0 || k == n) return 1;
		if(k > n - k) k = n - k;
		long long result = 1;
		for(int i = 1; i <= k; i++){
			result = (result * (n - k + i)) / i;
		}
		return result;
	};

	long long total = 0;
	for(int curr_max = start_max; curr_max <= d; curr_max++){
		total += choose(curr_max - 1, c - 1);
	}
	return total;
}

vector<RationalValue> compute_pi_values(const vector<int>& degrees) {
	vector<RationalValue> pi_values;
	pi_values.reserve(degrees.size());
	pi_values.push_back({1, 1});

	for(size_t i = 1; i < degrees.size(); ++i){
		long long numerator = 1;
		long long denominator = 1;
		for(size_t j = 1; j < degrees.size(); ++j){
			if(j == i) continue;
			numerator *= degrees[j];
			denominator *= llabs(static_cast<long long>(degrees[j]) - degrees[i]);
		}

		long long divisor = gcd(numerator, denominator);
		pi_values.push_back({numerator / divisor, denominator / divisor});
	}

	return pi_values;
}
