

# Struct dxl2\_packet



[**ClassList**](annotated.md) **>** [**dxl2\_packet**](structdxl2__packet.md)



_One decoded Protocol 2.0 packet._ [More...](#detailed-description)

* `#include <dxl2_protocol.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint8\_t | [**id**](#variable-id)  <br>_Packet source/destination ID._  |
|  uint8\_t | [**instruction**](#variable-instruction)  <br>_Instruction byte, 0x55 for Status._  |
|  uint16\_t | [**parameter\_length**](#variable-parameter_length)  <br>_Number of valid bytes in parameters._  |
|  const uint8\_t \* | [**parameters**](#variable-parameters)  <br>_Unstuffed parameter bytes, in the parser buffer._  |












































## Detailed Description


Byte stuffing has been removed from `parameters`. For a Status Packet (instruction 0x55), the status error byte is the first parameter.


The parameters are not copied: the parser removes the byte stuffing in place, and `parameters` points into the buffer of the parser that decoded the packet. They stay valid until the next call of [**dxl2\_parser\_feed()**](group__dxl2__protocol.md#function-dxl2_parser_feed) or [**dxl2\_parser\_init()**](group__dxl2__protocol.md#function-dxl2_parser_init) with that parser. Copy the bytes to keep them longer, not the structure. 


    
## Public Attributes Documentation




### variable id 

_Packet source/destination ID._ 
```C++
uint8_t dxl2_packet::id;
```




<hr>



### variable instruction 

_Instruction byte, 0x55 for Status._ 
```C++
uint8_t dxl2_packet::instruction;
```




<hr>



### variable parameter\_length 

_Number of valid bytes in parameters._ 
```C++
uint16_t dxl2_packet::parameter_length;
```




<hr>



### variable parameters 

_Unstuffed parameter bytes, in the parser buffer._ 
```C++
const uint8_t* dxl2_packet::parameters;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `inc/dxl2_protocol.h`

