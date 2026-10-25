

# Struct dxl2\_link



[**ClassList**](annotated.md) **>** [**dxl2\_link**](structdxl2__link.md)



_Link state._ [More...](#detailed-description)

* `#include <dxl2_link.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  struct [**dxl2\_link\_config**](structdxl2__link__config.md) | [**config**](#variable-config)  <br>_Copy of the configuration._  |
|  uint32\_t | [**invalid\_packets**](#variable-invalid_packets)  <br>_Malformed or CRC-invalid packets rejected._  |
|  struct [**dxl2\_packet**](structdxl2__packet.md) | [**packet**](#variable-packet)  <br>_Last decoded packet._  |
|  struct [**dxl2\_parser**](structdxl2__parser.md) | [**parser**](#variable-parser)  <br>_Receive stream parser._  |
|  uint32\_t | [**rx\_packets**](#variable-rx_packets)  <br>_Valid packets received._  |












































## Detailed Description


Treat the members as private; use the functions below. 


    
## Public Attributes Documentation




### variable config 

_Copy of the configuration._ 
```C++
struct dxl2_link_config dxl2_link::config;
```




<hr>



### variable invalid\_packets 

_Malformed or CRC-invalid packets rejected._ 
```C++
uint32_t dxl2_link::invalid_packets;
```




<hr>



### variable packet 

_Last decoded packet._ 
```C++
struct dxl2_packet dxl2_link::packet;
```




<hr>



### variable parser 

_Receive stream parser._ 
```C++
struct dxl2_parser dxl2_link::parser;
```




<hr>



### variable rx\_packets 

_Valid packets received._ 
```C++
uint32_t dxl2_link::rx_packets;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `inc/dxl2_link.h`

