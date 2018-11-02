all: trch rtps

trch:
	$(MAKE) -C trch
rtps:
	$(MAKE) -C rtps

clean:
	$(MAKE) -C trch clean
	$(MAKE) -C rtps clean

.PHONY: rtps trch
