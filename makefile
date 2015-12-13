subdir=src
subsystem:
	$(MAKE) -C $(subdir)  
check: subsystem
	test/test.sh
clean:
	$(RM) chanvese compare && cd src && make clean
