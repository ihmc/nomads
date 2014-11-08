PROJECTNAME = DisServiceProxy.net
OUTDIR = bin
CS = gmcs

# List of source files to compile
CSSOURCES = ../Properties/AssemblyInfo.cs ../CommHelper.cs \
	../DisServiceProxyCallbackHandler.cs ../DisServiceProxy.cs

CSFLAGS =

all: dirs $(OUTDIR)/$(PROJECTNAME).dll

clean: 
	rm -f $(OUTDIR)/$(PROJECTNAME).dll

dirs:
	mkdir -p $(OUTDIR)

# generic rule for building a dot net library
$(OUTDIR)/%.dll : $(CSSOURCES) $(DOTNET_RESOURCES)
	$(CS) $(CSFLAGS) -t:library -out:$(OUTDIR)/$(PROJECTNAME).dll \
	$(CSSOURCES)

