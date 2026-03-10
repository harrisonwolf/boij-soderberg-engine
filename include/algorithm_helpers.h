#ifndef ALGORITHM_HELPERS_H
#define ALGORITHM_HELPERS_H

#include <vector>

struct RationalValue {
	long long num;
	long long den;
};

inline bool operator==(const RationalValue& lhs, const RationalValue& rhs) {
	return lhs.num == rhs.num && lhs.den == rhs.den;
}

long long count_degree_sequences(int c, int d, int lowbound);

std::vector<RationalValue> compute_pi_values(const std::vector<int>& degrees);

#endif
