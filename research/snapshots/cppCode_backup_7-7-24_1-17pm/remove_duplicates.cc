#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include <fstream>
#include <sstream>
#include <numeric>
#include "seq_funcs.h"
using namespace std;

/*
 * Program should take in an input file and output file It should read in degree sequences of
 * from input file (length implied), remove any degree sequence that is a multiple of another, and write out the
 * remaining ones to the output file
 *
 * usage: sieve [input_file.txt] [output_file.txt] 
 * MAKE SURE OUTPUTFILE.TXT EXISTS ALREADY  
 */

int main(int argc, char** args){
//	cout << "argc: " << argc << endl;
	if(argc != 3){
		cout << "usage: remove_duplicates [input_file.txt] [output_file.txt]" << endl;
		exit(0);

	}
	string input_filename = args[1];
	string output_filename = args[2];
	//make sure output file exists
	fstream test;
	test.open(output_filename);
	if(!test.is_open()){ cout << "Must provide an existing file to write output to\n"; exit(1); }

	//cout << "args[0]: " << args[0] << endl; //always "./a.out"
	//cout << "args[1]: " << args[1] << endl;

	vector<vector<int>> seqs = read_degree_seqs(input_filename);

	//let's test to see if we're pulling the list correctly
//	cerr << "Seqs have been read, printing now...\n";
//	for(auto seq: seqs){
//		cout << "{";
//		for(int i: seq) cout << i << " ";
//		cout << "}" << endl;
//	}
//	cerr << "Seqs have been read\n";
	
	//going to do slower imp. than sieve of era. but should have few enough deg. seqs. that shouldn't matter
	/* Have a bunch of degseqs eg
	 * {0,1,2,3}, {0,1,3,4}, {0,2,4,6}, {0,1,5,6), {0,3,5,8} {0,3,6,9}
	 * In this list we would need to remove {0,2,4,6} and {0,3,6,9} since they are multiples of {0,1,2,3}
	 * Obv every seq that is a multiple of another will all share a gcd of whatever the multiple is (times the
	 * gcd of the original seq) but this method runs into problems
	 *
	 * Dr. Boocher's method: every seq we run into, we mark all of its multiples up to a certain number as "seen"
	 * in the seen hash table
	 * As we go through the given list of seqs, if seq#seen == 0, add it to retlist, mark all multiples of it
	 * as seen, move on
	 * If seq#seen == 1, just go to the next
	 *
	 * As for how many multiples of a given list we should mark as seen, an upper bound is just seqs.size()
	 * gonna try that first;
	 */

	//rinse the sequences

	vector<vector<int>> retlist = rinse_seqs(seqs);
	//print out rinses seqs
	//in this case we should just have {0,1,2,3}, {0,1,3,4}, {0,1,5,6}, {0,3,5,8} left
//	cerr << "\nPrinted rinsed seqs...\n";
//	for(auto seq: retlist){
//		cout << "{";
//		for(int i: seq) cout << i << " ";
//		cout << "}" << endl;
//	}
//	works first try...

	//now make each sequence into a nice string again and write it to output file
//	cerr << "Writing rinsed list to " << output_filename << endl;
	fstream outs;
	outs.open(output_filename); //this will not work unless output_filename is already an existing file
	outs << "Rinsed degree sequences from file " << input_filename << "\n";
//	cerr << "Turning seqs back into strings...\n";
	for(vector<int> seq: retlist){ //take each vector in the rinsed seqs
		string curr_seq_string = seq_to_string(seq);
//		curr_seq_string.insert(0,"{"); //add an open brace to the front of the list; std::string does not have push_front method
//		curr_seq_string.push_back('}');	
		//check what it looks like
		//cerr << "curr_seq_string: " << curr_seq_string << endl; 
		//now write it to output file along with a newline
		outs << curr_seq_string << "\n";
		//need to make sure outs exists before opening
	}
	outs.close();
//	cerr << "Finished writing rinsed seqs\n";
	

}
