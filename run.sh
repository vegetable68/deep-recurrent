# compile & run
rm Alloutput/*
g++ lrChange.cpp -I ./Eigen/ -std=c++11 -O3 -o lr 
./lr 0 > Alloutput/res.out 
