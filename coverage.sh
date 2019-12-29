# Working workflow
mkdir `pwd`/build/coverage
rm -rf `pwd`/build/coverage/*
LLVM_PROFILE_FILE="`pwd`/build/coverage/cppalloc-unit-test-validity-cpp.profraw" `pwd`/build/unit_tests/cppalloc-unit-test-validity-cpp
llvm-profdata merge -output=`pwd`/build/coverage/code.profdata `pwd`/build/coverage/cppalloc-unit-test-*.profraw
llvm-cov show `pwd`/build/unit_tests/cppalloc-unit-test-validity-cpp -instr-profile=`pwd`/build/coverage/code.profdata `pwd`/include/*.* -path-equivalence -use-color --format html > `pwd`/build/coverage/coverage.html
llvm-cov export `pwd`/build/unit_tests/cppalloc-unit-test-validity-cpp -instr-profile=`pwd`/build/coverage/code.profdata `pwd`/include/*.* -path-equivalence -use-color --format lcov > `pwd`/build/coverage/coverage.lcov

genhtml --prefix `pwd`/build/unit_tests/cppalloc-unit-test-validity-cpp --ignore-errors source `pwd`/build/coverage/coverage.lcov \
--legend --title "cppalloc-unit-test-validity-cpp" --output-directory=`pwd`/build/coverage
