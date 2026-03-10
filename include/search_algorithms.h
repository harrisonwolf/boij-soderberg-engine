#ifndef SEARCH_ALGORITHMS_H
#define SEARCH_ALGORITHMS_H

#include <cstddef>
#include <vector>

struct LargeBettiResult {
	std::vector<int> degrees;
	std::vector<long long> betti;
};

inline bool operator==(const LargeBettiResult& lhs, const LargeBettiResult& rhs) {
	return lhs.degrees == rhs.degrees && lhs.betti == rhs.betti;
}

std::vector<std::vector<int>> find_bad_degree_sequences(int c, int d, int lowbound = 1);

std::vector<std::vector<int>> find_self_dual_l_one_sequences(int max_third_degree = 100, std::size_t limit = 0);

std::vector<LargeBettiResult> find_large_betti_sequences(int c, int d, long long threshold, std::size_t limit = 0);

#endif
