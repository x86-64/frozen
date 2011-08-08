AC_DEFUN([FROZEN_MODULE],[
	define([module_name],translit($1,_,-))
	define([module_action],ifelse($2,yes,disable,enable))
	AC_ARG_ENABLE(module$1,AS_HELP_STRING([--]module_action()[-module-]module_name(),module_action()[ module ]module_name()))
	
	if test "$2" = "yes"; then
		if test "${enable_module$1}" = "yes"; then
			enable_module$1="no"
		else
			enable_module$1="yes"
		fi
	fi
	
	if test "${enable_module$1}" = "yes"; then
		MODULE_DIR_$1="$1"
		MODULE_DEP_$1="$3"
		MODULE_SELECTED="$MODULE_SELECTED $1"
	fi
	
	if test "$MODULE_MAKEFILE_$1" = ""; then
		GEN_MAKEFILES="$GEN_MAKEFILES src/modules/$1/Makefile"
		MODULE_DIST_DIRS="$MODULE_DIST_DIRS $1"
		AC_SUBST([MODULE_DIST_DIRS])
		MODULE_MAKEFILE_$1=yes
	fi
])

AC_DEFUN([FROZEN_MODULE_END],[
	MODULE_SELECTED_DEPS=$MODULE_SELECTED
	for m in $MODULE_SELECTED; do
		eval m_deps="\$MODULE_DEP_$m"
		for d in $m_deps; do
			MODULE_SELECTED_DEPS=$( echo $MODULE_SELECTED_DEPS | sed "s|$d|$d $m|" | sed "s|$m||" )
			# add this module after dependency and remove first occurence of module, leaving inserted
		done;
	done;
	for m in $MODULE_SELECTED_DEPS; do
		eval m_folder="\$MODULE_DIR_$m"
		MODULE_DIRS="$MODULE_DIRS $m_folder"
		MODULE_DIRS=$(echo $MODULE_DIRS | sed "s|$m_folder|REPLACEMEBACK|" | sed "s|$m_folder||g" | sed "s|REPLACEMEBACK|$m_folder|" )
		
	done;
	AC_SUBST([MODULE_DIRS])
])

