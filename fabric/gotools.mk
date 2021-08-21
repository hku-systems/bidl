# Copyright IBM Corp All Rights Reserved.
# Copyright London Stock Exchange Group All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

GOTOOLS = counterfeiter dep golint goimports protoc-gen-go ginkgo gocov gocov-xml misspell mockery manifest-tool
BUILD_DIR ?= .build
GOTOOLS_GOPATH ?= $(BUILD_DIR)/gotools
GOTOOLS_BINDIR ?= $(GOPATH)/bin

# go tool->path mapping
go.fqp.counterfeiter := github.com/maxbrunsfeld/counterfeiter
go.fqp.gocov         := github.com/axw/gocov/gocov
go.fqp.gocov-xml     := github.com/AlekSi/gocov-xml
go.fqp.goimports     := golang.org/x/tools/cmd/goimports
go.fqp.golint        := github.com/golang/lint/golint
go.fqp.manifest-tool := github.com/estesp/manifest-tool
go.fqp.misspell      := github.com/client9/misspell/cmd/misspell
go.fqp.mockery       := github.com/vektra/mockery/cmd/mockery

.PHONY: gotools-install
gotools-install: $(patsubst %,$(GOTOOLS_BINDIR)/%, $(GOTOOLS))

.PHONY: gotools-clean
gotools-clean:
	-@rm -rf $(BUILD_DIR)/gotools

# Special override for protoc-gen-go since we want to use the version vendored with the project
gotool.protoc-gen-go:
	@echo "Building github.com/golang/protobuf/protoc-gen-go -> protoc-gen-go"
	@mkdir -p $(GOTOOLS_GOPATH)/src/github.com/golang/protobuf/
	@cp -R $(GOPATH)/src/github.com/hyperledger/fabric/vendor/github.com/golang/protobuf/* $(GOTOOLS_GOPATH)/src/github.com/golang/protobuf
	@GOPATH=$(abspath $(GOTOOLS_GOPATH)) GOBIN=$(abspath $(GOTOOLS_BINDIR)) go install github.com/golang/protobuf/protoc-gen-go

# Special override for ginkgo since we want to use the version vendored with the project
gotool.ginkgo:
	@echo "Building github.com/onsi/ginkgo/ginkgo -> ginkgo"
	@mkdir -p $(GOTOOLS_GOPATH)/src/github.com/onsi/ginkgo/ginkgo/
	@cp -R $(GOPATH)/src/github.com/hyperledger/fabric/vendor/github.com/onsi/ginkgo/* $(GOTOOLS_GOPATH)/src/github.com/onsi/ginkgo
	@GOPATH=$(abspath $(GOTOOLS_GOPATH)) GOBIN=$(abspath $(GOTOOLS_BINDIR)) go install github.com/onsi/ginkgo/ginkgo

# Lock to a versioned dep
gotool.dep: DEP_VERSION ?= "v0.5.0"
gotool.dep:
	@GOPATH=$(abspath $(GOTOOLS_GOPATH)) go get -d -u github.com/golang/dep
	@git -C $(abspath $(GOTOOLS_GOPATH))/src/github.com/golang/dep checkout -q $(DEP_VERSION)
	@echo "Building github.com/golang/dep $(DEP_VERSION) -> dep"
	@GOPATH=$(abspath $(GOTOOLS_GOPATH)) GOBIN=$(abspath $(GOTOOLS_BINDIR)) go install -ldflags="-X main.version=$(DEP_VERSION) -X main.buildDate=$$(date '+%Y-%m-%d')" github.com/golang/dep/cmd/dep
	@git -C $(abspath $(GOTOOLS_GOPATH))/src/github.com/golang/dep checkout -q master

# Default rule for gotools uses the name->path map for a generic 'go get' style build
gotool.%:
	$(eval TOOL = ${subst gotool.,,${@}})
	@echo "Building ${go.fqp.${TOOL}} -> $(TOOL)"
	@GOPATH=$(abspath $(GOTOOLS_GOPATH)) GOBIN=$(abspath $(GOTOOLS_BINDIR)) go get ${go.fqp.${TOOL}}

$(GOTOOLS_BINDIR)/%:
	$(eval TOOL = ${subst $(GOTOOLS_BINDIR)/,,${@}})
	@$(MAKE) -f gotools.mk gotool.$(TOOL)

