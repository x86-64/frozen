use Data::Dumper;
BEGIN {
        push @INC, ("./blib/lib", "./blib/arch");
        require "Frozen.pm";
}
sub ok { die(shift) unless shift; }

exit; # BAD BAD BAD

my ($ret, $config, $r_create, $machine, $string);

# init all
        $config = Frozen::configs_string_parse(q!
        	{ name => "perl_test", class => "file", filename => "data_perl_test.dat" }
        !);
                ok($config, "configs_string_parse");

        $machine = Frozen::machine_new($config);
                ok($machine, "machine_new");

# test creates
        $ret = Frozen::query($machine, {
		action => { uint32_t => $Frozen::ACTION_CREATE                },
		buffer => { string_t => "Hello, world!"                            },
		size   => { size_t   => 100                                        }
	});
	ok($ret >= 0, "r_create");

# test reads
	$ret = Frozen::query($machine, {
		action => { uint32_t => $Frozen::ACTION_READ                 },
		offset => { off_t    => 0                                         },
		buffer => { string_t => " " x 100                                 },
		size   => { size_t   => 100                                       }
	}, sub {
		# BAD BAD BAD
		#$string = Frozen::hash_get(shift, "buffer");
	});
	ok($ret >= 0,                                   "r_read");
	ok(substr($string, 0, 13) eq "Hello, world!",   "r_read data");
	
# free all
        Frozen::shop_destroy($machine);
        Frozen::hash_free($config);
	
