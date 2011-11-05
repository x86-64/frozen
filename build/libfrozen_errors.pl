#!/usr/bin/perl

use File::Find;

my ($ebase, $estep, @allerrs);

my $errors_h="src/libfrozen/core/errors.h";
my $main_dir="src/libfrozen/";
my $errors_list_c="src/libfrozen/core/errors_list.c";

open FH, "<$errors_h";
while(my $line = <FH>){
        $ebase = int($1) if $line =~ /#\s*define\s+EBASE\s+(\d+)/;
        $estep = int($1) if $line =~ /#\s*define\s+ESTEP\s+(\d+)/;
}
close FH;

my @files_list;
find(
        {   
                wanted     => sub{ push @files_list, $1 if /^(.*\.(c|m4))$/i; },
                no_chdir   => 1
        }, $main_dir);

foreach my $file (@files_list){
        my ($emod, $errnum, $errmsg);
        my $ecount = 0;
	my $linen = 1;
        
        open FH, "<$file";
        while(my $line = <FH>){
                $emod = $1 if $line =~ /#\s*define\s+EMODULE\s*(\d+)/;
                
                if($line =~ /(?:debug|notice|warning|error)\s*\("([^"]+)"\)/){
                        $errmsg = $1;
                        $errmsg =~ s/\\n//g;
			$errnum = -($ebase + $estep * $emod + $linen);
                        
                        push @allerrs, { errnum => $errnum, errmsg => $errmsg, errfile => $file };
                        $ecount++;
                }
		$linen++;
        }
        close FH;
}

@allerrs = sort { $a->{errnum} <=> $b->{errnum} } @allerrs;

open FH, ">$errors_list_c";
print FH q!
typedef struct err_item {
        intmax_t    errnum;
        const char *errmsg;
} err_item;
static err_item errs_list[] = {
!;

foreach my $e (@allerrs){
        print FH " { ", $e->{errnum}, ", \"", $e->{errfile}, ": ", $e->{errmsg}, "\" },\n";
}

print FH q!
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
!;

