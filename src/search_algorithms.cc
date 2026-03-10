#include "search_algorithms.h"

#include "seq_funcs.h"
#include "test_funcs.h"

using namespace std;

vector<vector<int>> find_bad_degree_sequences(int c, int d, int lowbound) {
	vector<vector<int>> bad_ones;
	vector<vector<int>> possibles = gen_deg_seqs(c, d, lowbound);
	for(const vector<int>& curr_test_seq : possibles){
		if(!test_conjs(curr_test_seq)){
			bad_ones.push_back(curr_test_seq);
		}
	}
	return bad_ones;
}

vector<vector<int>> find_self_dual_l_one_sequences(int max_third_degree, size_t limit) {
	vector<vector<int>> matches;
	for(int a = 1; a <= max_third_degree - 2; a++){
		for(int b = a + 1; b <= max_third_degree - 1; b++){
			for(int c = b + 1; c <= max_third_degree; c++){
				vector<int> curr_d = {0, a, b, c, b + c - a, b + c};
				if(calc_L(curr_d) == 1){
					matches.push_back(curr_d);
					if(limit > 0 && matches.size() >= limit){
						return matches;
					}
				}
			}
		}
	}
	return matches;
}

vector<LargeBettiResult> find_large_betti_sequences(int c, int d, long long threshold, size_t limit) {
	vector<LargeBettiResult> matches;
	vector<vector<int>> seqs = gen_deg_seqs(c, d);
	for(const vector<int>& seq : seqs){
		vector<long long> betti = pure_betti(seq);
		for(long long b : betti){
			if(b > threshold){
				matches.push_back({seq, betti});
				if(limit > 0 && matches.size() >= limit){
					return matches;
				}
				break;
			}
		}
	}
	return matches;
}
