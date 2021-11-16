#!/bin/bash 

execution_file=../bin/project

config1=../testcases-release1/config/cache2.org
config2=../testcases-release2/config/cacheA.org
config3=../testcases-release2/config/cacheB.org
config4=../testcases-release2/config/cacheC.org
config5=../testcases-release2/config/cacheD.org
config6=../testcases-release2/config/cacheE.org

testbench1=../testcases-release1/bench/reference2.lst
testbench2=../testcases-release2/bench/DataReference_n_comp.lst
testbench3=../testcases-release2/bench/DataReference_n_real.lst
testbench4=../testcases-release2/bench/InstReference_iir_one.lst
testbench5=../testcases-release2/bench/InstReference_lms.lst
testbench6=../testcases-release2/bench/randcase1.lst


myconfig=myconfig.org

output=../output/index.rpt




make clean
make

# clean the output
rm $output

# run
./$execution_file $myconfig $testbench1 $output

# verify
#./../verifier/verify $config4 $testbench2 $output

