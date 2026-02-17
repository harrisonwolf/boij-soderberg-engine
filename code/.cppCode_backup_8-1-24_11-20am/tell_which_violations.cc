//program to take a file of degseqs and explicitly print what violations each one committed
//will just print to stdout for now
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include "seq_funcs.h"
#include "test_funcs.h"
using namespace std;

//usage: tell_which_violations [input filename]
int main(int argc, char** args){
	if(argc	!= 2) cout << "usage: tell_which_violations [input filename]\n";
	string infile_name = args[1];
	vector<vector<int>> seqs = read_degree_seqs(infile_name);
	for(auto seq: seqs){
		if(which_violations(seq).size() > 0)
			cout << seq_to_string(seq) << ": " << which_violations(seq) << "" << "\n\n";
		
	}
}
