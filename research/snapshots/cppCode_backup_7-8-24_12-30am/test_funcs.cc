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
#include "binom.h"
#include "seq_funcs.h"
using namespace std;

bool test_conjs_v2(vector<int> D){
//	cerr << "Entering function test_conjs_v2, arg D = " << seq_to_string(D) << endl;
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
	//this all works, now the hard part
	//now do this for each vector of pairs:
	//for each numerator, see if it reduces with each denom (and if it does, just reduce it and continue)	
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
	//at this point all the fractions are reduced as much as possible, so L will be the lcm of all of them
	//question is, if we have (a*b) and (c*d), is lcm((a*b),(c*d)) the same as lcm(lcm(a,c),lcm(b,d))?
	//if so, this is good, since we can check L before multiplying out any one denominator, as any given denominator can be very big once fully multiplied out, but if we can go
	//factor by factor for each one, we will have a much, much easier time
	//lcm(2*3,6*1) = 6, lcm(lcm(2,6),lcm(3,1)) = lcm(6,3) = 6. Seems promising
	//IF we can "distribute" lcms like this, then we just update L not with each denominator, but with each iteration of a term in every denominator
	//eg if we have denoms (abc), (def), and (xyz), we don't have start with L=1 and do L = lcm(L,abc) then L = lcm(L,def) then check L (since abc and def could each be huge and at risk of
	//overflowing), but rather we start with L=1 and do L = lcm(L,a,d,x), then check L, then L = lcm(L,b,e,y), ... and stop if at any point L is big enough to pass the tests
	//if L is NOT big enough and we reach the end, does the pass necessarily fail? Could it still work? DO we have to worry abt overflow? Will solve these tomorrow 
	//NOTE: STL lcm function DOES take more than 2 params	
	//
	//start L calc
	int L = 1; //"global" L, lcm of all interim L's
	int target = binom(c,c/2);
	int warning_val = INT_MAX; //lol sqrt(LLONG_MAX) is just INT_MAX... duh, sqrt(2^64) = sqrt(2^32*2) = sqrt((2^32)^2) = 2^32
	if(target > warning_val) cout << "In function test_conjs_v2, target val for L greater than sqrt(LLONG_MAX), overflow likely.\n";
	//each denom will have c-1 factors, each of which may or may not be 1
	//need to go term by term in each denominator. remember each denominator is .second in a pair, and a vector of pairs makes up a whole pi_i
	//CANNOT DO THIS. L MUST BE LCM OF ALL DENOMS, BUT WITHIN A DENOM L MUST BE AT LEAST THE ENTIRE PRODUCT OF THAT DENOM
	//What this means is in any one denom, we can check if L would ever be made too big, but if not, L has to become the entire product
	vector<int> L_vec(c+1);
	L_vec.at(0) = 1; //since pi_0 is just 1/1
	int curr_L = 1;
	//need to calc an L for each denom, checking at every step, then do lcm of all the L's, again checking at every step
	for(int i=1; i<=c; i++){ //for each pi (starting at 1 since pi_0 is just 1/1)
		curr_L = 1; //reset curr L
		for(int j=0; j<c-1; j++){ //for each factor of the denom
			curr_L = curr_L *= pis.at(i).at(j).second;
			//each lcm can only make L bigger, so I can actually check intermediately
			if(curr_L >= target){
				cout << "An interim L hit " << curr_L << " which is > binom(c,c/2); both conjs autopass.\n";
				return true;
			}
			//curr_L is not too big, push to L vec
			L_vec.at(i) = curr_L;
		}
//		cerr << "After pi_" << i << ", curr L is now " << L << endl;
	}	
//	cerr << "L vec filled. Printing:\n";
//	cerr << "{";
//	for(int l: L_vec) cerr << l << " ";
//	cerr << "}\n";
	//now do lcm of all L's, checking each time
	//remember L (global L) is 1 at this point
	for(int l: L_vec){
		L = lcm(L,l);
		if(L >= target){
			cout << "L hit " << L << " which is > binom(c,c/2); conjs autopass\n";
			return true;
		}
	}
	//at this point L should be fully calculated without getting too big
//	cerr << "L fully calculated as " << L << " without reaching binom(c,c/2) or warning val (hopefully).\n";
	//now just test conjs normally
	//"reduce" all fractions now with L
//	cerr << "Now clearing all fractions with L:\n";
	for(int i=1; i<=c; i++){ //for each vector of pairs pi_i
		int mini_L = L; //our new factor to mult by
		for(pair<int,int> &q: pis.at(i)){ //grab each factor in the denominator and do the following
			int comm_div = gcd(mini_L,q.second);
			if(comm_div != 1){
				mini_L /= comm_div;
				q.second /= comm_div;
			}	
		}
		//after we've squeezed the denom out of L, just multiply it by the first guy in numerator
		pis.at(i).at(0).first *= mini_L;

	}
	//and do the same for pi_0
	pis.at(0).at(0).first *= L;
	//now print
//	cerr << "\nReduction done:\n";
//	for(auto v: pis){ //for each pi, which is a vector of pairs
//		cerr << "New pi: ";
//		for(pair<int,int> p: v) cerr << "{" << p.first << "," << p.second << "} ";
//		cerr << endl;
//	}
	//if we've reached this point, L didn't bail us out
	//now check the conjs with the "pure betti"
	int total_sum = 0;
	int curr_prod = 1;
	int curr_target = 1; //for BEH
	total_sum += L; //B_0's contribution to LLBC; will never fail BEH since pi_0 = 1 so B_0 always >= 1
	for(int i=1; i<=c; i++){ 
		//this loop is essentially calculating and testing B_i for each pi_i
		curr_prod = 1; //reset curr_prod
		curr_target = binom(c,i); //reset curr BEH target
		for(int j=0; j<c-1; j++){
			curr_prod *= pis.at(i).at(j).first;
			if(curr_prod >= warning_val) cout << "Warning: overflow possible.\n";
		}
		//now curr_prod is fully built
		if(curr_prod < curr_target) { //BEH failed
			cout << "BEH failed; B_" << i << " = " << curr_prod << " when c = " << c << ".\n";
			return false; 
		}
		total_sum += curr_prod;
		//not checking if total_sum >= warning_val since we'd have to add (and ergo have c equal to) like 2 billion of these for the sum to break it
	}
	//now total sum is fully calculated
	if(total_sum < pow(2,c-1) * 3){ //2^c-1 * 3 = 2^c * 3/2
		cout << "LLBC fail. Total sum was " << total_sum << ", with c of " << c << ".\n"; 
		return false;
	}	
	return true;
}

