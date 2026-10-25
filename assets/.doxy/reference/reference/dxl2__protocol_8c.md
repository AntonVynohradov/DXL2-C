

# File dxl2\_protocol.c



[**FileList**](files.md) **>** [**src**](dir_68267d1309a1af8e8297ef4c3efbcdba.md) **>** [**dxl2\_protocol.c**](dxl2__protocol_8c.md)

[Go to the source code of this file](dxl2__protocol_8c_source.md)



* `#include <stdbool.h>`
* `#include <string.h>`
* `#include "dxl2_defs.h"`
* `#include "dxl2_protocol.h"`























## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  const uint8\_t | [**dxl2\_header**](#variable-dxl2_header)   = `{0xFF, 0xFF, 0xFD, 0x00}`<br>_DYNAMIXEL Protocol 2.0 packet header._  |
















## Public Static Functions

| Type | Name |
| ---: | :--- |
|  int | [**dxl2\_decode\_packet**](#function-dxl2_decode_packet) (const struct [**dxl2\_parser**](structdxl2__parser.md) \* parser, struct [**dxl2\_packet**](structdxl2__packet.md) \* packet, size\_t total\_length) <br>_Validate CRC and decode a complete wire packet._  |
|  void | [**dxl2\_parser\_reset**](#function-dxl2_parser_reset) (struct [**dxl2\_parser**](structdxl2__parser.md) \* parser) <br>_Discard the parser's current partial packet._  |

























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**DXL2\_BODY\_OFFSET**](dxl2__protocol_8c.md#define-dxl2_body_offset)  `7U`<br> |
| define  | [**DXL2\_CRC\_SIZE**](dxl2__protocol_8c.md#define-dxl2_crc_size)  `2U`<br> |
| define  | [**DXL2\_HEADER\_SIZE**](dxl2__protocol_8c.md#define-dxl2_header_size)  `4U`<br> |
| define  | [**DXL2\_LENGTH\_OFFSET**](dxl2__protocol_8c.md#define-dxl2_length_offset)  `5U`<br> |
| define  | [**DXL2\_MIN\_LENGTH**](dxl2__protocol_8c.md#define-dxl2_min_length)  `3U`<br> |

## Public Static Attributes Documentation




### variable dxl2\_header 

_DYNAMIXEL Protocol 2.0 packet header._ 
```C++
const uint8_t dxl2_header[DXL2_HEADER_SIZE];
```




<hr>
## Public Static Functions Documentation




### function dxl2\_decode\_packet 

_Validate CRC and decode a complete wire packet._ 
```C++
static int dxl2_decode_packet (
    const struct dxl2_parser * parser,
    struct dxl2_packet * packet,
    size_t total_length
) 
```





**Parameters:**


* `parser` - Parser containing the complete wire packet 
* `packet` - Receives the decoded packet on success 
* `total_length` - Complete wire-packet length, including header and CRC 



**Returns:**

1 on success, DXL2\_PARSE\_ERROR on CRC mismatch, invalid stuffing or payload 





        

<hr>



### function dxl2\_parser\_reset 

_Discard the parser's current partial packet._ 
```C++
static void dxl2_parser_reset (
    struct dxl2_parser * parser
) 
```





**Parameters:**


* `parser` - Parser state to reset 




        

<hr>
## Macro Definition Documentation





### define DXL2\_BODY\_OFFSET 

```C++
#define DXL2_BODY_OFFSET `7U`
```




<hr>



### define DXL2\_CRC\_SIZE 

```C++
#define DXL2_CRC_SIZE `2U`
```




<hr>



### define DXL2\_HEADER\_SIZE 

```C++
#define DXL2_HEADER_SIZE `4U`
```




<hr>



### define DXL2\_LENGTH\_OFFSET 

```C++
#define DXL2_LENGTH_OFFSET `5U`
```




<hr>



### define DXL2\_MIN\_LENGTH 

```C++
#define DXL2_MIN_LENGTH `3U`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `src/dxl2_protocol.c`

