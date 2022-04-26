
ifeq ($(FHEW),1)
OPTS += -I./$(FHEWD0)/inc/fhew
OPTS += -I./$(FHEWD0)/inc/fftwa
OPTS += -I./$(FHEWD0)/inc/fftw3
endif

ifeq ($(HELI),1)
OPTS += -I./$(HELID0)/src
endif

ifneq ($(MPIR),0)
OPTS += -I./$(MPIRD0)
endif

ifeq ($(SEAL),1)
OPTS += -I./$(SEALD0)/include
endif

ifeq ($(TFHE),1)
OPTS += -I./$(TFHED0)/inc/tfhe
OPTS += -I./$(TFHED0)/inc/fftwa
OPTS += -I./$(TFHED0)/inc/fftw3
endif

E3NOABORTMAK =
ifdef E3NOABORT
ifeq ($(E3NOABORT),1)
E3NOABORTMAK = -DE3NOABORT=1
endif
endif
