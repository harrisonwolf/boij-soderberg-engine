#shell script that will take all normal (non-rinsed) degree sequences from burn files 1-119 in c=3 and compile them into a single text file, then I can run remove_duplicates on this file
#also in remove_duplicates, I only need to multiple each new degseq by ciel(max_degree_seen / c)
#I will literally just smash together all of the output files and do the parsing in C++
touch ~/betti/code/cppCode/burnfiles/c3/huge_output.txt
for (( n=0; n<=119; n++ ))
do
	cat ~/betti/code/cppCode/burnfiles/c3/burn$n.txt >> ~/betti/code/cppCode/huge_output.txt
done
