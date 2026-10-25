

# File dxl2\_defs.h

[**File List**](files.md) **>** [**inc**](dir_bfccd401955b95cf8c75461437045ac0.md) **>** [**dxl2\_defs.h**](dxl2__defs_8h.md)

[Go to the documentation of this file](dxl2__defs_8h.md)


```C++

#ifndef DXL2_DEFS_H
#define DXL2_DEFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DXL2_MAX_ID 252U
#define DXL2_BROADCAST_ID 0xFEU

#define DXL2_INST_PING 0x01U
#define DXL2_INST_READ 0x02U
#define DXL2_INST_WRITE 0x03U
#define DXL2_INST_REG_WRITE 0x04U
#define DXL2_INST_ACTION 0x05U
#define DXL2_INST_FACTORY_RESET 0x06U
#define DXL2_INST_REBOOT 0x08U
#define DXL2_INST_CLEAR 0x10U
#define DXL2_INST_BACKUP 0x20U
#define DXL2_INST_STATUS 0x55U
#define DXL2_INST_SYNC_READ 0x82U
#define DXL2_INST_SYNC_WRITE 0x83U
#define DXL2_INST_FAST_SYNC_READ 0x8AU
#define DXL2_INST_BULK_READ 0x92U
#define DXL2_INST_BULK_WRITE 0x93U
#define DXL2_INST_FAST_BULK_READ 0x9AU

#define DXL2_ERR_NONE 0x00U
#define DXL2_ERR_RESULT_FAIL 0x01U
#define DXL2_ERR_INSTRUCTION 0x02U
#define DXL2_ERR_CRC 0x03U
#define DXL2_ERR_DATA_RANGE 0x04U
#define DXL2_ERR_DATA_LENGTH 0x05U
#define DXL2_ERR_DATA_LIMIT 0x06U
#define DXL2_ERR_ACCESS 0x07U
#define DXL2_ERR_ALERT_BIT 0x80U

static inline uint16_t dxl2_get_u16(const uint8_t* data)
{
    return (uint16_t) ((uint16_t) data[0] | ((uint16_t) data[1] << 8));
}

static inline uint32_t dxl2_get_u32(const uint8_t* data)
{
    return (uint32_t) data[0] | ((uint32_t) data[1] << 8) | ((uint32_t) data[2] << 16)
           | ((uint32_t) data[3] << 24);
}

static inline void dxl2_put_u16(uint8_t* data, uint16_t value)
{
    data[0] = (uint8_t) (value & 0xFFU);
    data[1] = (uint8_t) (value >> 8);
}

static inline void dxl2_put_u32(uint8_t* data, uint32_t value)
{
    data[0] = (uint8_t) (value & 0xFFU);
    data[1] = (uint8_t) ((value >> 8) & 0xFFU);
    data[2] = (uint8_t) ((value >> 16) & 0xFFU);
    data[3] = (uint8_t) (value >> 24);
}

#ifdef __cplusplus
}
#endif

#endif /* DXL2_DEFS_H */

```


