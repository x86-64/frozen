FROZEN_DATA(off_t, numeric, uint, yes,, 13)
FROZEN_DATA(size_t, numeric, uint, yes,, 12)
FROZEN_DATA(uint_t, numeric, uint, yes,, 14)
FROZEN_DATA(int_t, numeric, uint, yes,, 15)
FROZEN_DATA(int8_t, numeric, uint, yes,, 4)
FROZEN_DATA(int16_t, numeric, uint, yes,, 5)
FROZEN_DATA(int32_t, numeric, uint, yes,, 6)
FROZEN_DATA(int64_t, numeric, uint, yes,, 7)
FROZEN_DATA(uint8_t, numeric, uint, yes,, 8)
FROZEN_DATA(uint16_t, numeric, uint, yes,, 9)
FROZEN_DATA(uint32_t, numeric, uint, yes,, 10)
FROZEN_DATA(uint64_t, numeric, uint, yes,, 11)

AC_ARG_ENABLE(uint-optimize,   AS_HELP_STRING([--disable-uint-optimize],  [do not optimize uints to have mimimal size in memory]))
if test "x$enable_uint-optimize" != "xyes"; then
	AC_DEFINE([OPTIMIZE_UINT], [1], [Define if optimization of uints enabled])
fi
