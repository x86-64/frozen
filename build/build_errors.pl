#!/usr/bin/perl

use File::Find;

my ($ebase, $estep, @allerrs);

my $main_dir=@ARGV[0];
my $errors_list_c=@ARGV[0]."/errors_list.c";

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
                $emod = 0; #$1 if $line =~ /#\s*define\s+EMODULE\s*(\d+)/;
                
                if($line =~ /(?:debug|notice|warning|error)\s*\("([^"]+)"\)/){
                        $errmsg = $1;
                        $errmsg =~ s/\\n//g;
			$errnum = -($linen);
                        
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
static err_item errs_list[] = {
!;

foreach my $e (@allerrs){
        print FH " { ", $e->{errnum}, ", \"", $e->{errfile}, ": ", $e->{errmsg}, "\" },\n";
}

print FH q!
	{ 0, NULL }
};
!;

