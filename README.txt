------------------------------------------------------------------------------------------------------------------------------------------------------------

                                            HASPPP: an open-source Galileo HAS embeddable RTKLIB decoding package

------------------------------------------------------------------------------------------------------------------------------------------------------------

OVERVIEW

HAS (high accuracy service) is an official precise point positioning (PPP) service provided by Galileo satellites to users worldwide. 
The HASPPP is an open-source C/C++ decoding package for HAS that can be seamlessly embedded into C/C++ GNSS 
(Global Navigation Satellite System) software like RTKLIB. Its main purpose is to simplify the intricate decoding process of Galileo HAS, 
enabling researchers to conveniently and rapidly apply HAS corrections directly. It offers a mode to embed 
into C/C++ GNSS software like an RTCM (Radio Technical Commission for Maritime Services) 
decoder, facilitating portability across different environments via cross-compilers. The HASPPP includes a decoding interface 
for Galileo HAS encoded pages, allowing support for HAS binary data output any manufacturer and enabling real-time HAS decoding.

In addition, the HASPPP manual provides detailed decoding instructions and code analysis, along with a range of HAS decoding examples
ranging from simple to complex cases. HASPPP provides three case studies, including two learning projects for HAS decoding 
(decoding official data and decoding real-time cached HAS data) and one project for embedding the HASPPP into PPP.
The three case studies are stored in separate folders named "office_example", "decode_example", and "ppp_example", respectively.
-------------------------------------------------------------------------------------------------------------------------------------------------------------

DIRECTORY STRUCTURE OF PACKAGE

HASPPP
├─ Src                                               HASPPP source code and dependency files
│   └── resource                              HASPPP dependency files
├─ manual                                        HASPPP software manual
├─ Matlab                                         MATLAB scripts to generate HASPPP dependency files
├─ office_example                            Learning case of decoding official data with HASPPP
│   ├── app                                      Qt project 
│   │     ├── debug                          Debug directory
│   │     ├── debugFiles                   The log folder for HASPPP, which must be located in the same directory as the debug folder containing the executable program.
│   │     └── resource                       The dependency folder for HASPPP, which must be located in the same directory as the debug folder containing the executable program.
│   ├── bin
│   │     ├── windows                       The executable program folder for Windows users
│   │     ├── linux                              The executable program folder for Linux users
│   │     ├── debugFiles                    The log folder for HASPPP, which must be located in the same directory as the folder (windows, linux) containing the executable program.
│   │     └── resource                        The dependency folder for HASPPP, which must be located in the same directory as the folder (windows, linux) containing the executable program.
│   └── data                                       Galileo HAS official HAS encoded page data
├─ decode_example                          Learning case of decoding cashed real-time HAS data with HASPPP
│   ├── app                                        Qt project 
│   ├── bin                                         The executable program folder 
│   ├── data                                        Real-time HAS data cached by receivers
│   │     ├── sino                                 Raw binary HAS data (*.HAS) cashed by the SINO K803 receiver
│   │     ├── novatel                            Raw binary HAS data (*.HAS) cashed by the NovAtel OEM729 receiver
│   │     └── septentrio                       Raw binary HAS data (*.HAS) cashed by the Septentrio AsteRx-m3 Pro+ receiver 
└─ ppp_example                                 Project for embedding the HASPPP into RTKLIB
     ├── app                                         Qt project 
     │     └── config                              Interface configuration directory
     │             ├──windows_config       The interface configuration software for Windows users
     │             └──linux_config              The interface configuration software for Linux users
     │     ├── debug                              Debug directory
     │     ├── debugFiles                      The log folder for HASPPP, which must be located in the same directory as the debug folder containing the executable program.
     │     └── resource                          The dependency folder for HASPPP, which must be located in the same directory as the debug folder containing the executable program.
     ├── bin                                          The executable program folder 
     │     └── config                              Interface configuration directory
     │             ├──windows_config       The interface configuration software for Windows users
     │             └──linux_config              The interface configuration software for Linux users
     │     ├── windows                          The executable program folder for Windows users
     │     ├── linux                                 The executable program folder for Linux users
     │     ├── debugFiles                      The log folder for HASPPP, which must be located in the same directory as the folder (windows, linux) containing the executable program.
     │     ├── resource                          The dependency folder for HASPPP, which must be located in the same directory as the folder (windows, linux) containing the executable program.
     │     └── config                              Interface configuration directory
     │             ├──windows_config       The interface configuration software for Windows users
     │             └──linux_config              The interface configuration software for Linux users
     └── data                                         Real-time HAS data cached by receivers
             ├── sino                                  Raw binary HAS data (*.HAS) cashed by the SINO K803 receiver, observations, and ephemeris
             ├── novatel                             Raw binary HAS data (*.HAS) cashed by the NovAtel OEM729 receiver, observations, and ephemeris
             └── septentrio                        Raw binary HAS data (*.HAS) cashed by the Septentrio AsteRx-m3 Pro+ receiver, observations, and ephemeris




--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
NOTES: Please take note of the locations of folders debug, debugFiles, resource, and config. 
Don't forget to copy the configuration file (.ini) to the config folder after finishing the interface setup software. 
For instance, on Windows, the configuration files from the windows_config folder need to be copied to the config folder.
A detailed explanation is provided in the manual.


The additional dataset and results of HAS time comparision are accessible through https://github.com/ZhangRunzhi20/High-Accuracy-Service-public-dataset.
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

CONTRIBUTOR

   Core developers:

   * Runzhi Zhang (zhangrunzhi@ntsc.ac.cn)
   * Rui Tu ( turui-2004@126.com)
   * Xiaochun Lu (luxc@ntsc.ac.cn)
   

--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


