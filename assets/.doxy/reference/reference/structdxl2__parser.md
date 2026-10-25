

# Struct dxl2\_parser



[**ClassList**](annotated.md) **>** [**dxl2\_parser**](structdxl2__parser.md)



_Incremental parser state for a byte stream._ [More...](#detailed-description)

* `#include <dxl2_protocol.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**buffer**](#variable-buffer)  <br>_In-progress wire packet, header to CRC._  |
|  size\_t | [**length**](#variable-length)  <br>_Number of bytes currently in buffer._  |












































## Detailed Description


Keep one instance per input stream and initialize it before feeding bytes. 


    
## Public Attributes Documentation




### variable buffer 

_In-progress wire packet, header to CRC._ 
```C++
uint8_t dxl2_parser::buffer[DXL2_MAX_PACKET_SIZE];
```




<hr>



### variable length 

_Number of bytes currently in buffer._ 
```C++
size_t dxl2_parser::length;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `inc/dxl2_protocol.h`

