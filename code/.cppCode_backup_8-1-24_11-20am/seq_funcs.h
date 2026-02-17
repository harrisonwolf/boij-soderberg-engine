//Header file for functions relating to degree sequences and pure betti tables of them
#ifndef SEQ_FUNCS_H
#define SEQ_FUNCS_H

#include <iostream>
#include <vector>

void die();

//nicely make a sequence of integers into a string
std::string seq_to_string(std::vector<int> seq);

//nicely make a sequence of long longs into a string
std::string pure_betti_to_string(std::vector<long long> B);

//read a single degree sequence from stdin and make sure it is valid
std::vector<int> read_degree_seq();

//generate all possible order-o subsets of {start...n} where biggest number in each one is at least lowbound
//Note: this is not usually called directly by the user; user calls gen_deg_seqs and gen_deg_seqs calls gen_subsets_fast
std::vector<std::vector<int>> gen_subsets_fast(int start, int o, int n, int lowbound);

//generate all possible degree sequences in codimension c up to max degree d, and max degree at least lowbound (defaults to 1)
std::vector<std::vector<int>> gen_deg_seqs(int c, int d, int lowbound = 1);

/*
 * Takes a degree sequence D, return the pure betti sequence
 */
std::vector<long long> pure_betti(std::vector<int> D);

//checks if a betti sequence passes BEH; returns false if it violates
//Note: Expects a betti sequence, NOT a degree sequence
bool test_BEH(std::vector<long long> B);

//checks if a betti sequence passes LLBC; returns false if it violates                                                                                                                                            //Note: Expects a betti sequence, NOT a degree sequence
bool test_LLBC(std::vector<long long> B);

//does a (lazy) removal of degenerate degree sequences in a given list by taking the gcd of last two d_i; NOT rigorous rinse and only for quick, sloppy checks
std::vector<std::vector<int>> gcd_rinse(std::vector<std::vector<int>> seqs);

//read an input file of formatted degree sequences and store in a list (vector) of vectors
std::vector<std::vector<int>> read_degree_seqs(std::string filename);

//Returns true if two vector contain the exact same elements in the same order, false otherwise
bool compare_seqs(std::vector<int> a, std::vector<int> b);

//Takes a vector of degree seqs (vectors of type int) and removes any that are a constant multiple of another
std::vector<int> mult_seq(std::vector<int> base, int fac);

//Takes a vector of degree seqs (vectors of type int) and removes any that are a constant multiple of another
//NOT THE SAME AS GCD RINSE
std::vector<std::vector<int>> rinse_seqs(std::vector<std::vector<int>> seqs);

//tests both conjs on a DEGREE SEQ, NOT BETTI SEQ, using the alternate method
bool test_conjs(std::vector<int> D);

#endif
