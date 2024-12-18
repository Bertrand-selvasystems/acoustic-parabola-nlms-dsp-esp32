# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Bertrand/esp/v5.3.1/esp-idf/components/bootloader/subproject"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/tmp"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/src/bootloader-stamp"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/src"
  "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "H:/PROJETS/PARABOLE 2 MICROS I2S/PROGRAMME/ESSAI4 - Copy/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
