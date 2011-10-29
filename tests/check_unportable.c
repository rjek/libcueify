/* check_unportable.h - Unit tests for unportable libcueify APIs
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

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <libcueify/types.h>
#include <libcueify/error.h>
#include <libcueify/device.h>
#include <libcueify/toc.h>

/* Create a binary track descriptor from a TOC. */
#define TRACK_DESCRIPTOR(adr, ctrl, track, address) \
    0, (((adr & 0xF) << 4) | (ctrl & 0xF)), track, 0,			\
	(address >> 24), ((address >> 16) & 0xFF),			\
	((address >> 8) & 0xFF), (address & 0xFF)

uint8_t expected_toc[] = {
    ((8 * 14 + 2) >> 8), ((8 * 14 + 2) & 0xFF), 1, 13,
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 1, 0),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 2, 21445),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 3, 34557),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 4, 61903),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 5, 83000),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 6, 98620),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 7, 112124),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 8, 135655),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 9, 154145),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 10, 176766),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 11, 194590),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, 0, 12, 213436),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, CUEIFY_TOC_TRACK_IS_DATA, 13,
		     244076),
    TRACK_DESCRIPTOR(CUEIFY_SUB_Q_POSITION, CUEIFY_TOC_TRACK_IS_DATA, 0xAA,
		     258988)
};


cueify_device *dev;


void setup() {
    dev = cueify_device_new();
    fail_unless(dev != NULL, "Failed to create cueify_device");
    fail_unless(cueify_device_open(dev, NULL) == CUEIFY_OK,
		"Failed to open device");
}


void teardown() {
    fail_unless(cueify_device_close(dev) == CUEIFY_OK,
		"Failed to close device");
    cueify_device_free(dev);
}


START_TEST (test_toc)
{
    cueify_toc *toc;
    size_t size = sizeof(expected_toc);
    uint8_t buffer[sizeof(expected_toc)];

    toc = cueify_toc_new();
    fail_unless(toc != NULL, "Failed to create cueify_toc object");
    fail_unless(cueify_device_read_toc(dev, toc) == CUEIFY_OK,
		"Failed to read TOC from device");
    fail_unless(cueify_toc_serialize(toc, buffer, &size) == CUEIFY_OK,
		"Could not serialize TOC");
    fail_unless(size == sizeof(expected_toc), "TOC size incorrect");
    fail_unless(memcmp(buffer, expected_toc, sizeof(expected_toc)) == 0,
		"TOC incorrect");
    cueify_toc_free(toc);
}
END_TEST


Suite *toc_suite() {
    Suite *s = suite_create("unportable");
    TCase *tc_core = tcase_create("core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_toc);
    suite_add_tcase(s, tc_core);

    return s;
}


int main() {
    int number_failed;
    Suite *s = toc_suite();
    SRunner *sr = srunner_create(s);

    printf("NOTE: These tests are expected to fail except when (certain\n"
	   "      printings of) David Bowie's Heathen is present in the\n"
	   "      current computer's CD drive.\n\n");

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}