# compile & run
g++ multitask.cpp -I ./Eigen/ -std=c++11 -O3 -o multi 
./multi 0 > output/res_target.txt
