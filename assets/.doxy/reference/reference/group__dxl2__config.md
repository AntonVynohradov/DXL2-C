

# Group dxl2\_config



[**Modules**](modules.md) **>** [**dxl2\_config**](group__dxl2__config.md)



_Build-time configuration for DXL2._ [More...](#detailed-description)

































































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**DXL2\_MAX\_PACKET\_SIZE**](group__dxl2__config.md#define-dxl2_max_packet_size)  `256U`<br>_Maximum complete wire-packet size, including header and CRC, in bytes._  |

## Detailed Description


Every parameter can be overridden by the integrator with a compiler definition (for example `-DDXL2_MAX_PACKET_SIZE=1024U`) without editing this file.




**
**


* [**DXL2\_MAX\_PACKET\_SIZE**](group__dxl2__config.md#define-dxl2_max_packet_size) : largest wire packet the parser and encoder accept. 







    
## Macro Definition Documentation





### define DXL2\_MAX\_PACKET\_SIZE 

_Maximum complete wire-packet size, including header and CRC, in bytes._ 
```
#define DXL2_MAX_PACKET_SIZE `256U`
```



Sizes the parser buffer, the decoded packet and the link transmit buffer, so it directly drives the RAM footprint of one link. 


        

<hr>

------------------------------


