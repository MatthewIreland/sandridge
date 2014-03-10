# syncs eclipse workspace/git repository with castalia for simulation
# Matthew Ireland, University of Cambridge

########## SOFTWARE ##########
rsync = rsync --exclude '*~'

########## LOCATION DECLARATIONS ##########
########## LOCATION DECLARATIONS -- Castalia ##########
castaliaRoot        = ~/proj/sim/castalia
castaliaSrc         = $(castaliaRoot)/src
castaliaMac         = $(castaliaSrc)/node/communication/mac
castaliaRouting     = $(castaliaSrc)/node/communication/routing
castaliaSimulations = $(castaliaRoot)/Simulations

########## LOCATION DECLARATIONS -- Sandridge ##########
macDir          = mac
macawDir        = macaw              # within mac dir
smacDir         = sMac
bmacDir         = bMac
xmacDir         = xMac
macBufferDir    = buffer
routingDir      = routing
neighboursDir   = neighbours
simulationsDir  = simulations
commonDir       = sandridge_routing
floodingDir     = floodingRouting           # within routing dir
randomDir       = randomRouting

########## SOURCE FILES ##########
########## SOURCE FILES -- MAC ##########
macBufferSrcs = $($(macDir)/$(macBufferDir)/$(wildcard *.cc)) \
$($(macDir)/$(macBufferDir)/$(wildcard *.h))

bmacSrcs = $($(macDir)/$(bmacDir)/$(wildcard *.cc)) \
$($(macDir)/$(bmacDir)/$(wildcard *.h)) \
$($(macDir)/$(bmacDir)/$(wildcard *.msg)) \
$($(macDir)/$(bmacDir)/$(wildcard *.ned))

########## SOURCE FILES -- ROUTING ##########
routingCommonSrcs = $($(routingSrc)/$(wildcard *.cc)) \
$($(routingSrc)/$(wildcard *.h)) \
$($(routingSrc)/neighbours/$(wildcard *.cc)) \
$($(routingSrc)/neighbours/$(wildcard *.h)) \
$($(routingSrc)/sandridge_routing/$(wildcard *.cc)) \
$($(routingSrc)/sandridge_routing/$(wildcard *.h))

floodingSrcs = $($(routingDir)/$(floodingDir)/$(wildcard *.cc)) \
$($(routingDir)/$(floodingDir)/$(wildcard *.h)) \
$($(routingDir)/$(floodingDir)/$(wildcard *.msg)) \
$($(routingDir)/$(floodingDir)/$(wildcard *.ned))

randomSrcs = $($(routingDir)/$(randomDir)/$(wildcard *.cc)) \
$($(routingDir)/$(randomDir)/$(wildcard *.h)) \
$($(routingDir)/$(randomDir)/$(wildcard *.msg)) \
$($(routingDir)/$(randomDir)/$(wildcard *.ned))

########## MAIN BODY OF MAKEFILE ##########
#all: applications simulations mac routing castalia
all: mac routing castalia

.PHONY:

########## SIMULATIONS ##########
simulations: $(simulationsDir)/routingTest/omnetpp.ini
	rsync -r $(simulationsDir)/* $(castaliaSimulations)

########## APPLICATIONS ##########
applications: application_text sandridge_application

application_text:
	@echo "Copying applications"

sandridge_application:
	rsync -r application/sandridge $(castaliaSrc)/node/application
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaSrc)/node/application/sandridge/SandridgeApplication.cc
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaSrc)/node/application/sandridge/SandridgeApplication.h

########## MAC PROTOCOLS ##########
mac: mac_text buffer macaw smac bmac xmac

buffer:
	@rsync -r $(macDir)/macBuffer $(castaliaMac)

mac_text:
	@echo "Updating mac protocols"

mac_dirs:
	mkdir -p $(castaliaMac)

mac_buffer: $(macBufferSrcs)
	rsync -r $(macDir)/$(macBufferDir) $(castaliaMac)

macaw: macaw_main patch_macaw

macaw_main:
	@echo "Copying macaw to simulator directory"
	rsync -r $(macDir)/$(macawDir) $(castaliaMac)

# removes Eclipse CastaliaIncludes.h prior to build
# NB: don't include the newline (this is correct), to preserve line numbering between IDE and final build
patch_macaw:
	@echo "Removing IDE convenience declarations from macaw"
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/macaw/MACAW.h
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/macaw/MACAW.cc

smac: smac_main patch_smac

smac_main:
	rsync -r $(macDir)/$(smacDir) $(castaliaMac)

patch_smac:
	@echo "Removing IDE convenience declarations from smac"
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/sMac/SMAC.h
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/sMac/SMAC.cc

bmac: bmac_main patch_bmac

bmac_main:
	rsync -r $(macDir)/$(bmacDir) $(castaliaMac)

patch_bmac:
	@echo "Removing IDE convenience declarations from bmac"
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/bMac/BMAC.h
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/bMac/BMAC.cc

xmac: xmac_main patch_xmac

xmac_main:
	@echo "Copying xmac to simulator directory"
	rsync -r $(macDir)/$(xmacDir) $(castaliaMac)

patch_xmac:
	@echo "Removing IDE convenience declarations from xmac"
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/xMac/XMAC.h
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaMac)/xMac/XMAC.cc

# copies latest version of header back to ide workspace, for convenience
bmac_header:
	@echo "BMAC HEADER TODO"


########## ROUTING PROTOCOLS ##########
routing: routing_text flooding random
	@echo "... done"

routing_text:
	@echo "Updating routing protocols"

routing_dirs:
	mkdir -p $(castaliaRouting)

routing_common: $(routingCommonSrcs)
	rsync -r $(routingDir)/neighbours $(castaliaRouting)
	rsync -r $(routingDir)/RemoteSequenceNumber* $(castaliaRouting)

routing_header: flooding_header random_header
	@echo "TODO: routing dir"
# TODO want VirtualRouting.h
#	@rsync -r $(castaliaRouting)/*.h $(routingSrc)
#	@rsync -r $(castaliaRouting)/*.cc $(routingSrc)

neighbours:
	rsync -r $(routingDir)/neighbours $(castaliaRouting)

flooding: neighbours
	@echo "... flooding routing"
	rsync -r $(routingDir)/$(floodingDir) $(castaliaRouting)
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaRouting)/floodingRouting/FloodingRouting*

random: neighbours
	@echo "... random routing"
	rsync -r $(routingDir)/$(randomDir) $(castaliaRouting)
	@sed -i "s/\include \"..\/..\/CastaliaIncludes.h\"//g" $(castaliaRouting)/randomRouting/RandomRouting*

########## CASTALIA ##########
castalia:
	@cd $(castaliaRoot); \
	make clean; \
	./makemake; \
	make

# copies virtual classes etc from castalia to workspace
headers: mac_header routing_header
	@echo "Headers copied. Remember to refresh view in eclipse!"

########## CLEANING ##########
clean: clean_castalia clean_sandridge
	@echo "Done cleaning."

clean_castalia: clean_castalia_srcs
	@cd $(castaliaRoot); \
	make clean

clean_castalia_srcs:
	@cd $(castaliaRoot); \
	rm -rf $(castaliaMac)/macaw $(castaliaMac)/smac $($(castaliaMac)/bmac) $($(castaliaMac)/xmac)
	rm -rf $(castaliaRouting)/sandridge_routing $(castaliaRouting)/neighbours $(castaliaMac)/flooding $(castaliaMac)/random

clean_headers:
	@echo "Cleaning headers..."

clean_sandridge:
	@echo "Cleaning sandridge..."
	@/usr/bin/rm -rf *~ *.o
