/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_random_test.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2019-08-02

        @ingroup        ov_random

        As the Monte Carlo Pi calculation shows,
        the libc library function random(3) and hence our random generators are
        BAD. Plotting the values shows clear patterns in the distribution.
        Don't use those random generators for serious stuff like
        ENCRYPTION!!!

        ------------------------------------------------------------------------
*/
#include "ov_random.c"
#include <ov_test/ov_test.h>

#include "../../include/ov_utils.h"
#include <ctype.h>
#include <math.h>

/*----------------------------------------------------------------------------*/

#define debug_out(msg, ...)
// #define debug_out(msg, ...) fprintf(stderr, msg, __VA_ARGS__)

/*****************************************************************************
                               Distribution test
 ****************************************************************************/

/**
 * The test is straight forward:
 *
 * We use the monte carlo method to calculate pi using our random numbers:
 *
 *
 * Take random pairs (x,y) with both x and y in range [a,b]
 *
 * Count how many of them lie inside the circle with radius b - a.
 *
 * If proper random values,
 * number_of_pairs_inside_circle / number_of_pairs_outside_circle should
 * be roughly equal to
 * area_in_circle / area_out_of_circle
 *
 * We renorm everything to the unit interval/square/circle.
 *
 * We use only positive integers here.
 *
 * All pairs (x,y) must lie in the unit square [0,1]x[0,1]
 * We count how many of those lie in the unit circle.
 *
 * Then, the ratio of pairs_in_circle/pairs_in_square should equal
 * the area_of_circle / area_of_square.
 * And that should approx
 *
 * pi / 4
 *
 * Area of the circle: pi * r * r  with r = 1 (unit circle !)
 * Area of the unit square: r * r = 1
 *
 * Area of a quarter of the circle: pi * r * r / 4
 * Area of a quarter of the entire unit square: r * r
 *
 * Ratio (pi * r * r / 4) / (r * r) = pi / 4
 */
static size_t inside_unit_circle = 0;
static size_t total_number_of_points = 0;

static void reset_circle_counters() {
  inside_unit_circle = 0;
  total_number_of_points = 0;
}

/*----------------------------------------------------------------------------*/

static bool check_in_or_out_of_circle(uint64_t x, uint64_t y,
                                      uint64_t interval_len) {

  OV_ASSERT(0 != interval_len);

  /*
   * Simple check:
   *
   * We renorm x and y to [0,1]
   *
   * If x*x + y*y < 1, it's inside unit circle, otherwise outside
   */

  double len = interval_len;
  double x_renorm = x;
  x_renorm = x_renorm / len;

  double y_renorm = y;
  y_renorm = y_renorm / len;

  debug_out("%lf   %lf\n", x_renorm, y_renorm);

  double x_renorm_square = x_renorm * x_renorm;
  double y_renorm_square = y_renorm * y_renorm;

  ++total_number_of_points;

  if (x_renorm_square + y_renorm_square < 1) {
    ++inside_unit_circle;
    return true;
  }

  return false;
}

/*----------------------------------------------------------------------------*/

static double calc_pi() {

  double pi_approx = inside_unit_circle;
  double total_number_of_pairs = total_number_of_points;

  debug_out("We got %lf points inside circle, %lf in total\n", pi_approx,
            total_number_of_pairs);

  pi_approx = pi_approx / total_number_of_pairs;

  return pi_approx;
}

/*----------------------------------------------------------------------------*/

static bool pi_is_close_enough(double pi_approx, double max_error) {

  pi_approx *= 4;
  bool close_enough = fabs(pi_approx - M_PI) < max_error;

  debug_out("Pi approximated is %lf, close enough: %s\n", pi_approx,
            close_enough ? "yes" : "no");

  return close_enough;
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_random_bytes() {

  size_t size = 1000000;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_random_bytes(NULL, 0));

  size_t sz = 1;
  testrun(ov_random_bytes(buffer, sz));

  for (size_t i = 0; i < sz; i++) {
    testrun(buffer[i] != 0);
  }

  for (size_t i = sz; i < size; i++) {
    testrun(buffer[i] == 0);
  }

  sz = 100;
  testrun(ov_random_bytes(buffer, sz));
  for (size_t i = 0; i < sz; i++) {
    testrun(buffer[i] != 0);
  }

  for (size_t i = sz; i < size; i++) {
    testrun(buffer[i] == 0);
  }

  testrun(ov_random_bytes(buffer, size));
  for (size_t i = 0; i < size; i++) {
    testrun(buffer[i] != 0);
  }

  // check randomness
  uint8_t *array[1000];
  srandom(ov_time_get_current_time_usecs());
  size_t bytes = 100;

  for (size_t i = 0; i < 1000; i++) {

    array[i] = calloc(bytes, sizeof(uint8_t));
    testrun(array[i]);
    memset(array[i], 0, bytes);
    testrun(ov_random_bytes(array[i], bytes));

    if (i == 0)
      continue;

    for (size_t n = 0; n < (i - 1); n++) {
      testrun(0 != memcmp(array[i], array[n], bytes));
    }
  }

  for (size_t i = 0; i < 1000; i++) {

    free(array[i]);
    array[i] = 0;
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_random_bytes_with_zeros() {

  size_t size = 1000000;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  testrun(!ov_random_bytes(NULL, 0));

  size_t sz = 1;
  testrun(ov_random_bytes(buffer, sz));

  for (size_t i = sz; i < size; i++) {
    testrun(buffer[i] == 0);
  }

  bool found = false;
  sz = 100;
  testrun(ov_random_bytes(buffer, sz));
  for (size_t i = 0; i < sz; i++) {
    if (buffer[i] != 0)
      found = true;
  }
  testrun(found);

  size_t count = 0;
  testrun(ov_random_bytes(buffer, size));
  for (size_t i = 0; i < size; i++) {
    if (buffer[i] != 0)
      count++;
  }
  testrun(count > size / 2);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_random_string() {

  char *buffer1 = 0;
  char *buffer2 = 0;

  int i, k = 0;
  int testValues = 200;
  int bufferLength = 10;

  char *array[testValues];
  memset(array, 0, testValues);

  // just a pretty basic test of this function
  // create an amount of testValues
  // bit check each value against each other
  // raise error if a value matches another value

  for (i = 0; i < testValues; i++) {

    buffer1 = calloc(bufferLength, sizeof(uint8_t));

    testrun(ov_random_string(&buffer1, bufferLength, 0));

    debug_out("Random String: %s\n", buffer1);

    array[i] = buffer1;
  }

  for (i = 0; i < testValues; i++) {

    buffer1 = array[i];

    for (k = 0; k < testValues; k++) {

      if (k == i)
        continue;

      buffer2 = array[k];
      testrun(strncmp(buffer1, buffer2, bufferLength) != 0);
    }
  }

  for (i = 0; i < testValues; i++) {
    buffer1 = array[i];
    free(buffer1);
  }

  /* Check again with custom alphabet */

  const char test_alphabet[] = "12aCe3";

  for (i = 0; i < testValues; i++) {

    buffer1 = calloc(bufferLength, sizeof(uint8_t));

    testrun(ov_random_string(&buffer1, bufferLength, test_alphabet));

    array[i] = buffer1;
  }

  for (i = 0; i < testValues; i++) {

    buffer1 = array[i];

    char *ptr = buffer1;

    while (0 != *ptr) {

      testrun(0 != strchr(test_alphabet, *ptr));
      ++ptr;
    };

    for (k = 0; k < testValues; k++) {

      if (k == i)
        continue;

      buffer2 = array[k];
      testrun(strncmp(buffer1, buffer2, bufferLength) != 0);
    }
  }

  for (i = 0; i < testValues; i++) {
    buffer1 = array[i];
    free(buffer1);
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_random_uint32() {

  // check randomness
  uint32_t array[10000];

  const size_t len = sizeof(array) / sizeof(array[0]);

  size_t doubles = 0;

  for (size_t i = 0; i < len; i++) {

    usleep(1000);
    array[i] = ov_random_uint32();

    for (size_t n = 0; n < i; n++) {

      if (array[i] == array[n])
        doubles++;
    }
  }

  testrun(doubles < 10);

  for (size_t i = 0; i < 100000; ++i) {
    check_in_or_out_of_circle(ov_random_uint32(), ov_random_uint32(),
                              UINT32_MAX);
  }

  double pi_approx = calc_pi();

  testrun(pi_is_close_enough(pi_approx, 0.2));

  reset_circle_counters();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_random_uint64() {

  // check randomness
  uint64_t array[10000];

  const size_t len = sizeof(array) / sizeof(array[0]);

  size_t doubles = 0;

  for (size_t i = 0; i < len; i++) {

    usleep(10);
    array[i] = ov_random_uint64();

    for (size_t n = 0; n < i; n++) {

      if (array[i] == array[n]) {
        doubles++;
      }
    }
  }

  testrun_log("doubles %zu\n", doubles);

  testrun(doubles < 10);

  for (size_t i = 0; i < 100000; ++i) {
    check_in_or_out_of_circle(ov_random_uint64(), ov_random_uint64(),
                              UINT64_MAX);
  }

  double pi_approx = calc_pi();

  testrun(pi_is_close_enough(pi_approx, 0.2));

  reset_circle_counters();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_random_range() {

  uint64_t out = ov_random_range(0, 0);
  testrun(out > 0);
  out = ov_random_range(1000, 0);
  testrun(out >= 1000);
  out = ov_random_range(0, 1000);
  testrun(out <= 1000);
  out = ov_random_range(500, 1000);
  testrun(out <= 1000);
  testrun(out >= 500);

  out = ov_random_range(500, 500);
  testrun(out <= 500);
  testrun(out >= 500);

  out = ov_random_range(500, 501);
  testrun(out <= 501);
  testrun(out >= 500);

  out = ov_random_range(100000, 100001);
  testrun(out <= 100001);
  testrun(out >= 100000);

  out = ov_random_range(100, 100001);
  testrun(out <= 100001);
  testrun(out >= 100);

  for (uint64_t i = 1; i < 10000; i++) {

    out = ov_random_range(0, i);
    testrun(out <= i);
    out = ov_random_range(i, 0);
    testrun(out >= i);
    out = ov_random_range(i / 2, i);
    testrun(out <= i);
    testrun(out >= i / 2);
  }

  for (size_t i = 0; i < 100000; ++i) {
    check_in_or_out_of_circle(ov_random_range(14, 829),
                              ov_random_range(14, 829), 829 - 14);
  }

  double pi_approx = calc_pi();

  testrun(pi_is_close_enough(pi_approx, 0.2));

  reset_circle_counters();

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_random_gaussian() {

  testrun(!ov_random_gaussian(0, 0));

  double r1 = 0.0;

  testrun(ov_random_gaussian(&r1, 0));

  fprintf(stderr, "Gaussian: %f\n", r1);

  double r2 = 0;

  testrun(ov_random_gaussian(&r1, &r2));
  testrun(ov_random_gaussian(0, &r2));

  const double interval = 3.0;
  const size_t buckets = 100;
  size_t hist[buckets];
  memset(hist, 0, buckets * sizeof(size_t));

  const double bucket_size = interval / (double)buckets;

  // Calculate some random numbers and plot the histogram
  for (size_t n = 0; n < 100000; ++n) {

    double r1 = 0;
    double r2 = 0;

    testrun(ov_random_gaussian(&r1, &r2));

    r1 = fabs(r1);
    r2 = fabs(r2);

    if (interval > r1) {
      size_t n = (size_t)(r1 / bucket_size);
      OV_ASSERT(n < buckets);
      ++hist[n];
    }

    if (interval > fabs(r2)) {
      size_t n = (size_t)(r2 / bucket_size);
      OV_ASSERT(n < buckets);
      ++hist[n];
    }
  }

  double bucket_val = 0.0;

  fprintf(stderr, "Buckets\n");

  for (size_t n = 0; n < buckets; ++n, bucket_val += bucket_size) {

    fprintf(stderr, "%f   %zu\n", bucket_val, hist[n]);
  }

  fprintf(stderr, "\n");

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_random", test_ov_random_bytes_with_zeros, test_ov_random_bytes,
            test_ov_random_string, test_ov_random_uint32, test_ov_random_uint64,
            test_ov_random_range, test_ov_random_gaussian);
