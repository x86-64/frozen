#!/bin/sh

# cleanup
rm -rf test/data_backend_file.dat      &>/dev/null
rm -rf test/data_backend_locator.dat   &>/dev/null
find . -name '*.gcov' -delete

# backends
planned=`find src/backends/ -name '*.c' -printf 'backends/%P '`
cat src/Makefile.am | sed "s#\(backends_sources=\).*#\1$planned#" > src/Makefile.tmp
mv src/Makefile.tmp src/Makefile.am

# data types
planned=`find src/data/ -name '*.c' -printf 'data/%P '`
cat src/Makefile.am | sed "s#\(data_sources=\).*#\1$planned#" > src/Makefile.tmp
mv src/Makefile.tmp src/Makefile.am

cat src/data/*.c | perl -e '
$e=""; $f=""; $a="";
while(<STDIN>){
	if(/REGISTER_DATA\(([^,]*),(.*)\)/){
		$n=lc(substr($1,5));
		$t=$1;
		$r=$2;
		$e .= "\t$t,\n";
		$a .= "\t{ \"$n\",$t,$r },\n";
	}
	if(/{$/ and /[\(\)]/){
		s/{$/;/g;
		$f .= $_;
	}
}
$e = "typedef enum data_type {\n$e} data_type;\n\n";
$data_types_c = "#include <libfrozen.h>\n\ndata_proto_t data_protos[] = {\n$a};\nsize_t data_protos_size = sizeof(data_protos) / sizeof(data_proto_t);\n\n";
$data_types_h = "#ifndef DATA_PROTOS_H\n#define DATA_PROTOS_H\n$e$f\n\n#endif // DATA_PROTOS_H";

open FH, ">src/data_protos.c"; print FH $data_types_c; close FH;
open FH, ">src/data_protos.h"; print FH $data_types_h; close FH;
'

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

