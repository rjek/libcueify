/* toc.h - Header for CD-ROM functions which read the (basic) TOC of a
 * disc.
 *
 * Copyright (c) 2011 Ian Jacobi <pipian@pipian.com>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _CUEIFY_TOC_H
#define _CUEIFY_TOC_H

#include <cueify/device.h>
#include <cueify/constants.h>
#include <cueify/types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * A transparent handle for the table of contents (TOC) of an audio CD.
 *
 * This is returned by cueify_toc_new() and is passed as the first
 * parameter to all cueify_toc_*() functions.
 */
typedef void *cueify_toc;


/**
 * Create a new TOC instance. The instance is created with no tracks,
 * and should be populated using cueify_device_read_toc() or
 * cueify_toc_deserialize().
 *
 * @return NULL if there was an error allocating memory, else the new TOC
 */
cueify_toc *cueify_toc_new();


/**
 * Read the TOC of the disc in the optical disc device associated with
 * a device handle.
 *
 * @pre { d != NULL, t != NULL }
 * @param d an opened device handle
 * @param t a TOC instance to populate
 * @return CUEIFY_OK if the TOC was successfully read; otherwise an
 *         error code is returned
 */
int cueify_device_read_toc(cueify_device *d, cueify_toc *t);


/**
 * Deserialize a TOC instance previously serialized with
 * cueify_toc_serialize().
 *
 * @note This serialization is, in principle, the same as that
 *       returned by the MMC READ TOC/PMA/ATIP command with format
 *       0000b, and is compatible with the (poorly defined) definition
 *       of the MCI/MCDI frame in ID3v2.
 *
 * @pre { t != NULL, buffer != NULL }
 * @param t a TOC instance to populate
 * @param buffer a pointer to the serialized TOC data
 * @param size the size of the buffer
 * @return CUEIFY_OK if the TOC was successfully deserialized;
 *         otherwise an error code is returned
 */
int cueify_toc_deserialize(cueify_toc *t, const uint8_t * const buffer,
			   size_t size);


/**
 * Serialize a TOC instance for later deserialization with
 * cueify_toc_deserialize().
 *
 * @note This serialization is, in principle, the same as that
 *       returned by the MMC READ TOC/PMA/ATIP command with format
 *       0000b, and is compatible with the (poorly defined) definition
 *       of the MCI/MCDI frame in ID3v2.
 *
 * @pre { t != NULL, size != NULL }
 * @param t a TOC instance to serialize
 * @param buffer a pointer to a location to serialize data to, or NULL
 *               to determine the optimal size of such a buffer
 * @param size a pointer to the size of the buffer. When called, the
 *             size must contain the maximum number of bytes that may
 *             be stored in buffer. When this function is complete,
 *             the pointer will contain the number of bytes
 *             needed to fully serialize the TOC instance.
 * @return CUEIFY_OK if the TOC was successfully serialized; otherwise
 *         an error code is returned
 */
int cueify_toc_serialize(cueify_toc *t, uint8_t *buffer, size_t *size);


/**
 * Free a TOC instance. Deletes the object pointed to by t.
 *
 * @pre { t != NULL }
 * @param t a cueify_toc object created by cueify_toc_new()
 */
void cueify_toc_free(cueify_toc *t);


/**
 * Get the number of the first track in a TOC instance.
 *
 * @pre { t != NULL }
 * @param t a TOC instance
 * @return the number of the first track in t
 */
uint8_t cueify_toc_get_first_track(cueify_toc *t);


/**
 * Get the number of the last track in a TOC instance.
 *
 * @pre { t != NULL }
 * @param t a TOC instance
 * @return the number of the last track in t
 */
uint8_t cueify_toc_get_last_track(cueify_toc *t);


/**
 * Get the track control flags for a track in a TOC instance.
 *
 * @pre { t != NULL,
 *        cueify_toc_get_first_track(t)<=track<=cueify_toc_get_last_track(t) OR
 *        track == CUEIFY_LEAD_OUT_TRACK }
 * @param t a TOC instance
 * @param track the number of the track for which control flags should
 *              be returned
 * @return the control flags for track number track in t
 */
uint8_t cueify_toc_get_track_control_flags(cueify_toc *t, uint8_t track);


/**
 * Get the format of the content of the sub-Q-channel in the block in
 * which a track in a TOC instance was found.
 *
 * @note In most cases, this function will return
 *       CUEIFY_SUB_Q_NOTHING, however it is provided for completeness.
 *
 * @pre { t != NULL,
 *        cueify_toc_get_first_track(t)<=track<=cueify_toc_get_last_track(t) OR
 *        track == CUEIFY_LEAD_OUT_TRACK }
 * @param t a TOC instance
 * @param track the number of the track for which the sub-Q-channel
 *              format should be returned
 * @return the format of the contents of the sub-Q-channel for track
 *         number track in t
 */
uint8_t cueify_toc_get_track_sub_q_channel_format(cueify_toc *t,
						  uint8_t track);


/**
 * Get the absolute CD-frame address (LBA) of the start of a track in
 * a TOC instance.
 *
 * @pre { t != NULL,
 *        cueify_toc_get_first_track(t)<=track<=cueify_toc_get_last_track(t) OR
 *        track == CUEIFY_LEAD_OUT_TRACK }
 * @param t a TOC instance
 * @param track the number of the track for which the address should
 *              be returned
 * @return the address of the start of track number track in t
 */
uint32_t cueify_toc_get_track_address(cueify_toc *t, uint8_t track);


/**
 * Get the total number of CD-frames in a TOC instance.
 *
 * @note Shorthand for cueify_toc_get_track_address(t, CUEIFY_LEAD_OUT_TRACK).
 *
 * @pre { t != NULL }
 * @param t a TOC instance
 * @return the total number of CD-frames in t
 */
#define cueify_toc_get_disc_length(t)  \
    cueify_toc_get_track_address(t, CUEIFY_LEAD_OUT_TRACK)


/**
 * Get the total number of CD-frames in a track in a TOC instance.
 *
 * @pre { t != NULL,
 *        cueify_toc_get_first_track(t)<=track<=cueify_toc_get_last_track(t) }
 * @param t a TOC instance
 * @param track the number of the track for which the total number of
 *              CD-frames be returned
 * @return the total number of CD-frames in track number track in t
 */
uint32_t cueify_toc_get_track_length(cueify_toc *t, uint8_t track);

#ifdef __cplusplus
};  /* extern "C" */
#endif  /* __cplusplus */

#endif /* _CUEIFY_TOC_H */
