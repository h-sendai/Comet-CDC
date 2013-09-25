SUBDIRS += CometCdcReader
SUBDIRS += CometCdcMonitor
SUBDIRS += CometCdcLogger

.PHONY: $(SUBDIRS)

all: $(SUBDIRS)
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

clean:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done
	rm -f omninames*.bak omninames*.log rtc.conf .confFilePath
