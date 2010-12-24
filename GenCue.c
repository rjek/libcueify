/* GenCue.c - A utility to generate CD cuesheets.
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "cdrom.h"
#include "toc.h"

/** Subtract the 2-second disc pregap from the specified TrackIndex struct.
 *
 * @param index The TrackIndex struct to subtract two seconds from.
 * @return index, with two seconds subtracted from it.
 */
static struct TrackIndex RemoveDiscPregap(struct TrackIndex index)
{
    struct TrackIndex retval;
    
    retval.F = index.F;
    retval.S = index.S - 2;
    retval.M = index.M;
    
    if (retval.S >= 60) {
	retval.S += 60;
	retval.M--;
    }
    
    return retval;
}

/** Generate a Cuesheet file from the contents of the drive cDriveLetter.
 *
 * @param szFile The cuesheet file to write.
 * @param cDriveLetter The letter of the drive to generate the cuesheet from.
 * @return 0 if succeeded.
 */
int GenCuesheet(char *szFile, char cDriveLetter)
{
    /* Do some extra logging of disc data, since dBpoweramp falls down
     * on the job. */
    FILE *log = fopen(szFile, "wb");
    
    if (log != NULL) {
	time_t t = time(NULL);
	char szTime[256];
	HANDLE hDevice;
	CDROM_TOC toc;
	CDROM_TOC_FULL_TOC_DATA *fulltoc;
	CDROM_TOC_SESSION_DATA session;
	unsigned char cdtextBacking[4096];
	CDROM_TOC_CD_TEXT_DATA *cdtext = (CDROM_TOC_CD_TEXT_DATA *)cdtextBacking;
	SUB_Q_CHANNEL_DATA data;
	int iTrack, iIndex, iDescriptor;
	struct CDText *cdtextData;
	struct TrackIndices indices;
	BOOL bHasPregap = FALSE;
	struct TrackIndex pregap;
	struct TrackIndex offset;
	UCHAR curSession = 0;
	
	hDevice = OpenVolume(cDriveLetter);
	if (hDevice != INVALID_HANDLE_VALUE) {
	    strftime(szTime, 256, "%Y-%m-%dT%H:%M:%S", gmtime(&t));
	    /* First read the last-session data. */
	    ReadLastSession(hDevice, &session);
	    fprintf(log,
		    "REM GENTIME \"%s\"\n"
		    "REM DRIVE \"%c\"\n",
		    szTime,
		    cDriveLetter);
	    fprintf(log,
		    "REM FIRSTSESSION %d\n"
		    "REM LASTSESSION %d\n"
		    "REM LASTSESSION TRACK %02d\n",
		    session.FirstCompleteSession,
		    session.LastCompleteSession,
		    session.TrackData[0].TrackNumber);
	    if (session.TrackData[0].Control != 0) {
		int iControl = session.TrackData[0].Control;
		
		fprintf(log, "REM LASTSESSION FLAGS");
		
		if (iControl & AUDIO_WITH_PREEMPHASIS) {
		    fprintf(log, " PRE");
		}
		if (iControl & DIGITAL_COPY_PERMITTED) {
		    fprintf(log, " DCP");
		}
		if (iControl & AUDIO_DATA_TRACK) {
		    fprintf(log, " DATA");
		}
		if (iControl & TWO_FOUR_CHANNEL_AUDIO) {
		    fprintf(log, " 4CH");
		}
		fprintf(log, "\n");
	    }
		    
	    offset.M = session.TrackData[0].Address[1];
	    offset.S = session.TrackData[0].Address[2];
	    offset.F = session.TrackData[0].Address[3];
	    offset = RemoveDiscPregap(offset);
	    
	    fprintf(log,
		    "REM LASTSESSION INDEX 01 %02d:%02d:%02d\n",
		    offset.M,
		    offset.S,
		    offset.F);
	    
	    /* Then get the raw TOC data. */
	    ReadTOC(hDevice, &toc);
	    fulltoc = ReadFullTOC(hDevice);
	    /* And the CD-Text. */
	    cdtext = ReadCDText(hDevice);
	    /* And actually parse the CD-Text. */
	    cdtextData = ParseCDText(cdtext, toc.LastTrack);
	    
	    /* Write out the MCN. */
	    if (ReadMCN(hDevice, &data) && data.MediaCatalog.Mcval) {
		char szMCN[16] = "";
		memcpy(szMCN, data.MediaCatalog.MediaCatalog, 15);
		
		fprintf(log, "CATALOG %s\n", szMCN);
	    }
	    
	    /* Print any disc-level CD-Text strings. */
	    if (cdtextData != NULL) {
		if (cdtextData->tracks[0].arranger != NULL) {
		    fprintf(log, "REM ARRANGER \"%s\"\n",
			    cdtextData->tracks[0].arranger);
		}
		if (cdtextData->tracks[0].composer != NULL) {
		    fprintf(log, "REM COMPOSER \"%s\"\n",
			    cdtextData->tracks[0].composer);
		}
		if (cdtextData->tracks[0].discId != NULL) {
		    fprintf(log, "REM DISK_ID \"%s\"\n",
			    cdtextData->tracks[0].discId);
		}
		if (cdtextData->tracks[0].genre != NULL) {
		    fprintf(log, "REM GENRE \"%s\"\n",
			    cdtextData->tracks[0].genre);
		}
		if (cdtextData->tracks[0].messages != NULL) {
		    fprintf(log, "REM MESSAGE \"%s\"\n",
			    cdtextData->tracks[0].messages);
		}
		if (cdtextData->tracks[0].performer != NULL) {
		    fprintf(log, "PERFORMER \"%s\"\n",
			    cdtextData->tracks[0].performer);
		}
		if (cdtextData->tracks[0].songwriter != NULL) {
		    fprintf(log, "SONGWRITER \"%s\"\n",
			    cdtextData->tracks[0].songwriter);
		}
		if (cdtextData->tracks[0].title != NULL) {
		    fprintf(log, "TITLE \"%s\"\n",
			    cdtextData->tracks[0].title);
		}
		if (cdtextData->tracks[0].tocInfo != NULL) {
		    fprintf(log, "REM TOC_INFO %s\n",
			    cdtextData->tracks[0].tocInfo);
		}
		if (cdtextData->tracks[0].tocInfo2 != NULL) {
		    fprintf(log, "REM TOC_INFO2 %s\n",
			    cdtextData->tracks[0].tocInfo2);
		}
		if (cdtextData->tracks[0].upc_ean != NULL) {
		    fprintf(log, "REM CATALOG %s\n",
			    cdtextData->tracks[0].upc_ean);
		}
		if (cdtextData->tracks[0].sizeInfo != NULL) {
		    fprintf(log, "REM SIZE_INFO %s\n",
			    cdtextData->tracks[0].sizeInfo);
		}
	    }
	    
	    /* And lastly the track stuff. */
	    fprintf(log, "FILE \"disc.bin\" BINARY\n");
	    for (iTrack = toc.FirstTrack; iTrack <= toc.LastTrack; iTrack++) {
		/* Find the appropriate descriptor. */
		for (iDescriptor = 0; iDescriptor * sizeof(CDROM_TOC_FULL_TOC_DATA_BLOCK) < ((fulltoc->Length[0] << 8) | fulltoc->Length[1]) - 2 && (fulltoc->Descriptors[iDescriptor].Point != iTrack || fulltoc->Descriptors[iDescriptor].Adr != 1); iDescriptor++);
		if (curSession != fulltoc->Descriptors[iDescriptor].SessionNumber) {
		    if (curSession != 0) {
			/* Print the leadout of the last session. */
			int iLeadoutDescriptor;
			
			for (iLeadoutDescriptor = 0; iLeadoutDescriptor * sizeof(CDROM_TOC_FULL_TOC_DATA_BLOCK) < ((fulltoc->Length[0] << 8) | fulltoc->Length[1]) - 2 && (fulltoc->Descriptors[iLeadoutDescriptor].Point != 0xA2 || fulltoc->Descriptors[iLeadoutDescriptor].Adr != 1 || fulltoc->Descriptors[iLeadoutDescriptor].SessionNumber != curSession); iLeadoutDescriptor++);
			offset.M = fulltoc->Descriptors[iLeadoutDescriptor].Msf[0];
			offset.S = fulltoc->Descriptors[iLeadoutDescriptor].Msf[1];
			offset.F = fulltoc->Descriptors[iLeadoutDescriptor].Msf[2];
			offset = RemoveDiscPregap(offset);
			
			fprintf(log,
				"  REM LEAD-OUT %02d:%02d:%02d\n",
				offset.M,
				offset.S,
				offset.F);
		    }
		    curSession = fulltoc->Descriptors[iDescriptor].SessionNumber;
		    fprintf(log,
			    "  REM SESSION %02d\n",
			    curSession);
		}
		if (toc.TrackData[iTrack - 1].Control & AUDIO_DATA_TRACK) {
		    fprintf(log,
			    "    TRACK %02d MODE1/2352\n",
			    iTrack);
		} else {
		    fprintf(log,
			    "    TRACK %02d AUDIO\n",
			    iTrack);
		}
		
		if (cdtextData != NULL) {
		    if (cdtextData->tracks[iTrack].arranger != NULL) {
			fprintf(log, "      REM ARRANGER \"%s\"\n",
				cdtextData->tracks[iTrack].arranger);
		    }
		    if (cdtextData->tracks[iTrack].composer != NULL) {
			fprintf(log, "      REM COMPOSER \"%s\"\n",
				cdtextData->tracks[iTrack].composer);
		    }
		    if (cdtextData->tracks[iTrack].discId != NULL) {
			fprintf(log, "      REM DISK_ID \"%s\"\n",
				cdtextData->tracks[iTrack].discId);
		    }
		    if (cdtextData->tracks[iTrack].genre != NULL) {
			fprintf(log, "      REM GENRE \"%s\"\n",
				cdtextData->tracks[iTrack].genre);
		    }
		    if (cdtextData->tracks[iTrack].messages != NULL) {
			fprintf(log, "      REM MESSAGE \"%s\"\n",
				cdtextData->tracks[iTrack].messages);
		    }
		    if (cdtextData->tracks[iTrack].performer != NULL) {
			fprintf(log, "      PERFORMER \"%s\"\n",
				cdtextData->tracks[iTrack].performer);
		    }
		    if (cdtextData->tracks[iTrack].songwriter != NULL) {
			fprintf(log, "      SONGWRITER \"%s\"\n",
				cdtextData->tracks[iTrack].songwriter);
		    }
		    if (cdtextData->tracks[iTrack].title != NULL) {
			fprintf(log, "      TITLE \"%s\"\n",
				cdtextData->tracks[iTrack].title);
		    }
		    if (cdtextData->tracks[iTrack].tocInfo != NULL) {
			fprintf(log, "      REM TOC_INFO %s\n",
				cdtextData->tracks[iTrack].tocInfo);
		    }
		    if (cdtextData->tracks[iTrack].tocInfo2 != NULL) {
			fprintf(log, "      REM TOC_INFO2 %s\n",
				cdtextData->tracks[iTrack].tocInfo2);
		    }
		    if (cdtextData->tracks[iTrack].upc_ean != NULL) {
			fprintf(log, "      REM ISRC %s\n",
				cdtextData->tracks[iTrack].upc_ean);
		    }
		    if (cdtextData->tracks[iTrack].sizeInfo != NULL) {
			fprintf(log, "      REM SIZE_INFO %s\n",
				cdtextData->tracks[iTrack].sizeInfo);
		    }
		}
		
		if (ReadISRC(hDevice, iTrack, &data) && data.TrackIsrc.Tcval) {
		    char szISRC[16] = "";
		    memcpy(szISRC, data.TrackIsrc.TrackIsrc, 15);
		    
		    fprintf(log, "      ISRC %s\n", szISRC);
		}
		
		if ((toc.TrackData[iTrack - 1].Control & ~AUDIO_DATA_TRACK) != 0) {
		    int iControl = toc.TrackData[iTrack - 1].Control;
		    
		    fprintf(log, "      FLAGS");
		    
		    if ((iControl & AUDIO_WITH_PREEMPHASIS) > 0) {
			fprintf(log, " PRE");
		    }
		    if ((iControl & DIGITAL_COPY_PERMITTED) > 0) {
			fprintf(log, " DCP");
		    }
		    if ((iControl & TWO_FOUR_CHANNEL_AUDIO) > 0) {
			fprintf(log, " 4CH");
		    }
		    fprintf(log, "\n");
		}
		
		if (bHasPregap) {
		    pregap = RemoveDiscPregap(pregap);
		    fprintf(log,
			    "      INDEX 00 %02d:%02d:%02d\n",
			    pregap.M,
			    pregap.S,
			    pregap.F);
		    bHasPregap = FALSE;
		} else if (iTrack == 1 &&
			   (toc.TrackData[iTrack - 1].Address[1] != 0 ||
			    toc.TrackData[iTrack - 1].Address[2] > 2 ||
			    toc.TrackData[iTrack - 1].Address[3] != 0)) {
		    fprintf(log, "      INDEX 00 00:00:00\n");
		}
		
		offset.M = toc.TrackData[iTrack - 1].Address[1];
		offset.S = toc.TrackData[iTrack - 1].Address[2];
		offset.F = toc.TrackData[iTrack - 1].Address[3];
		offset = RemoveDiscPregap(offset);
		
		fprintf(log,
			"      INDEX 01 %02d:%02d:%02d\n",
			offset.M,
			offset.S,
			offset.F);
		
		/* Detect any other indices. */
		if (DetectTrackIndices(hDevice, &toc, iTrack, &indices)) {
		    for (iIndex = 1; iIndex < indices.iIndices; iIndex++) {
			if (iIndex + 1 == indices.iIndices &&
			    indices.bHasPregap) {
			    pregap = indices.indices[iIndex];
			    bHasPregap = TRUE;
			    continue;
			}
			
			offset = RemoveDiscPregap(indices.indices[iIndex]);
			
			fprintf(log,
				"      INDEX %02d %02d:%02d:%02d\n",
				iIndex + 1,
				offset.M,
				offset.S,
				offset.F);
		    }
		    free(indices.indices);
		}
	    }
	    
	    offset.M = toc.TrackData[iTrack - 1].Address[1];
	    offset.S = toc.TrackData[iTrack - 1].Address[2];
	    offset.F = toc.TrackData[iTrack - 1].Address[3];
	    offset = RemoveDiscPregap(offset);
	    
	    fprintf(log,
		    "  REM LEAD-OUT %02d:%02d:%02d\n",
		    offset.M,
		    offset.S,
		    offset.F);
	    
	    free(cdtext);
	    FreeCDText(cdtextData);
	    free(fulltoc);
	    CloseVolume(hDevice);
	}
	
	fclose(log);
	
	return 0;
    }
    
    /* We failed?? */
    return 1;
}

int main(int argc, char *argv[])
{
    char *szDriveLetter;
    char *szCuesheet;
    
    /* Just got two command-line arguments we expect: drive letter and log. */
    if (argc != 3) {
	printf("Usage: GenCue DRIVELETTER CUESHEET\n");
	return 0;
    }
    
    szDriveLetter = argv[1];
    szCuesheet = argv[2];
    
    if (GenCuesheet(szCuesheet, szDriveLetter[0])) {
	printf("There was an issue writing the cuesheet!\n");
    }
    
    return 0;
}
