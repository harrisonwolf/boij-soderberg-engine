Legacy note from the pre-migration `code/cppCode` layout.
Current build location is repository root (`make`) with binaries in `build/bin/`.

Everything is compiled with make
To compile only a specific executable, run "make [executable name]"; eg "make bad_one_generator"

remove_duplicates will take a formatted list of degree sequences (see bad_ones_c3.txt for formatting example) in any codimension and write the rinsed sequences to a given output file
To run, arguments [inputfile] [outputfile]
Make sure the output file alrady exists and does not contain important info, remove_duplicates cannot create new files but will overwrite what is in the given output file
Note: touch [filename] will create an empty file called [filename] (if [filename] does not already exist)

bad_one_generator will 
usage: bad_one_generator [output file] [codimension] [max degree] [min degree (default = 1)]

Starts to give false positives on bad ones at around max degree 6500 //fixed this; L was not a long long in pure_betti and was overflowing

test_program is mainly just a driver to test functionality as I write code and/or edit functions
usage: test_program

parse_huge_output is made to take the merged output file made by merge_outputs.sh and turn it into a nice formatted file of degree sequences
usage: parse_huge_output [input file] [output file]

seq_funcs.h contains the declarations and brief descriptions of the functions having to do with degree sequences, betti numbers, etc, that I have implemented and/or are in development


TODO: 
-Verify data collected for c=4 and c=5
-Implement Erman's obstructions
-Implement flag functionality for the programs 

DONE:
-Fix problem where gds(3,20,1) is not the same as gds(3,20,10) + gds(3,10,1)
-Revise pure_betti to not use huge numbers when not really needed 
-Modify/create overloaded function gen_deg_seqs that takes in a list of ones I've already tested (or their nondegen bases) and ignores them if they're a mult. of one I've done (since now we're running into
problems w memory since so many sequences are being generated; also runtime issues, we will still need to generate those seqs just to see if we can ignore them, but at least we won't have to call test_conjs
on them, which takes quite a while. But gcd prob isn't that bad tbh


purebetti c=4 starts to break at around 340
c=5 starts breaking around 70

./run_tests c=4 runs out of memory w/ increment 5 at 545-550
c=5 dies at 171-175

Starting to get some false positives around d=70 for c=6 (saying a degseq is violator when it is not)
eg {0,69,71,73,74,75,76}, {0,71,73,74,75,76,77}
Fixed this, need t restart at d=70 for c=6 (I was storing the products and LLBC sum as ints instead of long longs)

{0,15,55,63,64,70} failed ^^^
