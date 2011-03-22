#!/usr/bin/perl

my ($ebase, $estep, @allerrs);

open FH, "<errors.h";
while(my $line = <FH>){
        $ebase = int($1) if $line =~ /#\s*define\s+EBASE\s+(\d+)/;
        $estep = int($1) if $line =~ /#\s*define\s+ESTEP\s+(\d+)/;
}
close FH;

foreach my $file (@ARGV){
        my ($emod, $errnum, $errmsg);
        my $ecount = 0;
        
        open FH, "<$file";
        while(my $line = <FH>){
                $emod = $1 if $line =~ /#\s*define\s+EMODULE\s*(\d+)/;
                
                if($line =~ /(?:debug|notice|warning|error)\s*\("([^"]+)"\)/){
                        $errmsg = $1;
                        $errnum = -($ebase + $estep * $emod + $ecount);
                        
                        push @allerrs, { errnum => $errnum, errmsg => $errmsg, errfile => $file };
                        $ecount++;
                }
        }
        close FH;
}

@allerrs = sort { $a->{errnum} <=> $b->{errnum} } @allerrs;

foreach my $e (@allerrs){
        print "ERROR_CALLBACK(", $e->{errnum}, ", `", $e->{errfile}, ": ", $e->{errmsg}, "')\n";
}
