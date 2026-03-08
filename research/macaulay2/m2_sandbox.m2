loadPackage "BoijSoederberg"

R = QQ[x,y,z]

I = ideal(x^2+y*z,x*y,x*z,y*z)
C = res I
C.dd
betti res I
codim I

pureBettiDiagram({0,2,3,4})
pureBetti I
viewHelp pureBetti

--{0,143,429,430}

143*3

35*4 + 4

144*3 + 6

35*6 -2
208*3 + 6

pureBetti {0,143,435,436}
pureBetti {0,144,438,439}
pureBetti {0,208,630,631}


findExample = (x) -> (
    l = {0}|{x}|{x*3 + 6}|{x*3 + 7};
    pureBetti l
    )

findExample = (x,y) -> (
    l = {0}|{x}|{y}|{y+1};
    pureBetti l
    )

l = {0,1,3,4}
B = pureBetti l
B_1


--D = {0,p,q,q+1} s.t. pureBetti(D)_1 = 2
p=1000
q=5000
select(toList(1..p), i-> (
	flag = false;
	for j in toList(i+1..q) do (
	    --print(i,j);
            B = findExample(i,j);
	    if (B_1 == 2) then (
		(x,y) = (i,j);
		print(0,x,y,y+1);
		flag = true;
		);
	    );
	flag
	)
    )

p=1000
q=5000
for i in (1..p) do (
    for j in (i+1..q) do (
	B = findExample(i,j);
	if (B_1 == 2) then (
	    print(0,i,j,j+1)
	    );
	);
    )

--{0,204,696,697}
pureBetti {0,204,696,697}
select(toList(205..1000), i-> (
	l = {0,204}|{i}|{i+1};
	B = pureBetti l;
	B_1 == 2
	)
    )

pureBetti {0,204,696,697}
select(toList(1190..5000), i-> (
	l = {0,1189}|{i}|{i+1};
	B = pureBetti l;
	B_1 == 2
	)
    )

--{0,1189,4059,4060}
pureBetti {0,1189,4059,4060}
204*6 - 36

1189*6 - 204
select(toList(6931..80000), i-> (
	l = {0,6930}|{i}|{i+1};
	B = pureBetti l;
	B_1 == 2
	)
    )

pureBetti{0,6930,23660,23661}

M = randomModule({0,1,5,6},2)
pureBettiDiagram({0,1,5,6})
C = res M
betti C

