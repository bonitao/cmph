AC_DEFUN([AC_ENABLE_CXXMPH], [AC_ARG_ENABLE([cxxmph],
	[  --enable-cxxmph	enable the c++ cxxmph library ],
	[case "${enableval}" in
		yes) cxxmph=true ;;
		no)  cxxmph=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-cxxmph]) ;;
	esac],[cxxmph=false])])

AC_DEFUN([AC_ENABLE_BENCHMARKS], [AC_ARG_ENABLE([benchmarks],
	[  --enable-benchmarks	enable cxxmph benchmarks against other libs ],
	[case "${enableval}" in
		yes) benchmarks=true ;;
		no)  benchmarks=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-benchmarks]) ;;
	esac],[benchmarks=false])])
