use 5.012002;
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME              => 'Frozen',
    LIBS              => ['-L../../libfrozen/.libs/ -lfrozen'],
    OBJECT            => 'swig_wrap.o',
    DEFINE            => '',
    INC               => '-I../../libfrozen/',
);
