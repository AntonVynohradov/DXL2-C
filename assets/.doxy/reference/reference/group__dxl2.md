

# Group dxl2



[**Modules**](modules.md) **>** [**dxl2**](group__dxl2.md)



_Public API of the DYNAMIXEL Protocol 2.0 library._ [More...](#detailed-description)












## Modules

| Type | Name |
| ---: | :--- |
| module | [**Configuration**](group__dxl2__config.md) <br>_Build-time configuration for DXL2._  |
| module | [**Definitions**](group__dxl2__defs.md) <br>_Protocol 2.0 IDs, instructions, error codes and byte-order helpers._  |
| module | [**Link**](group__dxl2__link.md) <br>_Transport-independent link that owns a parser and dispatches packets._  |
| module | [**Codec**](group__dxl2__protocol.md) <br>_CRC-16, incremental stream parser and packet encoder with byte stuffing._  |





















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**DXL2\_VERSION**](group__dxl2.md#define-dxl2_version)  `"1.0.0"`<br>_Version string "MAJOR.MINOR.PATCH"._  |
| define  | [**DXL2\_VERSION\_MAJOR**](group__dxl2.md#define-dxl2_version_major)  `1`<br> |
| define  | [**DXL2\_VERSION\_MINOR**](group__dxl2.md#define-dxl2_version_minor)  `0`<br> |
| define  | [**DXL2\_VERSION\_PATCH**](group__dxl2.md#define-dxl2_version_patch)  `0`<br> |

## Detailed Description


This module groups the public interface and its supporting submodules: [**Configuration**](group__dxl2__config.md), [**Definitions**](group__dxl2__defs.md), [**Codec**](group__dxl2__protocol.md) and [**Link**](group__dxl2__link.md).


Include this single header to use the library: 
* [**dxl2\_config.h**](dxl2__config_8h.md) build-time configuration (buffer sizes) 
* [**dxl2\_defs.h**](dxl2__defs_8h.md) instructions, error codes, IDs, little-endian helpers 
* [**dxl2\_protocol.h**](dxl2__protocol_8h.md) CRC-16, incremental parser, encoder with byte stuffing 
* [**dxl2\_link.h**](dxl2__link_8h.md) transport-independent link (stream in, packets out) 




    
## Macro Definition Documentation





### define DXL2\_VERSION 

_Version string "MAJOR.MINOR.PATCH"._ 
```
#define DXL2_VERSION `"1.0.0"`
```




<hr>



### define DXL2\_VERSION\_MAJOR 

```
#define DXL2_VERSION_MAJOR `1`
```




<hr>



### define DXL2\_VERSION\_MINOR 

```
#define DXL2_VERSION_MINOR `0`
```




<hr>



### define DXL2\_VERSION\_PATCH 

```
#define DXL2_VERSION_PATCH `0`
```




<hr>

------------------------------


