LLVMPASSPATH="../../My Solution/build/catpass/libCatPass.so"
INCLUDES_TO_CONSIDER=`pkg-config --cflags libCAT`
LIBS_TO_CONSIDER=`pkg-config --libs libCAT`

all: program_optimized.bc program.bc

program_optimized.bc: program.bc
	opt -load ${LLVMPASSPATH} -basicaa -CAT $< -o $@ 2>myout
	llvm-dis $@

program.bc: program.c
	clang ${INCLUDES_TO_CONSIDER} -O0 -emit-llvm -c $<
	opt -mem2reg $@ -o $@
	llvm-dis $@

program: program.bc
	clang -O3 ${LIBS_TO_CONSIDER} $< -o $@

program_optimized: program_optimized.bc
	clang -O3 ${LIBS_TO_CONSIDER} $< -o $@

program_output: program
	./$< > $@

program_optimized_output: program_optimized
	./$< > $@

check: program_optimized.bc program_optimized_output
	../misc/check.sh "program_optimized_output" output2 "program_optimized.ll" output

clean:
	rm -f program program_optimized program_optimized_output *.bc *.ll a.out diff/* ;
	cp bitcode/* . ;
