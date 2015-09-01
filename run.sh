# compile & run
rm output/*
g++ multi.cpp -I ./Eigen/ -std=c++11 -O3 -o multi 
./multi 0 > output/res.txt
