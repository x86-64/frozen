use Data::Dumper;
BEGIN {
        push @INC, ("./blib/lib", "./blib/arch");
        require "Frozen.pm";
}
sub ok { croak(shift) unless shift; }

my ($ret, $config, $r_create, $backend, $string);

# init all
        $ret = Frozen::frozen_init();
                ok($ret == 0, "init");

        $config = Frozen::configs_string_parse(q!
                name   => "perl_test",
                chains => {
                       { name => "file", filename => "data_perl_test.dat" }
                }
        !);
                ok($config, "configs_string_parse");

        $backend = Frozen::backend_new($config);
                ok($backend, "backend_new");

# test creates
        $r_create = Frozen::hash_new(10);
                Frozen::hash_set($r_create, "action", "int32",  sprintf("%d", $Frozen::ACTION_CRWD_CREATE));
                Frozen::hash_set($r_create, "buffer", "string", "Hello, world!");
                Frozen::hash_set($r_create, "size",   "size_t", sprintf("%d", 100));

                $ret = Frozen::backend_query($backend, $r_create); ok($ret >= 0, "r_create");
        Frozen::hash_free($r_create);

# test reads
        $r_read = Frozen::hash_new(10);
                Frozen::hash_set($r_read, "action", "int32",  sprintf("%d", $Frozen::ACTION_CRWD_READ));
                Frozen::hash_set($r_read, "offset", "off_t",  sprintf("%d", 0));
                Frozen::hash_set($r_read, "buffer", "string", " " x 100);
                Frozen::hash_set($r_read, "size",   "size_t", sprintf("%d", 100));
                
                $ret = Frozen::backend_query($backend, $r_read); ok($ret >= 0, "r_read");

        $string = Frozen::hash_get($r_read, "buffer");
                ok(substr($string, 0, 13) eq "Hello, world!", "r_read data");

        Frozen::hash_free($r_read);

# free all
        Frozen::backend_destroy($backend);
        Frozen::hash_free($config);
                
        $ret = Frozen::frozen_destroy();
                ok($ret == 0, "destroy");

