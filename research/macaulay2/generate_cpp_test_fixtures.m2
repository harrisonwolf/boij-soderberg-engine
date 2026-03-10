-- Generate the baked fixture values used by apps/algorithm_test_driver.cc.
-- The C++ runner does not call M2 at runtime; this script is only for refreshing
-- the prewritten expected outputs from native Macaulay2 + BoijSoederberg.

loadPackage "BoijSoederberg"

degreeCases = {
    {0,1}, {0,2}, {0,1,2}, {0,1,3}, {0,2,3}, {0,2,4}, {0,3,4}, {0,1,2,3}, {0,1,3,4}, {0,2,3,4},
    {0,2,4,6}, {0,3,5,8}, {0,2,7,8}, {0,4,7,9}, {0,1,2,4,5}, {0,1,3,4,6}, {0,2,4,5,7}, {0,2,5,6,9}, {0,3,4,7,8}, {0,4,6,9,10}
    };

testBEH = (B) -> (
    c := #B - 1;
    any(toList(0..c), i -> B#i < binomial(c,i))
    );

testLLBC = (B) -> (
    c := #B - 1;
    sum B < ((2^c) * 3 / 2)
    );

piValues = (D) -> (
    out := {{1,1}};
    for i from 1 to (#D - 1) do (
        num := 1;
        den := 1;
        for j from 1 to (#D - 1) do (
            if i =!= j then (
                num = num * (D#j);
                den = den * abs((D#j) - (D#i));
                );
            );
        g := gcd(num, den);
        out = append(out, {(num // g), (den // g)});
        );
    out
    );

say "-- Degree fixtures";
for idx from 0 to #degreeCases - 1 do (
    D := degreeCases#idx;
    B := pureBetti D;
    << toString(idx + 1) << ": D=" << toString(D)
      << " B=" << toString(B)
      << " BEH=" << toString(not testBEH(B))
      << " LLBC=" << toString(not testLLBC(B))
      << " L=" << toString(first B)
      << " pi=" << toString(piValues D)
      << endl;
    );

say "-- Bad-search fixtures";
searchCases = {
    {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, {3,3}, {3,4}, {3,5}, {3,6},
    {3,7}, {3,8}, {4,4}, {4,5}, {4,6}, {4,7}, {4,8}, {4,9}, {5,5}, {5,6}
    };
for idx from 0 to #searchCases - 1 do (
    c := (searchCases#idx)#0;
    d := (searchCases#idx)#1;
    bad := sort select(subsets(toList(1..d), c)/(s -> {0}|s), D -> (
        B := pureBetti D;
        testBEH(B) or testLLBC(B)
        ));
    << toString(idx + 1) << ": c=" << toString(c) << " d=" << toString(d) << " -> " << toString(bad) << endl;
    );

say "-- Self-dual L=1 first 20";
count := 0;
for a from 1 to 98 do (
    for b from (a + 1) to 99 do (
        for c from (b + 1) to 100 do (
            D := {0,a,b,c,b+c-a,b+c};
            if count < 20 and first pureBetti D == 1 then (
                count = count + 1;
                << toString(count) << ": " << toString(D) << endl;
                );
            );
        );
    );

say "-- Large-Betti first 20 (sorted lexicographically)";
matches := sort select(subsets(toList(1..100),3)/(s -> {0}|s), D -> any(pureBetti D, b -> b > 50000));
for i from 0 to 19 do (
    D := matches#i;
    << toString(i + 1) << ": " << toString(D) << " -> " << toString(pureBetti D) << endl;
    );

exit 0
