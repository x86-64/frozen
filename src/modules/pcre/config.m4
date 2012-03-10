AC_CHECK_LIB(pcre, pcre_exec, [HAVE_PCRE=1], [HAVE_PCRE=0])
AS_IF([test "x$HAVE_PCRE" = "x1"], [
        FROZEN_MODULE(pcre, yes)
])
