GIT_BRANCH  := $(shell git rev-parse --abbrev-ref HEAD)
GIT_SHA     := $(shell git rev-parse --verify HEAD)
GIT_TAG     := $(shell git --no-pager describe --tags --always)
GIT_DATE    := $(firstword $(shell git --no-pager show --date=iso --format="%ad" --name-only))


CPPFLAGS += -DGIT_BRANCH=\"$(GIT_BRANCH)\" \
            -DGIT_SHA=\"$(GIT_SHA)\" \
            -DGIT_TAG=\"$(GIT_TAG)\" \
            -DGIT_DATE=\"$(GIT_DATE)\"
