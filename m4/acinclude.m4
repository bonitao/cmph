AC_DEFUN([AC_ENABLE_CXXMPH], [AC_ARG_ENABLE([cxxmph],
	[  --enable-cxxmph	enable the c++ cxxmph library ],
	[case "${enableval}" in
		yes) cxxmph=true ;;
		no)  cxxmph=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cxxmph]) ;;
	esac],[cxxmph=false])])
