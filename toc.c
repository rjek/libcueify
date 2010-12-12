/* toc.c - Advanced TOC-based CD-ROM functions.
 *
 * Copyright (c) 2010 Ian Jacobi
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

#include <windows.h>
#include "toc.h"

int AddressToSectors(UCHAR address[4])
{
    return (((address[1] * 60) + address[2]) * 75) + address[3];
}

BOOL ReadLastSession(HANDLE hDevice, CDROM_TOC_SESSION_DATA *session)
{
    DWORD dwReturned;
    CDROM_READ_TOC_EX toc_ex;
    
    toc_ex.Format = CDROM_READ_TOC_EX_FORMAT_SESSION;
    toc_ex.Reserved1 = 0;
    toc_ex.Msf = TRUE;
    toc_ex.SessionTrack = 0;
    toc_ex.Reserved2 = 0;
    toc_ex.Reserved3 = 0;
    
    return DeviceIoControl(hDevice,
			   IOCTL_CDROM_READ_TOC_EX,
			   &toc_ex, sizeof(CDROM_READ_TOC_EX),
			   session, sizeof(CDROM_TOC_SESSION_DATA),
			   &dwReturned, NULL);
}

BOOL ReadTOC(HANDLE hDevice, CDROM_TOC *toc)
{
    DWORD dwReturned;
    CDROM_READ_TOC_EX toc_ex;
    
    toc_ex.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
    toc_ex.Reserved1 = 0;
    toc_ex.Msf = TRUE;
    toc_ex.SessionTrack = 1;
    toc_ex.Reserved2 = 0;
    toc_ex.Reserved3 = 0;
    
    return DeviceIoControl(hDevice,
			   IOCTL_CDROM_READ_TOC_EX,
			   &toc_ex, sizeof(CDROM_READ_TOC_EX),
			   toc, sizeof(CDROM_TOC),
			   &dwReturned, NULL);
}

BOOL ReadCDText(HANDLE hDevice, CDROM_TOC_CD_TEXT_DATA *cdtext, size_t size)
{
    DWORD dwReturned;
    CDROM_READ_TOC_EX toc_ex;
    
    toc_ex.Format = CDROM_READ_TOC_EX_FORMAT_CDTEXT;
    toc_ex.Reserved1 = 0;
    toc_ex.Msf = FALSE;
    toc_ex.SessionTrack = 0;
    toc_ex.Reserved2 = 0;
    toc_ex.Reserved3 = 0;
    
    return DeviceIoControl(hDevice,
			   IOCTL_CDROM_READ_TOC_EX,
			   &toc_ex, sizeof(CDROM_READ_TOC_EX),
			   cdtext, size,
			   &dwReturned, NULL);
}

BOOL ReadMCN(HANDLE hDevice, SUB_Q_CHANNEL_DATA *data)
{
    DWORD dwReturned;
    CDROM_SUB_Q_DATA_FORMAT format;
    
    format.Track = 0;
    format.Format = IOCTL_CDROM_MEDIA_CATALOG;
    
    return DeviceIoControl(hDevice,
			   IOCTL_CDROM_READ_Q_CHANNEL,
			   &format, sizeof(format),
			   data, sizeof(*data),
			   &dwReturned, NULL);
}

BOOL ReadISRC(HANDLE hDevice, int iTrack, SUB_Q_CHANNEL_DATA *data)
{
    DWORD dwReturned;
    CDROM_SUB_Q_DATA_FORMAT format;
    
    format.Track = iTrack;
    format.Format = IOCTL_CDROM_TRACK_ISRC;
    
    return DeviceIoControl(hDevice,
			   IOCTL_CDROM_READ_Q_CHANNEL,
			   &format, sizeof(format),
			   data, sizeof(*data),
			   &dwReturned, NULL);
}
