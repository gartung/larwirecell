
# This is what's needed for building he WireCellLarsoft WCT component
# plugin library.  
art_make(
  MODULE_LIBRARIES
        art_Framework_Core art_Framework_Principal art_Framework_Services_Registry
        art_Persistency_Common art_Persistency_Provenance
        art_Utilities
        canvas cetlib cetlib_except
        ${Boost_SYSTEM_LIBRARY}
        ${ROOT_LIBRARIES}
        jsoncpp
	lardata_Utilities
	lardataobj_RawData
	lardataobj_RecoBase
        ${WIRECELL_LIBS}
)


install_headers()
install_source()
