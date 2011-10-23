/* cueify.c - A utility to generate CD cuesheets from a CD.
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

#include <stdio.h>
#include <time.h>
#include <libcueify/device.h>
#include <libcueify/error.h>
#include <libcueify/toc.h>
#include <libcueify/sessions.h>
#include <libcueify/full_toc.h>
#include <libcueify/types.h>

const char * const genreNames[0x1D] = {
    "NULL",
    "Unknown",
    "Adult Contemporary",
    "Alternative Rock",
    "Childrens",
    "Classical",
    "Contemporary Christian",
    "Country",
    "Dance",
    "Easy Listening",
    "Erotic",
    "Folk",
    "Gospel",
    "Hip Hop",
    "Jazz",
    "Latin",
    "Musical",
    "New Age",
    "Opera",
    "Operetta",
    "Pop",
    "Rap",
    "Reggae",
    "Rock",
    "Rhythm and Blues",
    "Sound Effects",
    "Soundtrack",
    "Spoken Word",
    "World Music"
};

const char * const languageNames[] = {
    "UNKNOWN", /* 0X00 */
    "ALBANIAN",
    "BRETON",
    "CATALAN",
    "CROATIAN",
    "WELSH",
    "CZECH",
    "DANISH",
    "GERMAN",
    "ENGLISH",
    "SPANISH",
    "ESPERANTO",
    "ESTONIAN",
    "BASQUE",
    "FAROESE",
    "FRENCH",
    "FRISIAN", /* 0X10 */
    "IRISH",
    "GAELIC",
    "GALICIAN",
    "ICELANDIC",
    "ITALIAN",
    "SAMI",
    "LATIN",
    "LATVIAN",
    "LUXEMBOURGISH",
    "LITHUANIAN",
    "HUNGARIAN",
    "MALTESE",
    "DUTCH",
    "NORWEGIAN",
    "OCCITAN",
    "POLISH", /* 0X20 */
    "PORTUGUESE",
    "ROMANIAN",
    "ROMANSH",
    "SERBIAN",
    "SLOVAK",
    "SLOVENIAN",
    "FINNISH",
    "SWEDISH",
    "TURKISH",
    "FLEMISH",
    "WALLOON",
    "",
    "",
    "",
    "",
    "", /* 0X30 */
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "", /* 0X40 */
    "",
    "",
    "",
    "",
    "ZULU",
    "VIETNAMESE",
    "UZBEK",
    "URDU",
    "UKRAINIAN",
    "THAI",
    "TELUGU",
    "TATAR",
    "TAMIL",
    "TAJIK",
    "SWAHILI",
    "SRANAN_TONGO",
    "SOMALI",
    "SINHALA",
    "SHONA",
    "SERBOCROAT",
    "RUTHENIAN",
    "RUSSIAN",
    "QUECHUA",
    "PUSHTU",
    "PUNJABI",
    "PERSIAN",
    "PAPIAMENTO",
    "ORIYA",
    "NEPALI",
    "NDEBELE",
    "MARATHI",
    "MOLDAVIAN",
    "MALAYSIAN",
    "MALAGASY",
    "MACEDONIAN",
    "LAO",
    "KOREAN",
    "KHMER",
    "KAZAKH",
    "KANNADA",
    "JAPANESE",
    "INDONESIAN",
    "HINDI",
    "HEBREW",
    "HAUSA",
    "GUARANI",
    "GUJARATI",
    "GREEK",
    "GEORGIAN",
    "FULAH",
    "DARI",
    "CHUVASH",
    "CHINESE",
    "BURMESE",
    "BULGARIAN",
    "BENGALI",
    "BELARUSIAN",
    "BAMBARA",
    "AZERBAIJANI",
    "ASSAMESE",
    "ARMENIAN",
    "ARABIC",
    "AMHARIC"
};

/** Get the MSF time for an absolute LBA offset.
 *
 * @param offset an offset to get the MSF time for.
 * @return offset in MSF time, excluding the pre-gap
 */
cueify_msf_t lba_to_msf(uint32_t offset)
{
    cueify_msf_t retval;

    retval.frm = offset % 75;
    retval.sec = offset / 75 % 60;
    retval.min = offset / 75 / 60;

    return retval;
}

/** Perform a partial sum of the CDDB DiscID.
 *
 * @param n The number of seconds in a track.
 * @return The partial sum of the CDDB DiscID so far.
 */
/*
int cddb_sum(int n)
{
    int ret;

    * For backward compatibility this algorithm must not change *

    ret = 0;
    while (n > 0) {
	ret = ret + (n % 10);
	n = n / 10;
    }
    return (ret);
}
*/

/** Generate the CDDB DiscID for a given CDROM_TOC.
 *
 * @param toc A pointer to the CDROM_TOC to generate a DiscID for.
 * @return The CDDB DiscID for this disc.
 */
/*
unsigned long cddb_discid(CDROM_TOC *toc)
{
    int i,
	t = 0,
	n = 0;
    
    * For backward compatibility this algorithm must not change *

    i = 0;
    while (i < toc->LastTrack) {
	n = n + cddb_sum((toc->TrackData[i].Address[1] * 60) + toc->TrackData[i].Address[2]);
	i++;
    }
    t = ((toc->TrackData[toc->LastTrack].Address[1] * 60) + toc->TrackData[toc->LastTrack].Address[2]) - ((toc->TrackData[0].Address[1] * 60) + toc->TrackData[0].Address[2]);
    return ((n % 0xff) << 24 | t << 8 | toc->LastTrack);
}
*/

/** Write a cuesheet file to STDOUT based on the contents of an optical
 * disc (CD-ROM) device.
 *
 * @param device the device to read out a cuesheet for
 * @return 0 if succeeded.
 */
int print_cuesheet(const char *device) {
    cueify_device *dev;
    cueify_toc *toc;
    cueify_sessions *sessions;
    cueify_full_toc *fulltoc;
    time_t t = time(NULL);
    char time_str[256];
    int i;
    cueify_msf_t offset;
    uint8_t cur_session = 0;

    dev = cueify_device_new();
    if (dev == NULL) {
	return 1;
    }

    if (cueify_device_open(dev, device) == CUEIFY_OK) {
	if (device == NULL) {
	    device = cueify_device_get_default_device();
	}
	strftime(time_str, 256, "%Y-%m-%dT%H:%M:%S", gmtime(&t));
	printf("REM GENTIME \"%s\"\n"
	       "REM DRIVE \"%s\"\n",
	       time_str,
	       device);

	/* First read the last-session data. */
	sessions = cueify_sessions_new();
	if (sessions == NULL) {
	    goto error;
	}
	if (cueify_device_read_sessions(dev, sessions) == CUEIFY_OK) {
	    /* It may be the case we can't read sessions (e.g permissions)! */
	    printf("REM FIRSTSESSION %d\n"
		   "REM LASTSESSION %d\n"
		   "REM LASTSESSION TRACK %02d\n",
		   cueify_sessions_get_first_session(sessions),
		   cueify_sessions_get_last_session(sessions),
		   cueify_sessions_get_last_session_track_number(sessions));
	    if (cueify_sessions_get_last_session_control_flags(sessions) != 0){
		int control =
		    cueify_sessions_get_last_session_control_flags(sessions);

		printf("REM LASTSESSION FLAGS");

		if ((control & CUEIFY_TOC_TRACK_HAS_PREEMPHASIS) > 0) {
		    printf(" PRE");
		}
		if ((control & CUEIFY_TOC_TRACK_PERMITS_COPYING) > 0) {
		    printf(" DCP");
		}
		if ((control & CUEIFY_TOC_TRACK_IS_QUADRAPHONIC) > 0) {
		    printf(" 4CH");
		}
		printf("\n");
	    }

	    offset =
		lba_to_msf(cueify_sessions_get_last_session_address(sessions));

	    printf("REM LASTSESSION INDEX 01 %02d:%02d:%02d\n",
		   offset.min,
		   offset.sec,
		   offset.frm);
	}

	cueify_sessions_free(sessions);

	/* Now get the full TOC data. */
	toc = cueify_toc_new();
	if (toc == NULL) {
	    goto error;
	}
	cueify_device_read_toc(dev, toc);

	fulltoc = cueify_full_toc_new();
	if (cueify_device_read_full_toc(dev, fulltoc) != CUEIFY_OK) {
	    cueify_full_toc_free(fulltoc);
	    fulltoc = NULL;
	}
	/* And the CD-Text. *
	cdtext = ReadCDText(hDevice);
	
	* We can write out the CD-Text. *
	if (cdtext != NULL &&
	    ((cdtext->Length[0] << 8) | cdtext->Length[1]) > 2) {
	    cdt = fopen(szCDTextFile, "wb");
	    if (cdt == NULL) {
		fclose(cdt);
		goto error;
	    }
	    
	    fwrite(cdtext, 1, ((cdtext->Length[0] << 8) | cdtext->Length[1]) + 2,
		   cdt);
	    
	    fclose(cdt);
	}
	
	* And actually parse the CD-Text. *
	cdtextData = ParseCDText(cdtext);
	
	* Write out the MCN. *
	if (ReadMCN(hDevice, &data) && data.MediaCatalog.Mcval) {
	    char szMCN[16] = "";
	    memcpy(szMCN, data.MediaCatalog.MediaCatalog, 15);
	    
	    fprintf(log, "CATALOG %s\n", szMCN);
	}
	
	* Print any disc-level CD-Text strings. *
	if (cdtextData != NULL) {
	    for (iBlock = 0; iBlock < 8; iBlock++) {
		if (cdtextData->blocks[iBlock].arrangers != NULL &&
		    cdtextData->blocks[iBlock].arrangers[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM ARRANGER \"%s\"\n",
				cdtextData->blocks[iBlock].arrangers[0]);
		    } else {
			fprintf(log, "REM ARRANGER_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].arrangers[0]);
		    }
		}
		if (cdtextData->blocks[iBlock].composers != NULL &&
		    cdtextData->blocks[iBlock].composers[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM COMPOSER \"%s\"\n",
				cdtextData->blocks[iBlock].composers[0]);
		    } else {
			fprintf(log, "REM COMPOSER_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].composers[0]);
		    }
		}
		if (cdtextData->blocks[iBlock].discID != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM DISK_ID \"%s\"\n",
				cdtextData->blocks[iBlock].discID);
		    } else {
			fprintf(log, "REM DISK_ID_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].discID);
		    }
		}
		if (cdtextData->blocks[iBlock].genreName != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM GENRE \"%s\"\n",
				genreNames[cdtextData->blocks[iBlock].genreCode]);
			if (cdtextData->blocks[iBlock].genreName[0] != '\0') {
			    fprintf(log, "REM SUPPLEMENTAL_GENRE \"%s\"\n",
				    cdtextData->blocks[iBlock].genreName);
			}
		    } else {
			fprintf(log, "REM GENRE_%d \"%s\"\n", iBlock,
				genreNames[cdtextData->blocks[iBlock].genreCode]);
			if (cdtextData->blocks[iBlock].genreName[0] != '\0') {
			    fprintf(log, "REM SUPPLEMENTAL_GENRE_%d \"%s\"\n",
				    iBlock,
				    cdtextData->blocks[iBlock].genreName);
			}
		    }
		}
		if (cdtextData->blocks[iBlock].messages != NULL &&
		    cdtextData->blocks[iBlock].messages[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM MESSAGE \"%s\"\n",
				cdtextData->blocks[iBlock].messages[0]);
		    } else {
			fprintf(log, "REM MESSAGE_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].messages[0]);
		    }
		}
		if (cdtextData->blocks[iBlock].performers != NULL &&
		    cdtextData->blocks[iBlock].performers[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "PERFORMER \"%s\"\n",
				cdtextData->blocks[iBlock].performers[0]);
		    } else {
			fprintf(log, "REM PERFORMER_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].performers[0]);
		    }
		}
		if (cdtextData->blocks[iBlock].songwriters != NULL &&
		    cdtextData->blocks[iBlock].songwriters[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "SONGWRITER \"%s\"\n",
				cdtextData->blocks[iBlock].songwriters[0]);
		    } else {
			fprintf(log, "REM SONGWRITER_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].songwriters[0]);
		    }
		}
		if (cdtextData->blocks[iBlock].titles != NULL &&
		    cdtextData->blocks[iBlock].titles[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "TITLE \"%s\"\n",
				cdtextData->blocks[iBlock].titles[0]);
		    } else {
			fprintf(log, "REM TITLE_%d \"%s\"\n", iBlock,
				cdtextData->blocks[iBlock].titles[0]);
		    }
		}
		* Ignore TOC_INFO. *
		* Ignore TOC_INFO2 (for now). *
		if (cdtextData->blocks[iBlock].upc_ean_isrcs != NULL &&
		    cdtextData->blocks[iBlock].upc_ean_isrcs[0] != NULL) {
		    if (iBlock == 0) {
			fprintf(log, "REM CATALOG %s\n",
				cdtextData->blocks[iBlock].upc_ean_isrcs[0]);
		    } else {
			fprintf(log, "REM CATALOG_%d %s\n", iBlock,
				cdtextData->blocks[iBlock].upc_ean_isrcs[0]);
		    }
		}
		* We ARE, however, interested in the sizeInfo. *
		if (iBlock == 0 && cdtextData->blocks[iBlock].bValid) {
		    switch (cdtextData->blocks[iBlock].charset) {
		    case CDROM_CD_TEXT_CHARSET_ISO8859_1:
			fprintf(log, "REM CHARSET ISO-8859-1\n");
			break;
		    case CDROM_CD_TEXT_CHARSET_ASCII:
			fprintf(log, "REM CHARSET ASCII\n");
			break;
		    case CDROM_CD_TEXT_CHARSET_MSJIS:
			fprintf(log, "REM CHARSET MS-JIS\n");
			break;
		    default:
			fprintf(log, "REM CHARSET 0x%02X\n",
				cdtextData->blocks[iBlock].charset);
			break;
		    }
		    fprintf(log, "REM LANGUAGE %s\n",
			    languageNames[cdtextData->blocks[iBlock].language]);
		    if (cdtextData->blocks[iBlock].bMessageCopyright ||
			cdtextData->blocks[iBlock].bNameCopyright ||
			cdtextData->blocks[iBlock].bTitleCopyright) {
			fprintf(log, "REM COPYRIGHT");
			if (cdtextData->blocks[iBlock].bTitleCopyright) {
			    fprintf(log, " TITLE");
			}
			if (cdtextData->blocks[iBlock].bNameCopyright) {
			    fprintf(log, " NAMES");
			}
			if (cdtextData->blocks[iBlock].bMessageCopyright) {
			    fprintf(log, " MESSAGE");
			}
			fprintf(log, "\n");
		    }
		} else if (cdtextData->blocks[iBlock].bValid) {
		    switch (cdtextData->blocks[iBlock].charset) {
		    case CDROM_CD_TEXT_CHARSET_ISO8859_1:
			fprintf(log, "REM CHARSET_%d ISO-8859-1\n", iBlock);
			break;
		    case CDROM_CD_TEXT_CHARSET_ASCII:
			fprintf(log, "REM CHARSET_%d ASCII\n", iBlock);
			break;
		    case CDROM_CD_TEXT_CHARSET_MSJIS:
			fprintf(log, "REM CHARSET_%d MS-JIS\n", iBlock);
			break;
		    default:
			fprintf(log, "REM CHARSET_%d 0x%02X\n", iBlock,
				cdtextData->blocks[iBlock].charset);
			break;
		    }
		    fprintf(log, "REM LANGUAGE_%d %s\n", iBlock,
			    languageNames[cdtextData->blocks[iBlock].language]);
		    if (cdtextData->blocks[iBlock].bMessageCopyright ||
			cdtextData->blocks[iBlock].bNameCopyright ||
			cdtextData->blocks[iBlock].bTitleCopyright) {
			fprintf(log, "REM COPYRIGHT_%d", iBlock);
			if (cdtextData->blocks[iBlock].bTitleCopyright) {
			    fprintf(log, " TITLE");
			}
			if (cdtextData->blocks[iBlock].bNameCopyright) {
			    fprintf(log, " NAMES");
			}
			if (cdtextData->blocks[iBlock].bMessageCopyright) {
			    fprintf(log, " MESSAGE");
			}
			fprintf(log, "\n");
		    }
		}
	    }
	}*/

	if (fulltoc != NULL) {
	    switch (cueify_full_toc_get_disc_type(fulltoc)) {
	    case CUEIFY_DISC_MODE_1:
		printf("REM ORIGINAL MEDIA-TYPE: CD\n");
		break;
	    case CUEIFY_DISC_CDI:
		printf("REM ORIGINAL MEDIA-TYPE: CD-I\n");
		break;
	    case CUEIFY_DISC_MODE_2:
		printf("REM ORIGINAL MEDIA-TYPE: CD-XA\n");
		break;
	    default:
		printf("REM ORIGINAL MEDIA-TYPE: UNKNOWN\n");
		break;
	    }
	}

	/* And lastly the track stuff. */
	printf("FILE \"disc.bin\" BINARY\n");
	for (i = cueify_toc_get_first_track(toc);
	     i <= cueify_toc_get_last_track(toc);
	     i++) {
	    if (fulltoc != NULL &&
		cur_session != cueify_full_toc_get_track_session(fulltoc, i)) {
		if (cur_session != 0) {
		    /* Print the leadout of the last session. */
		    offset =
			cueify_full_toc_get_session_leadout_address(
			    fulltoc, cur_session);

		    /* Adjust the lead-out. */
		    if (offset.sec < 2) {
			offset.min--;
			offset.sec += 60;
		    }
		    offset.sec -= 2;

		    printf("  REM LEAD-OUT %02d:%02d:%02d\n",
			   offset.min,
			   offset.sec,
			   offset.frm);
		}

		cur_session = cueify_full_toc_get_track_session(fulltoc, i);
		printf("  REM SESSION %02d\n",
		       cur_session);
	    }

	    if (cueify_toc_get_track_control_flags(toc, i) &
		CUEIFY_TOC_TRACK_IS_DATA) {
		/*
		if (trackMode == 0x10) {
		    * CD-I...  We special case. *
		    fprintf(log,
			    "    TRACK %02d CDI/2352\n",
			    iTrack);
		} else {
		    * Best way to detect the data mode is a raw read. *
		    raw = ReadRawSector(
			hDevice,
			toc.TrackData[iTrack - 1].Address[1] * 60 * 75 +
			toc.TrackData[iTrack - 1].Address[2] * 75 +
			toc.TrackData[iTrack - 1].Address[3],
			trackMode);
		    if (raw != NULL) {
			switch (raw[15]) {
			case 0x01:
		*/
			    printf("    TRACK %02d MODE1/2352\n",
				   i);
			    /*
			    break;
			case 0x02:
			    fprintf(log,
				    "    TRACK %02d MODE2/2352\n",
				    iTrack);
			    break;
			case 0x0F:
			    * Unknown *
			default:
			    fprintf(log,
				    "    TRACK %02d MODE1/2352\n",
				    iTrack);
			    break;
			}
			free(raw);
			raw = NULL;
		    } else {
			* Dunno :-/ *
			fprintf(log,
				"    TRACK %02d MODE1/2352\n",
				iTrack);
		    }
		}
			    */
	    } else {
		printf("    TRACK %02d AUDIO\n",
		       i);
	    }

	    /*
	    if (cdtextData != NULL) {
		for (iBlock = 0; iBlock < 8; iBlock++) {
		    if (cdtextData->blocks[iBlock].iTracks < iTrack) {
			continue;
		    }
		    
		    if (cdtextData->blocks[iBlock].arrangers != NULL &&
			cdtextData->blocks[iBlock].arrangers[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      REM ARRANGER \"%s\"\n",
				    cdtextData->blocks[iBlock].arrangers[iTrack]);
			} else {
			    fprintf(log, "      REM ARRANGER_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].arrangers[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].composers != NULL &&
			cdtextData->blocks[iBlock].composers[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      REM COMPOSER \"%s\"\n",
				    cdtextData->blocks[iBlock].composers[iTrack]);
			} else {
			    fprintf(log, "      REM COMPOSER_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].composers[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].messages != NULL &&
			cdtextData->blocks[iBlock].messages[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      REM MESSAGE \"%s\"\n",
				    cdtextData->blocks[iBlock].messages[iTrack]);
			} else {
			    fprintf(log, "      REM MESSAGE_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].messages[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].performers != NULL &&
			cdtextData->blocks[iBlock].performers[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      PERFORMER \"%s\"\n",
				    cdtextData->blocks[iBlock].performers[iTrack]);
			} else {
			    fprintf(log, "      REM PERFORMER_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].performers[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].songwriters != NULL &&
			cdtextData->blocks[iBlock].songwriters[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      SONGWRITER \"%s\"\n",
				    cdtextData->blocks[iBlock].songwriters[iTrack]);
			} else {
			    fprintf(log, "      REM SONGWRITER_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].songwriters[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].titles != NULL &&
			cdtextData->blocks[iBlock].titles[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      TITLE \"%s\"\n",
				    cdtextData->blocks[iBlock].titles[iTrack]);
			} else {
			    fprintf(log, "      REM TITLE_%d \"%s\"\n", iBlock,
				    cdtextData->blocks[iBlock].titles[iTrack]);
			}
		    }
		    if (cdtextData->blocks[iBlock].upc_ean_isrcs != NULL &&
			cdtextData->blocks[iBlock].upc_ean_isrcs[iTrack] != NULL) {
			if (iBlock == 0) {
			    fprintf(log, "      REM ISRC %s\n",
				    cdtextData->blocks[iBlock].upc_ean_isrcs[iTrack]);
			} else {
			    fprintf(log, "      REM ISRC_%d %s\n", iBlock,
				    cdtextData->blocks[iBlock].upc_ean_isrcs[iTrack]);
			}
		    }
		}
	    }
	    
	    if (ReadISRC(hDevice, iTrack, &data) && data.TrackIsrc.Tcval) {
		char szISRC[16] = "";
		memcpy(szISRC, data.TrackIsrc.TrackIsrc, 15);
		
		fprintf(log, "      ISRC %s\n", szISRC);
	    }
	    */

	    if ((cueify_toc_get_track_control_flags(toc, i) &
		 ~CUEIFY_TOC_TRACK_IS_DATA) != 0) {
		int control = cueify_toc_get_track_control_flags(toc, i);

		printf("      FLAGS");

		if ((control & CUEIFY_TOC_TRACK_HAS_PREEMPHASIS) > 0) {
		    printf(" PRE");
		}
		if ((control & CUEIFY_TOC_TRACK_PERMITS_COPYING) > 0) {
		    printf(" DCP");
		}
		if ((control & CUEIFY_TOC_TRACK_IS_QUADRAPHONIC) > 0) {
		    printf(" 4CH");
		}
		printf("\n");
	    }

	    /*
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
	    */

	    offset = lba_to_msf(cueify_toc_get_track_address(toc, i));

	    printf("      INDEX 01 %02d:%02d:%02d\n",
		   offset.min,
		   offset.sec,
		   offset.frm);

	    /* Detect any other indices. *
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
	    */
	}

	offset = lba_to_msf(cueify_toc_get_disc_length(toc));

	printf("  REM LEAD-OUT %02d:%02d:%02d\n",
	       offset.min,
	       offset.sec,
	       offset.frm);

	/* And finally, we can dump out intervals from TOC_INFO2. *
	if (cdtextData != NULL) {
	    if (cdtextData->tocInfo2.iIntervals > 0) {
		fprintf(log, "REM INTERVALS:\n");
	    }
	    for (iBlock = 0;
		 iBlock < cdtextData->tocInfo2.iIntervals;
		 iBlock++) {
		fprintf(log,
			"  REM INTERVAL %d %02d:%02d:%02d-%02d:%02d:%02d\n",
			cdtextData->tocInfo2.intervals[iBlock].priorityNumber,
			cdtextData->tocInfo2.intervals[iBlock].start.M,
			cdtextData->tocInfo2.intervals[iBlock].start.S,
			cdtextData->tocInfo2.intervals[iBlock].start.F,
			cdtextData->tocInfo2.intervals[iBlock].end.M,
			cdtextData->tocInfo2.intervals[iBlock].end.S,
			cdtextData->tocInfo2.intervals[iBlock].end.F);
	    }
	}
	
	free(cdtext);
	FreeCDText(cdtextData);
	free(fulltoc);
*/
	if (fulltoc != NULL) {
	    cueify_full_toc_free(fulltoc);
	}
	cueify_toc_free(toc);
    error:
	if (cueify_device_close(dev) != CUEIFY_OK) {
	    cueify_device_free(dev);
	    return 1;
	}
    }
    cueify_device_free(dev);

    return 0;
}

int main(int argc, char *argv[]) {
    char *device;

    /* Just got two command-line arguments we expect: drive letter and log. */
    if (argc != 1 && argc != 2) {
	printf("Usage: cueify [DEVICE]\n");
	return 0;
    }

    if (argc == 2) {
	device = argv[1];
    } else {
	device = NULL;
    }

    if (print_cuesheet(device)) {
	printf("There was an issue reading the cuesheet!\n");
    }

    return 0;
}
