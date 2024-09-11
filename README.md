# HASPPP
HASPPP (Galileo High Accuracy Service Precise Point Positioning): an open-source Galileo HAS embeddable RTKLIB decoding package

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

CONTRIBUTOR

   Core developers:

   * Runzhi Zhang (zhangrunzhi@ntsc.ac.cn)
   * Rui Tu ( turui-2004@126.com)
   * Xiaochun Lu (luxc@ntsc.ac.cn)

CONTACT
   QQ Groups: 651080705 (HASPPP)
   Emails: zhangrunzhi@ntsc.ac.cn; turui-2004@126.com; luxc@ntsc.ac.cn
