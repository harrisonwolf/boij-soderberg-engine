#shell script that takes the huge output file, parses it, rinses it, and writes the unique seqs to a new file
let c=8
rm ~/betti/code/cppCode/burnfiles/c$c/huge_output_parsed.txt
touch ~/betti/code/cppCode/burnfiles/c$c/huge_output_parsed.txt
~/betti/code/cppCode/bin/parse_huge_output ~/betti/code/cppCode/burnfiles/c$c/huge_output.txt ~/betti/code/cppCode/burnfiles/c$c/huge_output_parsed.txt #parse
rm ~/betti/code/cppCode/burnfiles/c$c/huge_output_unique.txt
touch ~/betti/code/cppCode/burnfiles/c$c/huge_output_unique.txt
~/betti/code/cppCode/bin/remove_duplicates ~/betti/code/cppCode/burnfiles/c$c/huge_output_parsed.txt ~/betti/code/cppCode/burnfiles/c$c/huge_output_unique.txt
