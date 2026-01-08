/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/testrun.h"
#include "ov_arch_math.c"

/*----------------------------------------------------------------------------*/

static int test_OV_MIN() {

  testrun(3 == OV_MIN(3, 4));
  testrun(3 == OV_MIN(4, 3));

  int x = 12;
  int y = 43;

  testrun(x == OV_MIN(x, y));
  testrun(x == OV_MIN(y, x));

  testrun(-4 == OV_MIN(-3, -4));
  testrun(-4 == OV_MIN(-4, -3));

  x = -12;
  y = -43;

  testrun(y == OV_MIN(x, y));
  testrun(y == OV_MIN(y, x));

  testrun(-4 == OV_MIN(3, -4));
  testrun(-4 == OV_MIN(-4, 3));

  x = -12;
  y = 43;

  testrun(x == OV_MIN(x, y));
  testrun(x == OV_MIN(y, x));

  size_t x_st = 12;
  size_t y_st = 43;

  testrun(x_st == OV_MIN(x_st, y_st));
  testrun(x_st == OV_MIN(y_st, x_st));

  size_t x_st_saved = x_st;

  /* Result should be x_st incremented ONCE */
  testrun(++x_st_saved == OV_MIN(++x_st, y_st));

  int x_saved = x;

  /* Result should be x_st incremented ONCE */
  testrun(++x_saved == OV_MIN(y, ++x));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_OV_MAX() {

  testrun(4 == OV_MAX(3, 4));
  testrun(4 == OV_MAX(4, 3));

  int x = 12;
  int y = 43;

  testrun(y == OV_MAX(x, y));
  testrun(y == OV_MAX(y, x));

  testrun(-3 == OV_MAX(-3, -4));
  testrun(-3 == OV_MAX(-4, -3));

  x = -12;
  y = -43;

  testrun(x == OV_MAX(x, y));
  testrun(x == OV_MAX(y, x));

  testrun(3 == OV_MAX(3, -4));
  testrun(3 == OV_MAX(-4, 3));

  x = -12;
  y = 43;

  testrun(y == OV_MAX(x, y));
  testrun(y == OV_MAX(y, x));

  size_t x_st = 12;
  size_t y_st = 43;

  testrun(y_st == OV_MAX(x_st, y_st));
  testrun(y_st == OV_MAX(y_st, x_st));

  size_t y_st_saved = y_st;

  /* Result should be x_st incremented ONCE */
  testrun(++y_st_saved == OV_MAX(++x_st, ++y_st));

  int y_saved = y;

  /* Result should be x_st incremented ONCE */
  testrun(++y_saved == OV_MAX(++y, ++x));

  // Check with char type
  signed char sc1 = 1;
  signed char sc2 = 2;

  testrun(sc2 == OV_MAX(sc1, sc2));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

RUN_TESTS("ov_arch_math", test_OV_MIN, test_OV_MAX);
