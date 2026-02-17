D = {0,2,3,5,6,7} --this is a degree sequence that defines our betti table's shape
--when we are in codimension 5 (c=5), our degree sequence has 6 numbers since indexing starts at 0
B = pureBetti D --this gives the smallest possible betti numbers that can arise from this shape; it's guaranteed that there exists an ideal with a betti table that is some multiple of these
pureBettiDiagram D --this shows the actual betti table
sum B --this returns the sum of the list B (which is the list of the betti numbers)
--in c=5, LLBC says the sum should be at least 2^5 * 1.5, or 48, for any pure resolution (betti table w one entry in each column)
B_0 --this gives the 1st betti number (index 0) (remember, indexing will always start at 0 for our purposes)
binomial(5,0) --this return 5 choose 0
--BEH states the i-th betti number should be greater than c choose i, to B_0 should be bigger than 5 choose 0 here.
B_0 >= binomial(5,0) --returns true in this case
for i from 0 to 5 do (
    print(B_i >= binomial(5,i)) --check each one against its binomial coeff
    ) --all are true in this case
all( {0,1,2,3,4,5}, i-> B_i >= binomial(5,i) ) --checks if this is true for all values of i in {0,1,2,3,4,5}
apply( {1,2,3}, i-> binomial(6,i) ) --apply takes a list ({1,2,3}, and sends each thing in the list here to 6 choose that thing
--so this return the list {6,15,20}
{1,2,3}/(i-> binomial(7,i)) -- "/" does the same thing as "apply"; this gives us back the list {7,21,35} - cool!
any({4,6,14,27,91},i-> isPrime(i)) --lets us know if anything in the list gives back true under the function --this one is false since none of those numbers are prime
any({4,6,101,37,45}, i-> isPrime(i)) --this one gives back true since some of those numbers (at least one) is prime
select({4,6,101,37,45}, i-> isPrime(i)) --this one actually returns us back the elements that pass! (in this case, 101 and 37)
c = {2,3,4,6,12,17,19,20,25,38,53,57,59,60,61,65,79}
select(toList(0..length(c)-1),i-> isPrime(c_i)) --we can't just pass in c as our list, we have to do this (i guess because macaualy2 says so)
--also, this returns the indices of c's primes, not the primes themselves
--how to define a function in M2
myFun = (a,b) -> (
    return toList(a..b)
    )
myFun(5,9) --gives back {5,6,7,8,9}
select(length(c),i-> isPrime(c_i)) --this also gives back the indices of the primes in c
subsets({1,2,3,4,5,6},3) --this gives back all 3-element subsets of [6]
S = {1,2,3,4,5}
subsets(S,2)
netList(subsets({1,2,3,4,5,6},3) / (i->{0}|i)/pureBettiDiagram)
--this takes every 3-elmt subset of {1..6}, eg, {2,3,4}, and does the diagram of the degree sequence {0,2,3,4} (so we are in c=3 here)
--so lets look at all possible degree sequences in c=4 with biggest number 6
possibles = subsets(toList(1..6),3)/(i->{0}|i) --take every 3-elmt subset of {1..6}, stick a 0 in front --this is much diff from subsets({0..6},4) since we need a zero in each one for sure
--in c=3, we need the sum of the betti nums to be at least 2^3 * 1.5 = 12
--let's see which degree sequences fail this
badOnes = select(possibles, i-> (sum pureBetti i) < 12) --we actually want the "indices" ie degree sequences here, not the sums, so this is good
--now let's take these bad degree sequences and see what their virtual betti tables look like
badOnes/pureBettiDiagram --this applies pureBettiDiagram to each item (degree sequence list) in badOnes
netList oo --this makes the output much nicer; oo refers to the last output in the terminal

R = QQ[x,y,z] --makes a polynomial ring over variables x, y, and z, and field is rationals Q
M = matrix{
    {0,x,y,z},
    {x,y,z,0}
}
--this makes a 2x4 matrix
C = res coker M --something to do with resolutions and cokernels i guess; it is doing all the syzygies and syzygies of syzygies etc
betti C --gives us the betti table of C, which is a chain complex (remember each mxn matrix in the syzygy chain is a function from Q^n -> Q^m)
C.dd --this shows us the actual syzygies that it did
