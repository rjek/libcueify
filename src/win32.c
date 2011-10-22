/* win32.c - win32-specific (Windows XP+) CD-ROM API glue
 *
 * Copyright (c) 2010, 2011 Ian Jacobi
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
#include <tchar.h>

LPCTSTR szVolumeFormat[9] = "\\\\.\\%lc:";
LPCTSTR szRootFormat[6] = "%lc:\\";

int cueify_device_open_unportable(cueify_device_private *d,
				  const char *device) {
    HANDLE hVolume;
    WCHAR cDriveLetter;
    UINT uDriveType;
    TCHAR szVolumeName[7];
    TCHAR szRootName[4];
    DWORD dwAccessFlags;

    /* Precondition: device[0] != '\0' */

    /* Is it a drive letter? (Regular Expression: c(?::\?\)?) */
    if ( device[1] == '\0' ||
	(device[1] == ':' && device[2] == '\0')) {
	/* Just a single letter. */
	cDriveLetter = device[0];
    } else {
	/* We currently don't handle drives without a letter. */
	return CUEIFY_BADARG;
    }
    wsprintf(szRootName, szRootFormat, cDriveLetter);

    uDriveType = GetDriveType(szRootName);
    switch (uDriveType) {
    case DRIVE_REMOVABLE:
	dwAccessFlags = GENERIC_READ | GENERIC_WRITE;
	break;
    case DRIVE_CDROM:
	dwAccessFlags = GENERIC_READ;
	break;
    default:
	return CUEIFY_ERR_NO_DEVICE;
    }
    wsprintf(szVolumeName, szVolumeFormat, cDriveLetter);

    hVolume = CreateFile(szVolumeName,
			 dwAccessFlags,
			 FILE_SHARE_READ | FILE_SHARE_WRITE,
			 NULL,
			 OPEN_EXISTING,
			 0,
			 NULL);
    if (hVolume == INVALID_HANDLE_VALUE) {
	return CUEIFY_ERR_NO_DEVICE;
    } else {
	d->handle = hVolume;
	return CUEIFY_OK;
    }
}  /* cueify_device_open_unportable */

int cueify_device_close_unportable(cueify_device_private *d) {
    if (CloseHandle(d->handle)) {
	return CUEIFY_OK;
    } else {
	return CUEIFY_ERR_INTERNAL;
    }
}  /* cueify_device_close_unportable */

const char *driveLetters[26] = {
    "a:\\", "b:\\", "c:\\", "d:\\", "e:\\", "f:\\", "g:\\", "h:\\",
    "i:\\", "j:\\", "k:\\", "l:\\", "m:\\", "n:\\", "o:\\", "p:\\",
    "q:\\", "r:\\", "s:\\", "t:\\", "u:\\", "v:\\", "w:\\", "x:\\",
    "y:\\", "z:\\"
};

const char *cueify_device_get_default_device_unportable() {
    int i;
    DWORD dwDrives;
    TCHAR szRootName[4] = "a:\\";

    dwDrives = GetLogicalDrives();
    for (i = 0; i < 26; i++) {
	if (dwDrives & 0x01) {
	    uDriveType = GetDriveType(szRootName);
	    if (uDriveType == DRIVE_CDROM) {
		return driveLetters[i];
	    }
	}
	dwDrives >>= 1;
	szRootName[0]++;
    }
    return NULL;
}  /* cueify_device_get_default_device_unportable */

int cueify_device_read_toc_unportable(cueify_device_private *d,
				      cueify_toc_private *t) {
    DWORD dwReturned;
    CDROM_READ_TOC_EX toc_ex;
    CDROM_TOC toc;
    BOOL succeded;
    int i;
    
    toc_ex.Format = CDROM_READ_TOC_EX_FORMAT_TOC;
    toc_ex.Reserved1 = 0;
    toc_ex.Msf = FALSE;
    toc_ex.SessionTrack = 1;
    toc_ex.Reserved2 = 0;
    toc_ex.Reserved3 = 0;
    
    succeded = DeviceIoControl(d->handle,
			       IOCTL_CDROM_READ_TOC_EX,
			       &toc_ex, sizeof(CDROM_READ_TOC_EX),
			       &toc, sizeof(CDROM_TOC),
			       &dwReturned, NULL);
    if (!succeeded) {
	return CUEIFY_ERR_INTERNAL;
    } else {
	t->first_track_number = toc->FirstTrack;
	t->last_track_number = toc->LastTrack;
	for (i = 0; i < MAXIMUM_NUMBER_TRACKS; i++) {
	    if (toc->TrackData[i].TrackNumber == 0xAA) {
		/* Lead-out Track */
		t->tracks[0].control = toc->TrackData[i].Control;
		t->tracks[0].adr = toc->TrackData[i].Adr;
		t->tracks[0].lba =
		    ((toc->TrackData[i].Address[0] << 24) |
		     (toc->TrackData[i].Address[1] << 16) |
		     (toc->TrackData[i].Address[2] << 8) |
		     toc->TrackData[i].Address[3]);
	    } else if (toc->TrackData[i].TrackNumber != 0) {
		t->tracks[toc->TrackData[i].TrackNumber].control =
		    toc->TrackData[i].Control;
		t->tracks[toc->TrackData[i].TrackNumber].adr =
		    toc->TrackData[i].Adr;
		t->tracks[toc->TrackData[i].TrackNumber].lba =
		    ((toc->TrackData[i].Address[0] << 24) |
		     (toc->TrackData[i].Address[1] << 16) |
		     (toc->TrackData[i].Address[2] << 8) |
		     toc->TrackData[i].Address[3]);
	    }
	}
    }

    return CUEIFY_OK;
}  /* cueify_device_read_toc_unportable */


int cueify_device_read_sessions_unportable(cueify_device_private *d,
					   cueify_sessions_private *s) {
{
    DWORD dwReturned;
    CDROM_READ_TOC_EX toc_ex;
    CDROM_TOC_SESSION_DATA session;
    BOOL succeded;

    toc_ex.Format = CDROM_READ_TOC_EX_FORMAT_SESSION;
    toc_ex.Reserved1 = 0;
    toc_ex.Msf = FALSE;
    toc_ex.SessionTrack = 0;
    toc_ex.Reserved2 = 0;
    toc_ex.Reserved3 = 0;

    succeeded = DeviceIoControl(d->device,
				IOCTL_CDROM_READ_TOC_EX,
				&toc_ex, sizeof(CDROM_READ_TOC_EX),
				&session, sizeof(CDROM_TOC_SESSION_DATA),
				&dwReturned, NULL);
    if (!succeeded) {
	return CUEIFY_ERR_INTERNAL;
    } else {
	s->first_session_number = session->FirstCompleteSession;
	s->last_session_number = session->LastCompleteSession;
	s->track_control = session->TrackData[0].Control;
	s->track_adr = session->TrackData[0].Adr;
	s->track_number = session->TrackData[0].TrackNumber;
	s->track_lba =
	  ((session->TrackData[0].Address[0] << 24) |
	   (session->TrackData[0].Address[1] << 16) |
	   (session->TrackData[0].Address[2] << 8) |
	   session->TrackData[0].Address[3]);
    }

    return CUEIFY_OK;
}

