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
#include "binom.h"
#include "seq_funcs.h"
using namespace std;

bool test_conjs_v2(vector<int> D){
	cerr << "Entering function test_conjs_v2, arg D = " << seq_to_string(D) << endl;
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
	for(auto v: pis){ //for each pi, which is a vector of pairs
		cerr << "New pi: ";
		for(pair<int,int> p: v) cerr << "{" << p.first << "," << p.second << "} ";
		cerr << endl;
	}
	//this all works, now the hard part
	
	
	return false; //FIXME
}

