#include "algorithm_helpers.h"
#include "seq_funcs.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

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
			const __int128 next = (
				static_cast<__int128>(result) * (n - k + i)) / i;
			if(next > numeric_limits<long long>::max()){
				throw overflow_error("degree-sequence count exceeds signed 64-bit range");
			}
			result = static_cast<long long>(next);
		}
		return result;
	};

	long long total = 0;
	for(int curr_max = start_max; ; curr_max++){
		const long long addend = choose(curr_max - 1, c - 1);
		if(total > numeric_limits<long long>::max() - addend){
			throw overflow_error("degree-sequence count exceeds signed 64-bit range");
		}
		total += addend;
		if(curr_max == d) break;
	}
	return total;
}

vector<RationalValue> compute_pi_values(const vector<int>& degrees) {
	const vector<long long> betti = pure_betti(degrees);
	const long long L = betti.front();
	vector<RationalValue> pi_values;
	pi_values.reserve(betti.size());
	for(long long value: betti){
		const long long divisor = gcd(value, L);
		pi_values.push_back({value / divisor, L / divisor});
	}

	return pi_values;
}
