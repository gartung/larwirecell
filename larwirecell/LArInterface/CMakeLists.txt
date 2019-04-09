# larwirecell/LArInterface/CMakeLists.txt
#
# David Adams
# June 2016
#
# CMake instructions for larsoft service interface classes.

art_make(
          LIB_LIBRARIES
                        ${ART_FRAMEWORK_CORE}
                        art_Utilities
                        canvas
                        ${MF_MESSAGELOGGER}
                        cetlib cetlib_except
                        ${Boost_SYSTEM_LIBRARY}
                        ${ROOT_LIBRARIES}
          MODULE_LIBRARIES
                        lardata_Utilities
                        lardataobj_RawData
                        larcorealg_Geometry
                        larevt_CalibrationDBI_IOVData
                        larevt_CalibrationDBI_Providers
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_ROOT_IO_TFILE_SUPPORT}
                        ${ART_ROOT_IO_TFILESERVICE_SERVICE}
                        art_Persistency_Common
                        art_Persistency_Provenance
                        art_Utilities
                        canvas
                        ${MF_MESSAGELOGGER}
                        ${FHICLCPP}
                        cetlib cetlib_except
                        ${ROOT_Core_LIBRARY}
                        ${ROOT_Geom_Library}
                        ${ROOT_XMLIO_LIBRARY}
                        ${ROOT_Gdml_LIBRARY}
                        ${ROOT_LIBRARIES}
                        ${WIRECELL_LIBS}
)

install_headers()
install_fhicl()
install_source()
