#!/usr/bin/perl

use File::Find;

my ($estep, $ecomm, @allerrs);

my $errors_h="/usr/include/frozen/errors.h";
my $main_dir=$ARGV[0];
my $errors_list_c="$main_dir/errors_list.c";

open FH, "<$errors_h";
while(my $line = <FH>){
        $estep = int($1) if $line =~ /#\s*define\s+ESTEP\s+(\d+)/;
        $ecomm = int($1) if $line =~ /#\s*define\s+ECOMMON\s+(\d+)/;
}
close FH;


my @files_list;
find(
        {   
                wanted     => sub{ push @files_list, $1 if /^(.*\.(c|m4))$/i; },
                no_chdir   => 1
        }, $main_dir);

foreach my $file (@files_list){
        my ($emod, $emod_name, $errnum, $errmsg);
	my $linen = 1;
        
        open FH, "<$file";
        while(my $line = <FH>){
                $emod = 0; # $1 if $line =~ /#\s*define\s+ERRORS_MODULE_ID\s*(\d+)/;
                
		if($line =~ /ERRORS_MODULE_NAME\s*"([^"]+)"/){
			$errmsg = $1;
                        $errmsg =~ s/\\n//g;
			$errnum = -($estep * $emod + 0);

                        push @allerrs, { errnum => $errnum, errmsg => "unknown error in module $errmsg" };
			$emod_name = $errmsg;
		}

                if($line =~ /(?:debug|notice|warning|error)\s*\("([^"]+)"\)/){
                        $errmsg = $1;
                        $errmsg =~ s/\\n//g;
			$errnum = -($linen + $ecomm);
                        
                        push @allerrs, { errnum => $errnum, errmsg => $errmsg, errfile => $file };
                }
		if($line =~ /errorn\s*\(([^)]+)\)/){
                        $errmsg = $1;
                        $errmsg =~ s/\\n//g;
			$errval = 0;
			if($errmsg eq "EFAULT"){    $errval = 14 };
			if($errmsg eq "EINVAL"){    $errval = 22 };
			if($errmsg eq "EEXIST"){    $errval = 17 };
			if($errmsg eq "ENOMEM"){    $errval = 12 };
			if($errmsg eq "ENOSPC"){    $errval = 28 };
			if($errmsg eq "ENOSYS"){    $errval = 38 };
			if($errmsg eq "ENOENT"){    $errval = 2  };
			if($errmsg eq "EBADF"){     $errval = 9  };
			if($errmsg eq "EOVERFLOW"){ $errval = 75 };
			$errnum = -($estep * $emod + $errval);
			
                        push @allerrs, { errnum => $errnum, errmsg => "$errmsg in module $emod_name", errfile => $file };
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

my $dedup = {};
foreach my $e (@allerrs){
	next if($dedup->{$e->{errnum}} == 1);
	$dedup->{$e->{errnum}} = 1;
	
	if($e->{errfile}){
		print FH " { ", $e->{errnum}, ", \"", $e->{errfile}, ": ", $e->{errmsg}, "\" },\n";
	}else{
		print FH " { ", $e->{errnum}, ", \"", $e->{errmsg}, "\" },\n";
	}
}

print FH q!
};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
!;

