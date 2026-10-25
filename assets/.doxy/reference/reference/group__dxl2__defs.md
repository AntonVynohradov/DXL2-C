

# Group dxl2\_defs



[**Modules**](modules.md) **>** [**dxl2\_defs**](group__dxl2__defs.md)



_Protocol 2.0 IDs, instructions, error codes and byte-order helpers._ [More...](#detailed-description)








































## Public Static Functions

| Type | Name |
| ---: | :--- |
|  uint16\_t | [**dxl2\_get\_u16**](#function-dxl2_get_u16) (const uint8\_t \* data) <br>_Read a little-endian 16-bit value._  |
|  uint32\_t | [**dxl2\_get\_u32**](#function-dxl2_get_u32) (const uint8\_t \* data) <br>_Read a little-endian 32-bit value._  |
|  void | [**dxl2\_put\_u16**](#function-dxl2_put_u16) (uint8\_t \* data, uint16\_t value) <br>_Write a 16-bit value in little-endian order._  |
|  void | [**dxl2\_put\_u32**](#function-dxl2_put_u32) (uint8\_t \* data, uint32\_t value) <br>_Write a 32-bit value in little-endian order._  |

























## Macros

| Type | Name |
| ---: | :--- |
| define  | [**DXL2\_BROADCAST\_ID**](group__dxl2__defs.md#define-dxl2_broadcast_id)  `0xFEU`<br>_Broadcast ID: every device executes the instruction._  |
| define  | [**DXL2\_ERR\_ACCESS**](group__dxl2__defs.md#define-dxl2_err_access)  `0x07U`<br> |
| define  | [**DXL2\_ERR\_ALERT\_BIT**](group__dxl2__defs.md#define-dxl2_err_alert_bit)  `0x80U`<br>_Hardware alert flag: the device has a Hardware Error Status set._  |
| define  | [**DXL2\_ERR\_CRC**](group__dxl2__defs.md#define-dxl2_err_crc)  `0x03U`<br> |
| define  | [**DXL2\_ERR\_DATA\_LENGTH**](group__dxl2__defs.md#define-dxl2_err_data_length)  `0x05U`<br> |
| define  | [**DXL2\_ERR\_DATA\_LIMIT**](group__dxl2__defs.md#define-dxl2_err_data_limit)  `0x06U`<br> |
| define  | [**DXL2\_ERR\_DATA\_RANGE**](group__dxl2__defs.md#define-dxl2_err_data_range)  `0x04U`<br> |
| define  | [**DXL2\_ERR\_INSTRUCTION**](group__dxl2__defs.md#define-dxl2_err_instruction)  `0x02U`<br> |
| define  | [**DXL2\_ERR\_NONE**](group__dxl2__defs.md#define-dxl2_err_none)  `0x00U`<br> |
| define  | [**DXL2\_ERR\_RESULT\_FAIL**](group__dxl2__defs.md#define-dxl2_err_result_fail)  `0x01U`<br> |
| define  | [**DXL2\_INST\_ACTION**](group__dxl2__defs.md#define-dxl2_inst_action)  `0x05U`<br> |
| define  | [**DXL2\_INST\_BACKUP**](group__dxl2__defs.md#define-dxl2_inst_backup)  `0x20U`<br> |
| define  | [**DXL2\_INST\_BULK\_READ**](group__dxl2__defs.md#define-dxl2_inst_bulk_read)  `0x92U`<br> |
| define  | [**DXL2\_INST\_BULK\_WRITE**](group__dxl2__defs.md#define-dxl2_inst_bulk_write)  `0x93U`<br> |
| define  | [**DXL2\_INST\_CLEAR**](group__dxl2__defs.md#define-dxl2_inst_clear)  `0x10U`<br> |
| define  | [**DXL2\_INST\_FACTORY\_RESET**](group__dxl2__defs.md#define-dxl2_inst_factory_reset)  `0x06U`<br> |
| define  | [**DXL2\_INST\_FAST\_BULK\_READ**](group__dxl2__defs.md#define-dxl2_inst_fast_bulk_read)  `0x9AU`<br> |
| define  | [**DXL2\_INST\_FAST\_SYNC\_READ**](group__dxl2__defs.md#define-dxl2_inst_fast_sync_read)  `0x8AU`<br> |
| define  | [**DXL2\_INST\_PING**](group__dxl2__defs.md#define-dxl2_inst_ping)  `0x01U`<br> |
| define  | [**DXL2\_INST\_READ**](group__dxl2__defs.md#define-dxl2_inst_read)  `0x02U`<br> |
| define  | [**DXL2\_INST\_REBOOT**](group__dxl2__defs.md#define-dxl2_inst_reboot)  `0x08U`<br> |
| define  | [**DXL2\_INST\_REG\_WRITE**](group__dxl2__defs.md#define-dxl2_inst_reg_write)  `0x04U`<br> |
| define  | [**DXL2\_INST\_STATUS**](group__dxl2__defs.md#define-dxl2_inst_status)  `0x55U`<br>_Instruction byte of a Status Packet._  |
| define  | [**DXL2\_INST\_SYNC\_READ**](group__dxl2__defs.md#define-dxl2_inst_sync_read)  `0x82U`<br> |
| define  | [**DXL2\_INST\_SYNC\_WRITE**](group__dxl2__defs.md#define-dxl2_inst_sync_write)  `0x83U`<br> |
| define  | [**DXL2\_INST\_WRITE**](group__dxl2__defs.md#define-dxl2_inst_write)  `0x03U`<br> |
| define  | [**DXL2\_MAX\_ID**](group__dxl2__defs.md#define-dxl2_max_id)  `252U`<br>_Highest ID a device may use._  |

## Detailed Description


Values come from the ROBOTIS Protocol 2.0 specification ([https://docs.robotis.com/docs/dxl/protocol/protocol2/](https://docs.robotis.com/docs/dxl/protocol/protocol2/)). 


    
## Public Static Functions Documentation




### function dxl2\_get\_u16 

_Read a little-endian 16-bit value._ 
```
static inline uint16_t dxl2_get_u16 (
    const uint8_t * data
) 
```




<hr>



### function dxl2\_get\_u32 

_Read a little-endian 32-bit value._ 
```
static inline uint32_t dxl2_get_u32 (
    const uint8_t * data
) 
```




<hr>



### function dxl2\_put\_u16 

_Write a 16-bit value in little-endian order._ 
```
static inline void dxl2_put_u16 (
    uint8_t * data,
    uint16_t value
) 
```




<hr>



### function dxl2\_put\_u32 

_Write a 32-bit value in little-endian order._ 
```
static inline void dxl2_put_u32 (
    uint8_t * data,
    uint32_t value
) 
```




<hr>
## Macro Definition Documentation





### define DXL2\_BROADCAST\_ID 

_Broadcast ID: every device executes the instruction._ 
```
#define DXL2_BROADCAST_ID `0xFEU`
```




<hr>



### define DXL2\_ERR\_ACCESS 

```
#define DXL2_ERR_ACCESS `0x07U`
```




<hr>



### define DXL2\_ERR\_ALERT\_BIT 

_Hardware alert flag: the device has a Hardware Error Status set._ 
```
#define DXL2_ERR_ALERT_BIT `0x80U`
```




<hr>



### define DXL2\_ERR\_CRC 

```
#define DXL2_ERR_CRC `0x03U`
```




<hr>



### define DXL2\_ERR\_DATA\_LENGTH 

```
#define DXL2_ERR_DATA_LENGTH `0x05U`
```




<hr>



### define DXL2\_ERR\_DATA\_LIMIT 

```
#define DXL2_ERR_DATA_LIMIT `0x06U`
```




<hr>



### define DXL2\_ERR\_DATA\_RANGE 

```
#define DXL2_ERR_DATA_RANGE `0x04U`
```




<hr>



### define DXL2\_ERR\_INSTRUCTION 

```
#define DXL2_ERR_INSTRUCTION `0x02U`
```




<hr>



### define DXL2\_ERR\_NONE 

```
#define DXL2_ERR_NONE `0x00U`
```




<hr>



### define DXL2\_ERR\_RESULT\_FAIL 

```
#define DXL2_ERR_RESULT_FAIL `0x01U`
```




<hr>



### define DXL2\_INST\_ACTION 

```
#define DXL2_INST_ACTION `0x05U`
```




<hr>



### define DXL2\_INST\_BACKUP 

```
#define DXL2_INST_BACKUP `0x20U`
```




<hr>



### define DXL2\_INST\_BULK\_READ 

```
#define DXL2_INST_BULK_READ `0x92U`
```




<hr>



### define DXL2\_INST\_BULK\_WRITE 

```
#define DXL2_INST_BULK_WRITE `0x93U`
```




<hr>



### define DXL2\_INST\_CLEAR 

```
#define DXL2_INST_CLEAR `0x10U`
```




<hr>



### define DXL2\_INST\_FACTORY\_RESET 

```
#define DXL2_INST_FACTORY_RESET `0x06U`
```




<hr>



### define DXL2\_INST\_FAST\_BULK\_READ 

```
#define DXL2_INST_FAST_BULK_READ `0x9AU`
```




<hr>



### define DXL2\_INST\_FAST\_SYNC\_READ 

```
#define DXL2_INST_FAST_SYNC_READ `0x8AU`
```




<hr>



### define DXL2\_INST\_PING 

```
#define DXL2_INST_PING `0x01U`
```




<hr>



### define DXL2\_INST\_READ 

```
#define DXL2_INST_READ `0x02U`
```




<hr>



### define DXL2\_INST\_REBOOT 

```
#define DXL2_INST_REBOOT `0x08U`
```




<hr>



### define DXL2\_INST\_REG\_WRITE 

```
#define DXL2_INST_REG_WRITE `0x04U`
```




<hr>



### define DXL2\_INST\_STATUS 

_Instruction byte of a Status Packet._ 
```
#define DXL2_INST_STATUS `0x55U`
```




<hr>



### define DXL2\_INST\_SYNC\_READ 

```
#define DXL2_INST_SYNC_READ `0x82U`
```




<hr>



### define DXL2\_INST\_SYNC\_WRITE 

```
#define DXL2_INST_SYNC_WRITE `0x83U`
```




<hr>



### define DXL2\_INST\_WRITE 

```
#define DXL2_INST_WRITE `0x03U`
```




<hr>



### define DXL2\_MAX\_ID 

_Highest ID a device may use._ 
```
#define DXL2_MAX_ID `252U`
```




<hr>

------------------------------


