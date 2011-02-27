use Data::Dumper;
use Test::More 'no_plan';
BEGIN { use_ok('Frozen') };

my ($ret, $hash, $backend);

$ret = Frozen::frozen_init();
        ok($ret == 0, "init");

$hash = Frozen::configs_string_parse(q!
        name   => "perl_test",
        chains => {
               { name => "file", filename => "data_perl_test.dat" }
        }
!);
        ok($hash, "configs_string_parse");

$backend = Frozen::backend_new($hash);
        ok($backend, "backend_new");

Frozen::backend_destroy($backend);
Frozen::hash_free($hash);
        
$ret = Frozen::frozen_destroy();
        ok($ret == 0, "destroy");

