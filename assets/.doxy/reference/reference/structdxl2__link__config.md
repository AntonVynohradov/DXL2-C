

# Struct dxl2\_link\_config



[**ClassList**](annotated.md) **>** [**dxl2\_link\_config**](structdxl2__link__config.md)



_Link configuration passed to_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._

* `#include <dxl2_link.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**dxl2\_packet\_handler\_t**](group__dxl2__link.md#typedef-dxl2_packet_handler_t) | [**handler**](#variable-handler)  <br>_Packet handler; NULL discards (still counted)._  |
|  void \* | [**handler\_context**](#variable-handler_context)  <br>_Context for handler._  |
|  [**dxl2\_write\_fn**](group__dxl2__link.md#typedef-dxl2_write_fn) | [**write**](#variable-write)  <br>_Byte writer; NULL makes the link receive-only._  |
|  void \* | [**write\_context**](#variable-write_context)  <br>_Context for write._  |












































## Public Attributes Documentation




### variable handler 

_Packet handler; NULL discards (still counted)._ 
```C++
dxl2_packet_handler_t dxl2_link_config::handler;
```




<hr>



### variable handler\_context 

_Context for handler._ 
```C++
void* dxl2_link_config::handler_context;
```




<hr>



### variable write 

_Byte writer; NULL makes the link receive-only._ 
```C++
dxl2_write_fn dxl2_link_config::write;
```




<hr>



### variable write\_context 

_Context for write._ 
```C++
void* dxl2_link_config::write_context;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `inc/dxl2_link.h`

