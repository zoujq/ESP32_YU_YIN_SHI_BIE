
#
# Main component of TF Micro project 'hello_world'.
#

idf_component_register(
    SRCS 
    main.cc 
    main_functions.cc 
    model.cc 
    audio_sample.cc
    feature_provider.cc
    ../components/tfmicro/third_party/kissfft/kiss_fft.c
    ../components/tfmicro/third_party/kissfft/tools/kiss_fftr.c
    INCLUDE_DIRS "")
