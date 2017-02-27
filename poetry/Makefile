bundle:
	$(MAKE) -C src all
	touch service/ro/ct
	rm service/ro/ct
	touch service/rw/database
	rm -rf service/rw/database
	touch scripts/ct
	rm scripts/ct
	touch scripts/database
	rm -rf scripts/database
	cp src/poetry service/ro/
	rm -f ../poetry.tgz
	tar caf ../poetry.tgz *
	@echo "#### Double check ../sample_c.tgz and submit it :) ####"
