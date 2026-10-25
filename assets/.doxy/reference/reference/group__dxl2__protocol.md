

# Group dxl2\_protocol



[**Modules**](modules.md) **>** [**dxl2\_protocol**](group__dxl2__protocol.md)



_CRC-16, incremental stream parser and packet encoder with byte stuffing._ [More...](#detailed-description)
















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**dxl2\_packet**](structdxl2__packet.md) <br>_One decoded Protocol 2.0 packet._  |
| struct | [**dxl2\_parser**](structdxl2__parser.md) <br>_Incremental parser state for a byte stream._  |






















## Public Functions

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**dxl2\_crc16**](#function-dxl2_crc16) (const uint8\_t \* data, size\_t length) <br>_Calculate the Protocol 2.0 CRC-16 checksum._  |
|  int | [**dxl2\_encode**](#function-dxl2_encode) (uint8\_t id, uint8\_t instruction, const uint8\_t \* parameters, uint16\_t parameter\_length, uint8\_t \* output, size\_t output\_capacity, size\_t \* output\_length) <br>_Encode a Protocol 2.0 packet, adding byte stuffing and CRC._  |
|  int | [**dxl2\_parser\_feed**](#function-dxl2_parser_feed) (struct [**dxl2\_parser**](structdxl2__parser.md) \* parser, uint8\_t byte, struct [**dxl2\_packet**](structdxl2__packet.md) \* packet) <br>_Feed one byte to the incremental packet parser._  |
|  void | [**dxl2\_parser\_init**](#function-dxl2_parser_init) (struct [**dxl2\_parser**](structdxl2__parser.md) \* parser) <br>_Reset a parser before starting or restarting a byte stream._  |



























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**DXL2\_MAX\_PARAMETER\_SIZE**](group__dxl2__protocol.md#define-dxl2_max_parameter_size)  `([**DXL2\_MAX\_PACKET\_SIZE**](group__dxl2__config.md#define-dxl2_max_packet_size) - 10U)`<br>_Maximum unstuffed parameter payload size supported by this implementation._  |
| define  | [**DXL2\_PARSE\_ERROR**](group__dxl2__protocol.md#define-dxl2_parse_error)  `(-1)`<br>_Error returned for invalid input, malformed packets, or insufficient capacity._  |

## Detailed Description




**
**


* CRC: [**dxl2\_crc16**](group__dxl2__protocol.md#function-dxl2_crc16). 
* Parser: [**dxl2\_parser\_init**](group__dxl2__protocol.md#function-dxl2_parser_init), [**dxl2\_parser\_feed**](group__dxl2__protocol.md#function-dxl2_parser_feed). 
* Encoder: [**dxl2\_encode**](group__dxl2__protocol.md#function-dxl2_encode). 







    
## Public Functions Documentation




### function dxl2\_crc16 

_Calculate the Protocol 2.0 CRC-16 checksum._ 
```
uint16_t dxl2_crc16 (
    const uint8_t * data,
    size_t length
) 
```



Uses polynomial 0x8005 and an initial CRC value of zero.




**Parameters:**


* `data` Bytes covered by the CRC. 
* `length` Number of bytes to process. 



**Returns:**

Calculated 16-bit CRC; 0 (the initial value) when `data` is NULL. 





        

<hr>



### function dxl2\_encode 

_Encode a Protocol 2.0 packet, adding byte stuffing and CRC._ 
```
int dxl2_encode (
    uint8_t id,
    uint8_t instruction,
    const uint8_t * parameters,
    uint16_t parameter_length,
    uint8_t * output,
    size_t output_capacity,
    size_t * output_length
) 
```





**Parameters:**


* `id` Packet ID. 
* `instruction` Instruction byte. 
* `parameters` Unstuffed parameter bytes, or NULL when the length is zero. 
* `parameter_length` Number of parameter bytes. 
* `output` Destination buffer for the complete wire packet. 
* `output_capacity` Capacity of `output` in bytes. 
* `output_length` Receives the encoded packet length on success. 



**Return value:**


* `0` Packet encoded successfully. 
* `DXL2_PARSE_ERROR` Invalid input or insufficient output capacity. 




        

<hr>



### function dxl2\_parser\_feed 

_Feed one byte to the incremental packet parser._ 
```
int dxl2_parser_feed (
    struct dxl2_parser * parser,
    uint8_t byte,
    struct dxl2_packet * packet
) 
```



The parser searches for the Protocol 2.0 header, waits for the declared packet length, validates CRC, then removes byte stuffing. Continue feeding bytes after a parse error so the parser can find the next packet header.




**Parameters:**


* `parser` Parser state. 
* `byte` Next byte from the input stream. 
* `packet` Output packet; its contents are valid only when the call returns 1. 



**Return value:**


* `1` A complete valid packet is available in `packet`. 
* `0` More bytes are required. 
* `DXL2_PARSE_ERROR` Invalid arguments or a malformed/CRC-invalid packet. 




        

<hr>



### function dxl2\_parser\_init 

_Reset a parser before starting or restarting a byte stream._ 
```
void dxl2_parser_init (
    struct dxl2_parser * parser
) 
```





**Parameters:**


* `parser` Parser state to initialize; NULL is ignored. 




        

<hr>
## Macro Definition Documentation





### define DXL2\_MAX\_PARAMETER\_SIZE 

_Maximum unstuffed parameter payload size supported by this implementation._ 
```
#define DXL2_MAX_PARAMETER_SIZE `( DXL2_MAX_PACKET_SIZE - 10U)`
```




<hr>



### define DXL2\_PARSE\_ERROR 

_Error returned for invalid input, malformed packets, or insufficient capacity._ 
```
#define DXL2_PARSE_ERROR `(-1)`
```




<hr>

------------------------------


