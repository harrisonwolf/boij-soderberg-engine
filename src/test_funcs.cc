//implementation file for functions currently in development

#include "test_funcs.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include <fstream>
#include <numeric> //for gcd
#include <sstream>
#include <ctime>
#include <climits> //for LLONG_MAX and others
#include <stdexcept>
#include "binom.h"
#include "seq_funcs.h"
using namespace std;

bool test_conjs_v2(vector<int> D){
	const vector<long long> B = pure_betti(D);
	return test_BEH(B) && test_LLBC(B);
}

bool is_degen(vector<int> D){
	int c = D.size()-1;
	int curr_div = 1;
	for(int i=1; i<c-1; i++){
		curr_div = gcd(D.at(i),D.at(i+1));
		if(curr_div == 1) return true;
	}
	if(curr_div != 1) return true;
	return false;
}

//only gives back ones with max degree AT LEAST lowbound
//v2 will only give back nondegen
vector<vector<int>> gen_subsets_fast_v2(int start, int o, int n, int lowbound, int starting_o){
        //sanity check
        if(o<1){ cerr << "Tried calling gen_subsets_fast with o<1\n"; exit(1); }
        if(n<o){ cerr << "Tried calling gen_subsets_fast with o>n; can't have subset of n with more than n elements\n"; exit(1); }
        vector<vector<int>> retlist;
        //base case should just be o=1
        if(o == 1){ //base case
//              if(start < lowbound) return retlist; //THIS WAS THE BUG that didn't gen everything when lowbound wasn't 1
//              cerr << "Reached base case in gen_subsets_fast\n";
//              cerr << "start,o,n: " << start << "," << o << "," << n << endl;
                for(int i=start; i<=n; i++){
                        if(i<lowbound) continue; //THIS FIXED THE BUG
                        vector<int> curr = {i};
                        retlist.push_back(curr);
                }
                return retlist;
        }
        //now do process
        for(int i=start; i<=n-o+1; i++){
                vector<vector<int>> to_add = gen_subsets_fast_v2(i+1, o-1, n, lowbound, starting_o);
                for(vector<int> curr_add: to_add){
                        vector<int> temp = {i};
                        for(int a: curr_add) temp.push_back(a);
			if(starting_o == o){
				//check if temp is degen
				//if it is, just skip is
				vector<int> temp_D = temp;
				temp_D.insert(temp_D.begin(),0); //make it a proper degseq so i can call is_degen() on D	
				if(is_degen(temp_D)) continue;
			}
                        retlist.push_back(temp);
                }
        }
	/*
	 * Question: how do I only give back nondegen? I can only determine this after the entire degseq has been generated, not in any of the recursive calls
	 * The thing is, technically speaking the function never knows if it's in a recursive call or the original unless I add an extra parameter like first_o or something, but then I will
	 * have to go back and make sure this is changed whenever this function is called anywhere in the source code (although I think it's not called very often)
	 * Other option is to leave it with the same name but as a different function, and let the signature decide which is called (whether it's called with 4 ints or 3)
	 */

        return retlist;
}

//will only return NONDEGERNATE degree sequences, using gcd
//actually need to modify gen_subsets_fast since this is where we may run into mem issues
vector<vector<int>> gen_deg_seqs_v2(int c, int d, int lowbound /* = 1 */ ){
        vector<vector<int>> retseqs;
        if(lowbound < 1) lowbound = 1;
        //first get the order-c subsets of d
//      cerr << "Calling gen_subsets_fast(" << c << "," << d << "," << lowbound << ")...\n";
        vector<vector<int>> subsets = gen_subsets_fast_v2(1,c,d,lowbound,c);
        for(vector<int> subset: subsets){ //now prepend a 0 to each
                subset.insert(subset.begin(),0);
                retseqs.push_back(subset);
        }
//      cerr << "Returning possible degree sequences...\n";
        return retseqs;
}

string which_violations(vector<int> D){
	stringstream retstream("");
	const vector<long long> B = pure_betti(D);
	const int c = static_cast<int>(B.size()) - 1;

	for(int i=0; i<=c; i++){
		if(B.at(i) < binom(c,i)){
			retstream << "B_" << i << " (" << B.at(i) << ") ";
		}
	}

	long long total_sum = 0;
	for(long long b: B){
		if(b < 0 || total_sum > LLONG_MAX - b){
			throw overflow_error("which_violations sum exceeds signed 64-bit range");
		}
		total_sum += b;
	}
	if(!test_LLBC(B)){
		retstream << "LLBC (" << total_sum << ")";
	}
	return retstream.str();
}

// Legacy factor-display helper for the development-only test_program. The
// supported calculator uses compute_pi_values(), and all exact search paths use
// pure_betti(). This routine preserves factor pairs after cross-cancellation;
// it does not compute L or decide either conjecture.
vector<vector<pair<int,int>>> pi(vector<int> D){
	int c = D.size()-1; //codimension we're working in
	//first calculate pi_i's
	vector<vector<pair<int,int>>> pis(D.size()); //each pi_i will have a vector of pairs of num,den that are mult. together to yield final pi_i
	//fill the vector
	pis.at(0) = {pair<int,int>(1,1)}; //pi_0 is always 1/1
	//now do the rest
	for(int i=1; i<=c; i++){ //curr pi we're calculating; prod j != i d_j / d_i-d_j
		for(int j=1; j<=c; j++){
			if(i!=j){
				int diff = abs(D.at(j) - D.at(i));
				pair<int,int> curr_pair = pair<int,int>(D.at(j),diff);
				pis.at(i).push_back(curr_pair);
			}
		}

	}
	//list of vectors of pairs should now be constructed
	//let's see it
//	for(auto v: pis){ //for each pi, which is a vector of pairs
//		cerr << "New pi: ";
//		for(pair<int,int> p: v) cerr << "{" << p.first << "," << p.second << "} ";
//		cerr << endl;
//	}
	//Cross-cancel numerator factors against denominator factors for display.
	for(int i=1; i<=c; i++){ //for each vector of pairs pi_i
		for(pair<int,int> &p: pis.at(i)){ //grab each numerator and do the following
			for(pair<int,int> &q: pis.at(i)){
				int comm_div = gcd(p.first,q.second);
				if(comm_div != 1){
					p.first /= comm_div;
					q.second /= comm_div;
				}	
			}
		}
	}
	//now let's see if that did anything
//	cerr << "\nReduction done:\n";
//	for(auto v: pis){ //for each pi, which is a vector of pairs
//		cerr << "New pi: ";
//		for(pair<int,int> p: v) cerr << "{" << p.first << "," << p.second << "} ";
//		cerr << endl;
//	}

	return pis;
}

long long calc_L(vector<int> D){
	return pure_betti(D).front();
}

long long calc_sum(vector<int> D){
	const vector<long long> B = pure_betti(D);
	long long sum = 0;
	for(long long b: B){
		if(b < 0 || sum > LLONG_MAX - b){
			throw overflow_error("calc_sum exceeds signed 64-bit range");
		}
		sum += b;
	}
	return sum;
}
