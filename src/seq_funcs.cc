//implementation file for seq_funcs.h

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
#include <climits> //for INT_MAX and others
#include <limits>
#include <stdexcept>
#include "binom.h"
#include "seq_funcs.h"
using namespace std;

namespace {

long long checked_multiply_nonnegative(long long lhs, long long rhs, const char* context) {
	if(lhs < 0 || rhs < 0){
		throw invalid_argument(string(context) + " received a negative factor");
	}
	const __int128 product = static_cast<__int128>(lhs) * static_cast<__int128>(rhs);
	if(product > numeric_limits<long long>::max()){
		throw overflow_error(string(context) + " exceeds signed 64-bit range");
	}
	return static_cast<long long>(product);
}

long long checked_add_nonnegative(long long lhs, long long rhs, const char* context) {
	if(lhs < 0 || rhs < 0){
		throw invalid_argument(string(context) + " received a negative addend");
	}
	if(lhs > numeric_limits<long long>::max() - rhs){
		throw overflow_error(string(context) + " exceeds signed 64-bit range");
	}
	return lhs + rhs;
}

long long checked_lcm_nonnegative(long long lhs, long long rhs, const char* context) {
	if(lhs == 0 || rhs == 0) return 0;
	const long long divisor = gcd(lhs, rhs);
	return checked_multiply_nonnegative(lhs / divisor, rhs, context);
}
pair<long long,long long> reduced_product_fraction(
	vector<long long> numerator_factors, vector<long long> denominator_factors) {
	// Cross-cancel before multiplying. Division can only remove prime factors,
	// so one pass over every pair leaves the two products globally coprime.
	for(long long& numerator_factor: numerator_factors){
		if(numerator_factor <= 0){
			throw invalid_argument("pure_betti received a nonpositive numerator factor");
		}
		for(long long& denominator_factor: denominator_factors){
			if(denominator_factor <= 0){
				throw invalid_argument("pure_betti received a nonpositive denominator factor");
			}
			const long long divisor = gcd(numerator_factor, denominator_factor);
			numerator_factor /= divisor;
			denominator_factor /= divisor;
		}
	}

	long long numerator = 1;
	for(long long factor: numerator_factors){
		numerator = checked_multiply_nonnegative(
			numerator, factor, "pure_betti reduced numerator/final value");
	}
	long long denominator = 1;
	for(long long factor: denominator_factors){
		denominator = checked_multiply_nonnegative(
			denominator, factor, "pure_betti reduced denominator/LCM");
	}
	return {numerator, denominator};
}

int supported_betti_codimension(const vector<long long>& B, const char* context) {
	if(B.size() < 2){
		throw invalid_argument(string(context) + " requires at least two Betti values");
	}
	const size_t codimension = B.size() - 1;
	if(codimension > static_cast<size_t>(BOIJ_MAX_SUPPORTED_CODIMENSION)){
		throw invalid_argument(
			string(context) + " supports codimension at most "
			+ to_string(BOIJ_MAX_SUPPORTED_CODIMENSION));
	}
	return static_cast<int>(codimension);
}

}

void die(){
	cout << "Bad input!\n";
	exit(1);
}

//nicely make a sequence of integers into a string
string seq_to_string(vector<int> seq){
	string ret = "";
	stringstream ss(ret);
	ss << "{";
	for(int i: seq){
		ss << i << ",";
	}
	ret = ss.str();
	if(ret.size() > 1) ret.pop_back(); //get rid of the extra comma
	ret.push_back('}');
	return ret;
}

//nicely make a sequence of long longs into a string
string pure_betti_to_string(vector<long long> B){
	string ret = "";
	stringstream ss(ret);
	ss << "{";
	for(long long i: B){
		ss << i << ",";
	}
	ret = ss.str();
	ret.pop_back(); //get rid of the extra comma
	ret.push_back('}');
	return ret;
}

//read from stdin a degree sequence for the user and do fairly extensive validity checking
vector<int> read_degree_seq(){
        vector<int> seq;
        cout << "Please enter each d_i, seperated by spaces, followed by -1 (eg 0 3 5 8 -1):\n";
        int inp = -2;
        while(true){
                cin >> inp;
//              cerr << "inp: " << inp << endl;
                if(!cin){
                        cout << "Bad input. Please try again.\n";
                        cout << "Current sequence being built: " << seq_to_string(seq) << endl;
                        cin.clear(); //clear cin fail flag
                        cin.ignore(100,'\n'); //ignore whatever numbers were left until they pressed until
                        continue;
                }
                if(inp < 0){ //either they tried quitting (-1), in which case make sure they are at least something in the seq
                             //or they tried entering a negative number, which is not allowed
                        if(inp == -1){
                                if(seq.size() < 2){
                                        cout << "Degree sequence must have length at least 1 (after the 0). Please reenter.\n";
                                        cout << "Current sequence being built: " << seq_to_string(seq) << endl;
                                        cin.clear(); //clear cin fail flag
                                        cin.ignore(100,'\n'); //ignore whatever numbers were left until they pressed enter
                                        continue;
                                }else return seq;
                        }else{
                                cout << "Degree sequence cannot contain negative numbers. Please reenter.\n";
                                cout << "Current sequence being built: " << seq_to_string(seq) << endl;
                                cin.clear(); //clear cin fail flag
                                cin.ignore(100,'\n'); //ignore whatever numbers were left until they pressed until
                                continue;
                        }
                }else if(inp == 0){ //make sure seq is empty, otherwise they cannot put a 0
                        if(seq.size() != 0){
                                cout << "Sequence must be strictly increasing. Please reenter.\n";
                                cout << "Current sequence being built: " << seq_to_string(seq) << endl;
                                cin.clear(); //clear cin fail flag
                                cin.ignore(100,'\n'); //ignore whatever numbers were left until they pressed until
                                continue;
                        }else{
                                seq.push_back(inp);
                                continue;
                        }
                }else{ //normal positive int; make sure it is strictly > prev degree
                        if(inp <= seq.back()){
                                cout << "Sequence must be strictly increasing. Please reenter.\n";
                                cout << "Current sequence being built: " << seq_to_string(seq) << endl;
                                cin.clear(); //clear cin fail flag
                                cin.ignore(100,'\n'); //ignore whatever numbers were left until they pressed until
                                continue;
                        }else{
                                seq.push_back(inp);
                                continue;
                        }
                }
        }

        return {-1}; //control really should not reach here unless something has gone wrong
}

//only gives back ones with max degree AT LEAST lowbound
vector<vector<int>> gen_subsets_fast(int start, int o, int n, int lowbound){
	//sanity check
	if(o<1){ cerr << "Tried calling gen_subsets_fast with o<1\n"; exit(1); }
	if(n<o){ cerr << "Tried calling gen_subsets_fast with o>n; can't have subset of n with more than n elements\n"; exit(1); }
	vector<vector<int>> retlist;
	//base case should just be o=1
	if(o == 1){ //base case
//		if(start < lowbound) return retlist; //THIS WAS THE BUG that didn't gen everything when lowbound wasn't 1
//		cerr << "Reached base case in gen_subsets_fast\n";
//		cerr << "start,o,n: " << start << "," << o << "," << n << endl;
		for(int i=start; i<=n; i++){
			if(i<lowbound) continue; //THIS FIXED THE BUG
			vector<int> curr = {i};
			retlist.push_back(curr);
		}
		return retlist;
	}
	//now do process
	for(int i=start; i<=n-o+1; i++){
		vector<vector<int>> to_add = gen_subsets_fast(i+1, o-1, n, lowbound);
		for(vector<int> curr_add: to_add){
			vector<int> temp = {i};
			for(int a: curr_add) temp.push_back(a);
			retlist.push_back(temp);
		}
	}

	return retlist;
}

vector<vector<int>> gen_deg_seqs(int c, int d, int lowbound){
	vector<vector<int>> retseqs;
	if(lowbound < 1) lowbound = 1;
	//first get the order-c subsets of d
//	cerr << "Calling gen_subsets_fast(" << c << "," << d << "," << lowbound << ")...\n";
	vector<vector<int>> subsets = gen_subsets_fast(1,c,d,lowbound);
	for(vector<int> subset: subsets){ //now prepend a 0 to each
		subset.insert(subset.begin(),0);
		retseqs.push_back(subset);
	}
//	cerr << "Returning possible degree sequences...\n";
	return retseqs;
}

/*
 * Takes a degree sequence D, return the pure betti sequence
 */
vector<long long> pure_betti(vector<int> D){
//	cerr << "Entered pure_betti function\n";
//	cerr << "D.size(): " << D.size() << endl;
	if(D.size() < 2){
		throw invalid_argument("pure_betti requires at least two degrees");
	}
	if(D.front() != 0){
		throw invalid_argument("pure_betti requires a degree sequence beginning with zero");
	}
	for(size_t i=0; i+1<D.size(); i++){
		if(D.at(i) >= D.at(i+1)){
			throw invalid_argument("pure_betti requires a strictly increasing degree sequence");
		}
	}
//	cerr << "Starting pure_betti...\n";
	int c = D.size(); //our codimension
	vector<long long> retvec(c);
//	cerr << "retvec.size(): " << retvec.size() << endl;
	vector<pair<long long,long long>> pi_vec(c);
	//first set B(0)
	retvec.at(0) = 1;
	//now set pi_vec(0)
	pi_vec.at(0) = pair<int,int>(1,1);
	//now do the rest of the pi's
	for(int i=1; i<c; i++){
//		cerr << "Making pi_" << i << "...\n";
		vector<long long> numerator_factors;
		vector<long long> denominator_factors;
		for(int x: D){
			if(x > 0 and x != D.at(i)){
				numerator_factors.push_back(x);
				const long long difference = llabs(
					static_cast<long long>(x) - static_cast<long long>(D.at(i)));
				denominator_factors.push_back(difference);
			}
		}
		pi_vec.at(i) = reduced_product_fraction(
			numerator_factors, denominator_factors);
	}
	//now find L and multiply each numerator by it
//	cerr << "Finding L...\n";
	long long L = 1; //find lcm of denoms
	for(auto p: pi_vec){
		long long curr_mult = checked_lcm_nonnegative(L, p.second, "pure_betti denominator lcm");
//		cerr << "L = " << L << ", curr_denom = " << p.second << endl;
//		cerr << "lcm(L,curr_denom) = " << curr_mult << endl;
		if(curr_mult > L) L = curr_mult;
	}
//	cerr << "L: " << L << endl;
//	cerr << "pi_vec before multiplying by L: {";
//	for(pair<int,int> p: pi_vec) cerr << "(" << p.first << "," << p.second << ") ";
//	cerr << endl;
	//Clear each reduced denominator before multiplying. Multiplying num*L first
	//can overflow even when the final integral Betti number fits in long long.
	for(int i=0; i<c; i++){
		const long long denominator = pi_vec.at(i).second;
		if(L % denominator != 0){
			throw logic_error("pure_betti denominator does not divide its lcm");
		}
		retvec.at(i) = checked_multiply_nonnegative(
			pi_vec.at(i).first, L / denominator, "pure_betti value");
	}

	return retvec;
}

//return FALSE if violator
bool test_BEH(vector<long long> B){ //see if a potential betti sequence is a violator //Note: pure betti is passed, NOT a degree sequence
	//cerr << "Testing BEH on " << seq_to_string(B) << "...\n";
	const int c = supported_betti_codimension(B, "test_BEH");
	for(size_t i=0; i<B.size(); i++){
		if(B.at(i) < binom(c, static_cast<int>(i))){
			return false;
		}
	}

	return true;
}

//returns FALSE if violator
bool test_LLBC(vector<long long> B){
	const int c = supported_betti_codimension(B, "test_LLBC");
	long long sum = 0;
	for(long long b: B) sum = checked_add_nonnegative(sum, b, "test_LLBC sum");
	long long target = 3; // 3 * 2^(c-1), with c >= 1 for a degree sequence
	for(int exponent=1; exponent<c; exponent++){
		target = checked_multiply_nonnegative(target, 2, "test_LLBC target");
	}

	if(sum < target) return false;
	return true;
}

vector<vector<int>> gcd_rinse(vector<vector<int>> seqs){
	int num_seqs = seqs.size();
	vector<vector<int>> retvec;
	for(int i=0; i<num_seqs; i++){
		auto seq = seqs.at(i); //curr deg seq
		if(seq.size() < 3){ cerr << "called gcd_rinse with a sequence of size less than 3\n"; exit(1); }
		int divis = gcd(seq.at(seq.size()-1),seq.at(seq.size()-2));
		if(divis == 1){
			retvec.push_back(seq);
		}else{
//			cerr << "rinsed out " << seq_to_string(seq) << endl;
		}
	}
	return retvec;
}

vector<vector<int>> read_degree_seqs(string filename){
//	cerr << "Calling read_degree_seqs(" << filename << ")...\n";
	//assume filename is a valid input file and that this has been vetted
	vector<vector<int>> retlist;
	vector<int> curr_seq;
	string curr_int = "";
	fstream ins(filename);
	ins.ignore(1000,'{'); //ignore input until reaching first open brace
	char c = -1;
	while(ins){
		c = ins.get();
//		cerr << "curr c: " << c << endl;
		if(c == '{'){ //starting a new seq
			curr_seq.clear();
			curr_int = "";
		}else if(c == ','){ //finished curr int, push back stoi to curr_seq, reset string
			curr_seq.push_back(stoi(curr_int));
			curr_int = "";
		}else if(c == '}'){ //finished reading (curr int) and curr seq; push back stoi to curr_seq, reset string, push back curr_seq, reset curr_seq
			curr_seq.push_back(stoi(curr_int));
			curr_int = "";
			retlist.push_back(curr_seq);
			curr_seq.clear();
			ins.ignore(1000,'{');
		}else if(c == EOF){ //finished reading file, close and return list
//			cerr << "Reached EOF, breaking loop\n";
			ins.close();
			break;
		}else if(c == '\n'){
			continue;	
		}else{ //either pulled a digit or a letter
			if(!isdigit(c)){ //if letter, just ignore until next bracd
				ins.ignore(1000,'{');
			}
			curr_int.push_back(c);
		}
	}

	return retlist;
}

bool compare_seqs(vector<int> A, vector<int> B){
	if(A.size() != B.size()) return false;
	size_t size = A.size();
	for(size_t i=0; i<size; i++){
		if(A.at(i) != B.at(i)) return false;
	}
	return true;
}

vector<int> mult_seq(vector<int> base, int fac){
	vector<int> retvec = base;
	for(int &i: retvec) i*=fac;
	return retvec;
}

//full rinse of all dupes
vector<vector<int>> rinse_seqs(vector<vector<int>> seqs){
	vector<vector<int>> seen;
	vector<vector<int>> retlist;
	int max_d = 0; //the maximum degree that shows up in the given list
	int max_fac = -1; //we will update this later
	bool seen_this = false;
	for(vector<int> seq: seqs) if (seq.back() > max_d) max_d = seq.back();
	for(vector<int> seq: seqs){ //for each vector in the unrinsed list, do the following:
				    // 1) check if we have seen it already
				    // 2) if yes, move on
				    // 3) if no, add it and all dupes of it (up to (max d / curr.back) + 1 ) to seen vector
		seen_this = false;
		max_fac = max_d/seq.back() + 1;
		for(vector<int> other: seen){
			if(compare_seqs(seq,other) == true){
				seen_this = true;
				break;
			}
		}	
		if(!seen_this){ //haven't seen this yet, push to retlist and mark all dupes as seen
			retlist.push_back(seq);
			for(int fac=1; fac<=max_fac; fac++){
				seen.push_back(mult_seq(seq,fac));
			}
		}

	}

	return retlist;
}

bool test_conjs(vector<int> D){
	const vector<long long> B = pure_betti(D);
	return test_BEH(B) && test_LLBC(B);
}
