AC_DEFUN([FROZEN_BACKEND],[
	define([backend_name],translit($1,_,-))
	define([backend_action],ifelse($2,yes,disable,enable))
	AC_ARG_ENABLE($1,AS_HELP_STRING([--]backend_action()[-backend-]backend_name(),backend_action()[ backend ]backend_name()))
	
	if test "$3" = "yes"; then
		if test "${enable_$1}" = "yes"; then
			enable_$1="no"
		else
			enable_$1="yes"
		fi
	fi
	
	if test "${enable_$1}" = "yes"; then
		BACKEND_DIR_$1="$2"
		BACKEND_DEP_$1="$4"
		BACKEND_OBJECTS_$2="$BACKEND_OBJECTS_$2 libfrozen_backend_$2_la-$1.lo"
		BACKEND_SELECTED="$BACKEND_SELECTED $1"

		AC_SUBST([BACKEND_OBJECTS_$2])
	fi
	
	if test "$BACKEND_MAKEFILE_$2" = ""; then
		GEN_MAKEFILES="$GEN_MAKEFILES src/libfrozen/backends/$2/Makefile"
		BACKEND_DIST_DIRS="$BACKEND_DIST_DIRS $2"
		AC_SUBST([BACKEND_DIST_DIRS])
		BACKEND_MAKEFILE_$2=yes
		
		BACKEND_LIBS_SELECTED="$BACKEND_LIBS_SELECTED $2/libfrozen_backend_$2.la"
		AC_SUBST([BACKEND_LIBS_SELECTED])
	fi
])

AC_DEFUN([FROZEN_BACKEND_END],[
	BACKEND_SELECTED_DEPS=$BACKEND_SELECTED
	for m in $BACKEND_SELECTED; do
		eval m_deps="\$BACKEND_DEP_$m"
		for d in $m_deps; do
			BACKEND_SELECTED_DEPS=$( echo $BACKEND_SELECTED_DEPS | sed "s|$d|$d $m|" | sed "s|$m||" )
			# add this module after dependency and remove first occurence of module, leaving inserted
		done;
	done;
	for m in $BACKEND_SELECTED_DEPS; do
		eval m_folder="\$BACKEND_DIR_$m"
		BACKEND_DIRS="$BACKEND_DIRS $m_folder"
		BACKEND_DIRS=$(echo $BACKEND_DIRS | sed "s|$m_folder|REPLACEMEBACK|" | sed "s|$m_folder||g" | sed "s|REPLACEMEBACK|$m_folder|" )
		
	done;
	AC_SUBST([BACKEND_DIRS])
])

