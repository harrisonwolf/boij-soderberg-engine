/*
 * Purpose of this program is to find bad degree sequences (ones that give purebetti violators of BEH and/or LLBC)
 *
 * usage: ./a.out [codimension c] [max degree d] [outputfile]
 * Will test all degree sequences in codimension c up to a maximum d_c of d and write them to given output file
 * Again, make sure output file exists and does not contain sensitive information before passing it as an arg
 *
 * This file will contain a function that takes a codimension c and integer d and will return as a list of degree sequences gotten by pureBetti (ie vector<vector<int>>) all violators of BEH of LLBC
 * This list can be written to a file and then passed into remove_duplicates program to rinse out any duplicate degree sequences
 */

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
#include "seq_funcs.h"
using namespace std;


/*
 * Recursively generates 
 */

/*
 * This will generate all of the order-o subsets of {1,...,n}
 */

/*
 * Recursion? So first I start with {1,2,...,o}
 * Eg. o = 4, n = 6
 * Start w/ {1,2,3,4}, then do {1,2,3,5} then {1,2,3,6}. Once last number reaches n, increment number before last, then repeat
 * Now {1,2,4,5} then {1,2,4,6} //repeat
 * Now {1,2,5,6} //number before last as high as it can go (n-1). Now do number 2 before last
 * Now {1,3,4,5} then {1,3,4,6} //last number reached n, increment number before last
 * Now {1,3,5,6} //gone as high as can go w/ number before last, now increment number 2 before last
 * Now {1,4,5,6} //number 2 before last as high as can go! now number 3 before last!
 * Now {2,3,4,5} then {2,3,4,6} //last number hit peak, do number 1 before last
 * Now {2,3,5,6} //number 1 before last hit peak, do 2 before last
 * Now {2,4,5,6} //number 2 before last hit peak, increment 3 before last
 * Now {3,4,5,6} //number 3 before last hit peak! once number (o-1) before last hits peak, we're done! 
 * This seems to lend itself very well to recursion; the question is how to implement it
 * Or maybe just nested for loops
 */

int main(int argc, char** args){
	//first make sure correct number of args are passed and output file already exists
	if(argc < 4 or argc > 5){ cerr << "usage: bad_one_generator [outputfile] [codimension c] [max degree d] [optional: lowbound l] \n"; exit(1); }
	string outfile_name = args[1];
	int c = stoi(args[2]);
	int d = stoi(args[3]);
	int l = -1;
	if(argc == 5) l = stoi(args[4]);
	else l = 1;
	if(c > 20){ cerr << "You tried running program w/ codimension greater than 20. If you are sure, comment the check out from the code\n"; exit(1); }
	if(d < l){ cerr << "Tried generated seqs of max degree d < least degree l\n"; exit(1); }

	fstream outs;
	outs.open(outfile_name);
	if(!outs.is_open()){ cerr << "usage: bad_one_generator [codimension c] [max degree d] [outputfile]\nMake sure file \"" << outfile_name << "\" already exists.\n"; exit(1); }
	outs.close();
	//now start the process
	vector<vector<int>> bad_ones;
	vector<int> test_seq;
	//I want ordered length-c subsets of {1,...,d} all with a 0 in front
	//test gen_subsets
//	cerr << "Testing gen_subsets...\n";
//	vector<vector<int>> gen_test = gen_subsets(5);
//	for(vector<int> list: gen_test){
//		cerr << "{";
//		for(int i: list) cerr << i << " ";
//	        cerr << "}" << endl;	
//	}
	//test gen_deg_seqs

//	cerr << "Testing pure_betti on {0,2,7,8} (expected {15,28,48,35}):\n";
//	vector<int> blah = {0,2,7,8};
//	vector<int> test_pure_betti = pure_betti(blah);
//	cerr << " { ";
//	for(int i: test_pure_betti) cerr << i << " ";
//	cerr << "}" << endl;

	//test making bigger than 100, up to d
//	cerr << "Testing gen_deg_seqs(" << c << "," << d << ")...\n";
//	vector<vector<int>> genseqs_test = gen_deg_seqs(c,d,100);
	//print them all out
//	for(auto seq: genseqs_test){
//		cerr << "{";
//		for(int i: seq) cerr << i<< " ";
//		cerr << "}\n";
//	}
	//just print the last one
//	vector<int> last_one = genseqs_test.back();
//	cerr << "{";
//	for(int i: last_one) cerr << i << " ";
//	cerr << "}\n";

	
	//now actually find the bad ones	
	//vector<vector<int>> possibles = gen_deg_seqs(c,d);
	//only do the ones after l (lowbound)
	cout << "Calling gen_deg_seqs(" << c << "," << d << "," << l << ")...\n";
	clock_t start_program_time;
	start_program_time = clock();
	vector<vector<int>> possibles = gen_deg_seqs(c,d,l);
	auto t = clock() - start_program_time;
	double secs = ((float)t)/CLOCKS_PER_SEC;
	cout << "Generated " << possibles.size() << " degree sequences in " << secs << " seconds\n";
	

//	cerr << "Testing pure betti on all seqs\n";
//	for(vector<int> curr_seq: possibles) cerr << seq_to_string(curr_seq) << " -> " << seq_to_string(pure_betti(curr_seq)) << endl;
	//pure_betti seems to work fine
	
	cout << "Starting tests...\n";
	int counter = 0;
	/*
	for(vector<int> curr_test_seq: possibles){
		//cerr << "Currently testing " << seq_to_string(curr_test_seq) << endl;
		bool is_good = true; //innocent until proven guilty
		//first find the pure betti
		vector<long long> B = pure_betti(curr_test_seq);
		//test BEH conj
		if(!test_BEH(B)){ 
			is_good = false;
//			cerr << "Degree sequence " << seq_to_string(curr_test_seq) << " with pure_betti " << pure_betti_to_string(B) << " failed BEH\n";
		}
		//test LLBC conj
		if(!test_LLBC(B)){ 
			is_good = false;
//			cerr << "Degree sequence " << seq_to_string(curr_test_seq) << " with pure_betti " << pure_betti_to_string(B)  << " failed LLBC\n";
		}
		//if he failed either test, blacklist him
		if(!is_good) bad_ones.push_back(curr_test_seq);	
		counter++;
	}
	*/
	for(vector<int> curr_test_seq: possibles){
		if(!test_conjs(curr_test_seq)) bad_ones.push_back(curr_test_seq);
		counter++; //making sure all were really tested
	}
	cout << "Tested BEH and LLBC on " << counter << " degree sequences\n";
	//now print them
//	cerr << "Bad ones found:\n";
//	for(vector<int> bad_one: bad_ones) cerr << seq_to_string(bad_one) << endl;

	//do gcd rinse
	cout << "Running gcd_rinse...\n";
	vector<vector<int>> rinsed_bad_ones = gcd_rinse(bad_ones);
	t = clock() - t;
	secs = ((float)t)/CLOCKS_PER_SEC;
	cout << "Finished all tests in " << secs << " seconds\n";
	//now write them to the outfile
	outs.open(outfile_name);
	outs << "Bad ones for codimension c = " << c << " up to max degree d = " << d << "\n";
	for(vector<int> bad_one: bad_ones){
		outs << seq_to_string(bad_one) << "\n";
	}
	outs << "gcd_rinsed ones: \n";
	for(vector<int> bad_one: rinsed_bad_ones){
		outs << seq_to_string(bad_one) << "\n";
	}
	outs << EOF;
	outs.close();
	//now print them
//	cout << "\nPrinting violators: \n";
//	for(vector<int> bad_one: bad_ones){
//		cout << seq_to_string(bad_one) << endl;
//	}
	cout << "\nPrinting rinsed violators: \n";
	for(vector<int> bad_one: rinsed_bad_ones){
		cout << seq_to_string(bad_one) << "\n";
	}



}
