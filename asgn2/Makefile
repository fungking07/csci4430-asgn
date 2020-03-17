all: 
	@if [ "`uname -s`" = "Linux" ]; then \
		make -f Makefile.linux ; \
	else \
		make -f Makefile.sunos ; \
	fi

clean: 
	@if [ "`uname -s`" = "Linux" ]; then \
		make clean -f Makefile.linux ; \
	else \
		make clean -f Makefile.sunos ; \
	fi
