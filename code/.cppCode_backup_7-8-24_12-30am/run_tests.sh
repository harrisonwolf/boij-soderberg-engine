for (( n=19; n<=500; n++ ))
do
	let c=3
	let start=($n*50)
	let end=($start+50)
	echo "running gen_bad_ones on $start to $end"
	touch ./burnfiles/c$c/burn$start-$end.txt
	./bad_one_generator ./burnfiles/c$c/burn$start-$end.txt $c $end $start
done
