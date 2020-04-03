# Build the libWireCellLarsoft WCT plugin library.
# See README for some details.

file(GLOB wcls_components_sources "*.cxx")

# This is what's needed for building he WireCellLarsoft WCT component
# plugin library.
art_make_library(
  LIBRARY_NAME WireCellLarsoft
  SOURCE ${wcls_components_sources}
  LIBRARIES
    ${ART_FRAMEWORK_CORE}
    ${ART_FRAMEWORK_PRINCIPAL}
    ${ART_FRAMEWORK_SERVICES_REGISTRY}
    ${ART_PERSISTENCY_PROVENANCE}
    ${ART_UTILITIES}
    ${JSONCPP}
    ROOT::Core
    ${WIRECELL_LIBS}
    canvas
    cetlib_except
    larcorealg_Geometry
    lardataobj_RawData
    lardataobj_RecoBase
    lardataobj_Simulation
    larevt_CalibrationDBI_IOVData
)
