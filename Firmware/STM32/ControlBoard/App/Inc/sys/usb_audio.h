#ifndef USB_AUDIO_H
#define USB_AUDIO_H

#include "sys/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Development USB identity. Replace these values with IDs assigned to the
 * product before shipping hardware.
 */
#ifndef USB_AUDIO_VENDOR_ID
#define USB_AUDIO_VENDOR_ID 0x0483u
#endif

#ifndef USB_AUDIO_PRODUCT_ID
#define USB_AUDIO_PRODUCT_ID 0xA2CAu
#endif

#define USB_AUDIO_SAMPLE_RATE_HZ 48000u
#define USB_AUDIO_CHANNEL_COUNT 2u
#define USB_AUDIO_BITS_PER_SAMPLE 16u

/* Live counters are intentionally exposed for debugger/watch-window use. */
typedef struct {
    uint32_t out_packets;
    uint32_t out_bytes;
    uint32_t out_nonzero_packets;
    uint32_t out_bad_packets;
    uint32_t ring_restarts;
    uint32_t feedback_packets;
    uint32_t feedback_completions;
    uint16_t last_out_length;
    uint32_t last_feedback_10_14;
} USB_AudioDiagnostics;

extern volatile USB_AudioDiagnostics usb_audio_diagnostics;

void USB_Audio_Init(void);

/* Attach the I2S DMA ring once the chassis audio path has been initialized. */
void USB_Audio_Attach(AudioData *audio);

#ifdef __cplusplus
}
#endif

#endif /* USB_AUDIO_H */
