# larwirecell/LArInterface/CMakeLists.txt
#
# David Adams
# June 2016
#
# CMake instructions for larsoft service interface classes.

art_make(
  MODULE_LIBRARIES
    lardataobj_RawData
    larevt_CalibrationDBI_IOVData
    larcorealg_Geometry
    ${ART_FRAMEWORK_SERVICES_REGISTRY}
    ${ART_ROOT_IO_TFILESERVICE_SERVICE}
    ${WIRECELL_LIBS}
    ROOT::Core
)

install_headers()
install_fhicl()
install_source()
