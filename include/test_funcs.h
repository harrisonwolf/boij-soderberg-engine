//header file for functions currently in development
#ifndef TEST_FUNCS_H
#define TEST_FUNCS_H

#include <vector>
#include <string>

//second iteration of test BEH and LLBC; tests both at same time and does in a way that prevents overflow
//Note: This takes as input a DEGREE SEQUENCE, NOT a BETTI SEQUENCE
bool test_conjs_v2(std::vector<int> D);

//takes a degseq, returns true if it is degen, false otherwise
bool is_degen(std::vector<int> D);

std::vector<std::vector<int>> gen_subsets_fast_v2(int start, int o, int n, int lowbound, int starting_o);

std::vector<std::vector<int>> gen_deg_seqs_v2(int c, int d, int lowbound = 1);

//returns a string with with i values where B_i < binom(c,i) and if LLBC failed
std::string which_violations(std::vector<int> D);

//Legacy development display: returns cross-cancelled factor pairs for each pi_i.
//Production callers use pure_betti() or compute_pi_values() instead.
std::vector<std::vector<std::pair<int,int>>> pi(std::vector<int> D);

long long calc_L(std::vector<int> D);

//Returns the checked sum of the exact pure Betti vector.
long long calc_sum(std::vector<int> D);

#endif
