

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
|  uint8\_t | [**parameters**](#variable-parameters)  <br>_Unstuffed parameter bytes._  |












































## Detailed Description


Byte stuffing has been removed from `parameters`. For a Status Packet (instruction 0x55), the status error byte is the first parameter. 


    
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

_Unstuffed parameter bytes._ 
```C++
uint8_t dxl2_packet::parameters[DXL2_MAX_PARAMETER_SIZE];
```




<hr>

------------------------------
The documentation for this class was generated from the following file `inc/dxl2_protocol.h`

