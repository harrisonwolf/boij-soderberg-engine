let c=8 #our codimension
let increment=1 #how many max degrees d to test in one run
touch ~/betti/code/cppCode/burnfiles/c$c/huge_output.txt 
for (( n=29; n<=500; n++ ))
do
	let start=($n*$increment + 1)
	let end=($start+$increment - 1)
	echo "running gen_bad_ones on $start to $end"
	touch ./burnfiles/c$c/burn$start-$end.txt
	./bad_one_generator ./burnfiles/c$c/burn$start-$end.txt $c $end $start
	cat ./burnfiles/c$c/burn$start-$end.txt >> ~/betti/code/cppCode/burnfiles/c$c/huge_output.txt #just merge the outputs in this
done
