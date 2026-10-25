

# Group dxl2\_link



[**Modules**](modules.md) **>** [**dxl2\_link**](group__dxl2__link.md)



_Transport-independent link that owns a parser and dispatches packets._ [More...](#detailed-description)
















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**dxl2\_link**](structdxl2__link.md) <br>_Link state._  |
| struct | [**dxl2\_link\_config**](structdxl2__link__config.md) <br>_Link configuration passed to_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._ |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef void(\* | [**dxl2\_packet\_handler\_t**](#typedef-dxl2_packet_handler_t)  <br>_Handle one valid packet received by the link._  |
| typedef int(\* | [**dxl2\_write\_fn**](#typedef-dxl2_write_fn)  <br>_Write a complete wire packet to the transport._  |




















## Public Functions

| Type | Name |
| ---: | :--- |
|  int | [**dxl2\_link\_init**](#function-dxl2_link_init) (struct [**dxl2\_link**](structdxl2__link.md) \* link, const struct [**dxl2\_link\_config**](structdxl2__link__config.md) \* config) <br>_Initialize a link and reset its parser and counters._  |
|  uint32\_t | [**dxl2\_link\_invalid\_packets**](#function-dxl2_link_invalid_packets) (const struct [**dxl2\_link**](structdxl2__link.md) \* link) <br>_Number of malformed or CRC-invalid packets rejected since_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._ |
|  void | [**dxl2\_link\_receive**](#function-dxl2_link_receive) (struct [**dxl2\_link**](structdxl2__link.md) \* link, const uint8\_t \* data, size\_t length) <br>_Feed received bytes; calls the handler for each complete valid packet._  |
|  uint32\_t | [**dxl2\_link\_rx\_packets**](#function-dxl2_link_rx_packets) (const struct [**dxl2\_link**](structdxl2__link.md) \* link) <br>_Number of valid packets received since_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._ |
|  int | [**dxl2\_link\_send**](#function-dxl2_link_send) (struct [**dxl2\_link**](structdxl2__link.md) \* link, uint8\_t id, uint8\_t instruction, const uint8\_t \* parameters, uint16\_t parameter\_length) <br>_Encode one packet (stuffing and CRC added) and write it._  |
|  int | [**dxl2\_link\_send\_status**](#function-dxl2_link_send_status) (struct [**dxl2\_link**](structdxl2__link.md) \* link, uint8\_t id, uint8\_t error, const uint8\_t \* data, uint16\_t data\_length) <br>_Send a Status Packet: error byte followed by_ `data` _._ |




























## Detailed Description


The link owns a parser, dispatches every valid packet to a handler, and encodes outgoing packets into a caller-supplied write function. It has no RTOS or driver dependency: the integrator moves received bytes from the UART/USB/socket into [**dxl2\_link\_receive**](group__dxl2__link.md#function-dxl2_link_receive) and provides the byte writer.


A link is not thread-safe. Call [**dxl2\_link\_receive**](group__dxl2__link.md#function-dxl2_link_receive) from one context only (for example an RX thread, not an interrupt handler), and serialize [**dxl2\_link\_send**](group__dxl2__link.md#function-dxl2_link_send) calls from several contexts with a mutex.




**
**


* Lifecycle: [**dxl2\_link\_init**](group__dxl2__link.md#function-dxl2_link_init). 
* Message flow: [**dxl2\_link\_receive**](group__dxl2__link.md#function-dxl2_link_receive), [**dxl2\_link\_send**](group__dxl2__link.md#function-dxl2_link_send), [**dxl2\_link\_send\_status**](group__dxl2__link.md#function-dxl2_link_send_status). 
* Diagnostics: [**dxl2\_link\_rx\_packets**](group__dxl2__link.md#function-dxl2_link_rx_packets), [**dxl2\_link\_invalid\_packets**](group__dxl2__link.md#function-dxl2_link_invalid_packets). 







    
## Public Types Documentation




### typedef dxl2\_packet\_handler\_t 

_Handle one valid packet received by the link._ 
```
typedef void(* dxl2_packet_handler_t) (const struct dxl2_packet *packet, void *context);
```



The packet pointer is valid only for the duration of the callback. Copy any data that must be retained after the callback returns. The handler may call [**dxl2\_link\_send**](group__dxl2__link.md#function-dxl2_link_send) on the same link to answer.




**Parameters:**


* `packet` Decoded packet with byte stuffing removed. 
* `context` Value of [**dxl2\_link\_config::handler\_context**](structdxl2__link__config.md#variable-handler_context). 




        

<hr>



### typedef dxl2\_write\_fn 

_Write a complete wire packet to the transport._ 
```
typedef int(* dxl2_write_fn) (void *context, const uint8_t *data, size_t length);
```





**Parameters:**


* `context` Value of [**dxl2\_link\_config::write\_context**](structdxl2__link__config.md#variable-write_context). 
* `data` Encoded packet, including header and CRC. 
* `length` Number of bytes in `data`. 



**Returns:**

0 on success, a negative value on failure (returned by the send call). 





        

<hr>
## Public Functions Documentation




### function dxl2\_link\_init 

_Initialize a link and reset its parser and counters._ 
```
int dxl2_link_init (
    struct dxl2_link * link,
    const struct dxl2_link_config * config
) 
```





**Parameters:**


* `link` Link state to initialize. 
* `config` Configuration copied into the link. 



**Return value:**


* `0` Link ready. 
* `DXL2_PARSE_ERROR` `link` or `config` is NULL. 




        

<hr>



### function dxl2\_link\_invalid\_packets 

_Number of malformed or CRC-invalid packets rejected since_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._
```
uint32_t dxl2_link_invalid_packets (
    const struct dxl2_link * link
) 
```




<hr>



### function dxl2\_link\_receive 

_Feed received bytes; calls the handler for each complete valid packet._ 
```
void dxl2_link_receive (
    struct dxl2_link * link,
    const uint8_t * data,
    size_t length
) 
```



Bytes may arrive in any chunking. Malformed or CRC-invalid packets are dropped and counted, and the parser resynchronizes on the next header.




**Parameters:**


* `link` Link state. 
* `data` Received bytes; may be NULL only when `length` is zero. 
* `length` Number of bytes in `data`. 




        

<hr>



### function dxl2\_link\_rx\_packets 

_Number of valid packets received since_ [_**dxl2\_link\_init**_](group__dxl2__link.md#function-dxl2_link_init) _._
```
uint32_t dxl2_link_rx_packets (
    const struct dxl2_link * link
) 
```




<hr>



### function dxl2\_link\_send 

_Encode one packet (stuffing and CRC added) and write it._ 
```
int dxl2_link_send (
    struct dxl2_link * link,
    uint8_t id,
    uint8_t instruction,
    const uint8_t * parameters,
    uint16_t parameter_length
) 
```





**Parameters:**


* `link` Link state. 
* `id` Destination ID. 
* `instruction` Instruction byte. 
* `parameters` Unstuffed parameters, or NULL when `parameter_length` is zero. 
* `parameter_length` Number of parameter bytes. 



**Return value:**


* `0` Packet written. 
* `DXL2_PARSE_ERROR` Invalid arguments, packet too large, or no writer. 
* `Other` negative value Error returned by the write function. 




        

<hr>



### function dxl2\_link\_send\_status 

_Send a Status Packet: error byte followed by_ `data` _._
```
int dxl2_link_send_status (
    struct dxl2_link * link,
    uint8_t id,
    uint8_t error,
    const uint8_t * data,
    uint16_t data_length
) 
```





**Parameters:**


* `link` Link state. 
* `id` ID of the answering device. 
* `error` Status error byte (DXL2\_ERR\_\*, optionally with DXL2\_ERR\_ALERT\_BIT). 
* `data` Status data, or NULL when `data_length` is zero. 
* `data_length` Number of data bytes (at most DXL2\_MAX\_PARAMETER\_SIZE - 1). 



**Returns:**

Same as [**dxl2\_link\_send**](group__dxl2__link.md#function-dxl2_link_send). 





        

<hr>

------------------------------


