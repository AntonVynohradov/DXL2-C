

# File dxl2\_config.h

[**File List**](files.md) **>** [**inc**](dir_bfccd401955b95cf8c75461437045ac0.md) **>** [**dxl2\_config.h**](dxl2__config_8h.md)

[Go to the documentation of this file](dxl2__config_8h.md)


```C++

#ifndef DXL2_CONFIG_H
#define DXL2_CONFIG_H

#ifndef DXL2_MAX_PACKET_SIZE
#define DXL2_MAX_PACKET_SIZE 256U
#endif

/*
 * The smallest packet (no parameters) is 10 bytes. The Length field is 16 bits
 * and counts everything after it, so a packet never exceeds 65535 + 7 bytes.
 */
#if (DXL2_MAX_PACKET_SIZE < 10U) || (DXL2_MAX_PACKET_SIZE > 65542U)
#error "DXL2_MAX_PACKET_SIZE must be in the range 10..65542"
#endif

#endif /* DXL2_CONFIG_H */

```


