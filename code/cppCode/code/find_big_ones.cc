#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include "seq_funcs.h"
using namespace std;

int main(){
	vector<vector<int>> seqs = gen_deg_seqs(3,100);
	vector<vector<int>> seq_vec;
	vector<vector<long long>> B_vec;
	for(auto seq: seqs){
		auto B = pure_betti(seq);
		for(auto b: B){
			if(b>50000){
				seq_vec.push_back(seq);
				B_vec.push_back(B);
				break;
			}
		}
	}		
	for(int i=0; i<seq_vec.size(); i++){
		cout << seq_to_string(seq_vec.at(i)) << ": " << pure_betti_to_string(B_vec.at(i)) << endl;
	}
}
