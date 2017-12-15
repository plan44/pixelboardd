AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

bin_PROGRAMS = pixelboardd

# pixelboardd

if DEBUG
pixelboardd_DEBUG = -D DEBUG=1
else
pixelboardd_DEBUG =
endif

if P44_BUILD_OW
pixelboardd_PLATFORM = -D P44_BUILD_OW=1
else
pixelboardd_PLATFORM =
endif



pixelboardd_LDADD = $(JSONC_LIBS) $(PTHREAD_LIBS) $(SQLITE3_LIBS) $(PNG_LIBS)


pixelboardd_CXXFLAGS = \
  -I ${srcdir}/src/p44utils \
  -I ${srcdir}/src/p44utils/thirdparty/mongoose \
  -I ${srcdir}/src/p44utils/thirdparty \
  -I ${srcdir}/src \
  -I ${srcdir}/src/Blocks \
  -I ${srcdir}/src/Life \
  -I ${srcdir}/src/Display \
  -I ${srcdir}/src/Torch \
  -D DISABLE_I2C=0 \
  -D DISABLE_SPI=0 \
  ${BOOST_CPPFLAGS} \
  ${JSONC_CFLAGS} \
  ${PTHREAD_CFLAGS} \
  ${SQLITE3_CFLAGS} \
  ${PNG_CFLAGS} \
  ${pixelboardd_PLATFORM} \
  ${pixelboardd_DEBUG}

pixelboardd_SOURCES = \
  src/p44utils/analogio.cpp \
  src/p44utils/analogio.hpp \
  src/p44utils/application.cpp \
  src/p44utils/application.hpp \
  src/p44utils/colorutils.cpp \
  src/p44utils/colorutils.hpp \
  src/p44utils/consolekey.cpp \
  src/p44utils/consolekey.hpp \
  src/p44utils/crc32.cpp \
  src/p44utils/crc32.hpp \
  src/p44utils/digitalio.cpp \
  src/p44utils/digitalio.hpp \
  src/p44utils/error.cpp \
  src/p44utils/error.hpp \
  src/p44utils/fdcomm.cpp \
  src/p44utils/fdcomm.hpp \
  src/p44utils/fnv.cpp \
  src/p44utils/fnv.hpp \
  src/p44utils/gpio.cpp \
  src/p44utils/gpio.h \
  src/p44utils/gpio.hpp \
  src/p44utils/pwm.cpp \
  src/p44utils/pwm.hpp \
  src/p44utils/httpcomm.cpp\
  src/p44utils/httpcomm.hpp\
  src/p44utils/i2c.cpp \
  src/p44utils/i2c.hpp \
  src/p44utils/igmp.cpp \
  src/p44utils/igmp.hpp \
  src/p44utils/iopin.cpp \
  src/p44utils/iopin.hpp \
  src/p44utils/jsoncomm.cpp \
  src/p44utils/jsoncomm.hpp \
  src/p44utils/jsonobject.cpp \
  src/p44utils/jsonobject.hpp \
  src/p44utils/jsonrpccomm.cpp \
  src/p44utils/jsonrpccomm.hpp \
  src/p44utils/jsonwebclient.cpp \
  src/p44utils/jsonwebclient.hpp \
  src/p44utils/ledchaincomm.cpp \
  src/p44utils/ledchaincomm.hpp \
  src/p44utils/logger.cpp \
  src/p44utils/logger.hpp \
  src/p44utils/macaddress.cpp \
  src/p44utils/macaddress.hpp \
  src/p44utils/mainloop.cpp \
  src/p44utils/mainloop.hpp \
  src/p44utils/operationqueue.cpp \
  src/p44utils/operationqueue.hpp \
  src/p44utils/p44obj.cpp \
  src/p44utils/p44obj.hpp \
  src/p44utils/persistentparams.cpp \
  src/p44utils/persistentparams.hpp \
  src/p44utils/serialcomm.cpp \
  src/p44utils/serialcomm.hpp \
  src/p44utils/serialqueue.cpp \
  src/p44utils/serialqueue.hpp \
  src/p44utils/socketcomm.cpp \
  src/p44utils/socketcomm.hpp \
  src/p44utils/spi.cpp \
  src/p44utils/spi.hpp \
  src/p44utils/sqlite3persistence.cpp \
  src/p44utils/sqlite3persistence.hpp \
  src/p44utils/ssdpsearch.cpp \
  src/p44utils/ssdpsearch.hpp \
  src/p44utils/utils.cpp \
  src/p44utils/utils.hpp \
  src/p44utils/thirdparty/sqlite3pp/sqlite3pp.cpp \
  src/p44utils/thirdparty/sqlite3pp/sqlite3pp.h \
  src/p44utils/thirdparty/sqlite3pp/sqlite3ppext.cpp \
  src/p44utils/thirdparty/sqlite3pp/sqlite3ppext.h \
  src/p44utils/thirdparty/mongoose/mongoose.c \
  src/p44utils/thirdparty/mongoose/mongoose.h \
  src/p44utils/p44utils_common.hpp \
  src/Blocks/blocks.cpp \
  src/Blocks/blocks.hpp \
  src/Life/life.cpp \
  src/Life/life.hpp \
  src/Display/display.cpp \
  src/Display/display.hpp \
  src/Torch/torch.cpp \
  src/Torch/torch.hpp \
  src/viewstack.cpp \
  src/viewstack.hpp \
  src/viewanimator.cpp \
  src/viewanimator.hpp \
  src/view.cpp \
  src/view.hpp \
  src/textview.cpp \
  src/textview.hpp \
  src/imageview.cpp \
  src/imageview.hpp \
  src/pixelpage.cpp \
  src/pixelpage.hpp \
  src/pixelboardd_main.cpp
