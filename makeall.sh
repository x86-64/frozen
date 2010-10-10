#!/bin/sh

# cleanup
find . -name '*.gcov' -delete

cat test/test_*.c | perl -e '
$e=""; $f=""; $e="";
while(<STDIN>){
	if(/REGISTER_TEST\(([^,]*),\s*(.*)\)/){
		my ($s,$n) = ($1, $2);
		$e .= "\ttcase_add_test(tc_$s, $n);\n";
	}
}

$files =
	join("\n", 
		map { "#include <$_>"; }
		split /\n/, 
		`find test/ -name \"test_*.c\" -printf \"%P\n\" | sort`
	);

$test_list_c = "$files\n\nvoid test_list(void){\n$e}\n";

open FH, ">test/tests_list.c"; print FH $test_list_c; close FH;
'

make clean all check

cd src
if [[ -e libfrozen_la-libfrozen.gcno ]]; then
	find . -maxdepth 1 -name '*.gcno' | sed "s/\.gcno//g" | awk '{print "gcov " $1 " -o .libs/" $1 ".gnco" }' | sh &>/dev/null
	find . -name '*.gcno' -delete
	find . -name '*.gcda' -delete 
	find . -name '*.h.gcov' -delete
fi;
cd ..

