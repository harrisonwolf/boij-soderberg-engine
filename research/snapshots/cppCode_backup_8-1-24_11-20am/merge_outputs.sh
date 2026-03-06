#shell script that will take all normal (non-rinsed) degree sequences from burn files 1-119 in c=3 and compile them into a single text file, then I can run remove_duplicates on this file
#also in remove_duplicates, I only need to multiple each new degseq by ciel(max_degree_seen / c)
#I will literally just smash together all of the output files and do the parsing in C++
let c=6
let increment=2 #incrmement used in run_tests to make the burnfiles
touch ~/betti/code/cppCode/burnfiles/c$c/huge_output.txt
for (( n=0; n<=22; n++ ))
do
	let start=($n*$increment + 1)
	let end=($start+$increment - 1)
	cat ~/betti/code/cppCode/burnfiles/c$c/burn$start-$end.txt >> ~/betti/code/cppCode/burnfiles/c$c/huge_output.txt
done
