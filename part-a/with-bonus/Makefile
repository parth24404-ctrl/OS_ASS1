# Top level Makefile.
# Invokes make inside loader, launcher and test (in that order).
# lib_simpleloader.so and launch are produced directly inside ./bin,
# while the factorial executable stays inside ./test

SUBDIRS = loader launcher test

all:
	@mkdir -p bin
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d || exit 1; \
	done

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $$d clean; \
	done
	-@rm -rf bin
