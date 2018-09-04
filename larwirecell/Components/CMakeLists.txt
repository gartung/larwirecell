# Build the libWireCellLarsoft WCT plugin library.
# See README for some details.

file(GLOB wcls_components_sources "*.cxx")

# This is what's needed for building he WireCellLarsoft WCT component
# plugin library.  
art_make_library(
  LIBRARY_NAME WireCellLarsoft
  SOURCE ${wcls_components_sources}
  LIBRARIES
        art_Framework_Core art_Framework_Principal art_Framework_Services_Registry
        art_Persistency_Common art_Persistency_Provenance
        art_Utilities
        canvas cetlib cetlib_except
        ${Boost_SYSTEM_LIBRARY}
        ${ROOT_BASIC_LIB_LIST}
        ${JSONCPP}
	lardata_Utilities
	lardata_DetectorInfoServices_DetectorClocksServiceStandard_service
	lardataobj_RawData
	lardataobj_RecoBase
	lardataobj_Simulation
        larcorealg_Geometry
	larevt_CalibrationDBI_IOVData
	larevt_CalibrationDBI_Providers
        ${WIRECELL_LIBS}
)
