#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include "search_algorithms.h"
#include "seq_funcs.h"
using namespace std;

int main(){
	cout << "Finding self_dual degseqs in c=5 with L=1: \n";
	vector<vector<int>> matches = find_self_dual_l_one_sequences();
	for(const vector<int>& curr_d : matches){
		cout << seq_to_string(curr_d) << endl;
	}
}
