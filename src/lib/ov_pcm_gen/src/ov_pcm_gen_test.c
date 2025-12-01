/***

Copyright   2018,2020   German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-08-31

 **/
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ov_arch/ov_byteorder.h>

#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>
#include <ov_test/ov_test.h>

#include "ov_pcm_gen.c"
#include <ov_base/ov_random.h>

/*----------------------------------------------------------------------------*/

void init_io_pair(int *spair, FILE **file) {
  OV_ASSERT(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, spair));
  *file = fdopen(spair[0], "w");
  OV_ASSERT(0 != *file);
}

/*----------------------------------------------------------------------------*/

void close_io_pair(int *spair, FILE **file) {
  if (0 != *file) {
    fclose(*file);
    *file = 0;
  }
  close(spair[0]);
  close(spair[1]);
}

/*----------------------------------------------------------------------------*/

bool could_read_from_socket(int fd) {

  char buf = 0;
  return 0 < read(fd, &buf, 1);
}

/*----------------------------------------------------------------------------*/

void drain_socket(int fd) {
  char buf[512] = {0};
  while (0 < read(fd, &buf, sizeof(buf))) {
  };
}
/*----------------------------------------------------------------------------*/
int test_ov_pcm_gen_create() {

  ov_pcm_gen_sinusoids sinusoids = {
      .frequency_hertz = 3000,
      .wobble.period_secs = 10,
      .wobble.frequency_disp_hertz = 1500,
  };

  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen *pcm = 0;
  testrun(0 == ov_pcm_gen_create(-1, config, 0));
  testrun(0 == ov_pcm_gen_create(OV_SINUSOIDS, config, 0));
  pcm = ov_pcm_gen_create(OV_SINUSOIDS, config, &sinusoids);
  testrun(0 != pcm);

  testrun(0 != pcm->generate_frame);
  testrun(0 != pcm->free);

  testrun(0 == pcm->free(pcm));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_pcm_gen_free() {

  testrun(0 == ov_pcm_gen_free(0));

  ov_pcm_gen_sinusoids sinusoids = {
      .frequency_hertz = 3000,
      .wobble.period_secs = 10,
      .wobble.frequency_disp_hertz = 1500,
  };

  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen *pcm = 0;
  testrun(0 == ov_pcm_gen_create(-1, config, 0));
  testrun(0 == ov_pcm_gen_create(OV_SINUSOIDS, config, 0));
  pcm = ov_pcm_gen_create(OV_SINUSOIDS, config, &sinusoids);
  testrun(0 != pcm);

  pcm = ov_pcm_gen_free(pcm);

  testrun(0 == pcm);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_pcm_gen_generate_frame() {

  testrun(0 == ov_pcm_gen_generate_frame(0));

  /* Yields a frame length of 320 samples , 640 bytes */
  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen_white_noise wnoise = {
      .max_amplitude = ((uint16_t)INT16_MAX) + 1,
  };

  ov_pcm_gen *pcm = 0;
  pcm = ov_pcm_gen_create(OV_WHITE_NOISE, config, &wnoise);
  testrun(0 == pcm);

  // Valid amplitude
  wnoise.max_amplitude = 3172;

  pcm = ov_pcm_gen_create(OV_WHITE_NOISE, config, &wnoise);
  testrun(0 != pcm);

  ov_buffer *buffer = ov_pcm_gen_generate_frame(pcm);

  testrun(0 != buffer);

  buffer = ov_buffer_free(buffer);
  pcm = ov_pcm_gen_free(pcm);

  testrun(0 == buffer);
  testrun(0 == pcm);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_generate_sinusoids() {

  ov_pcm_gen_sinusoids sinusoids = {
      .frequency_hertz = 3000,
      .wobble.period_secs = 10,
      .wobble.frequency_disp_hertz = 1500,
  };

  /* Yields a frame length of 320 samples , 640 bytes */
  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen *pcm = 0;
  pcm = ov_pcm_gen_create(OV_SINUSOIDS, config, &sinusoids);
  testrun(0 != pcm);

  ov_buffer *buffer = buffer = pcm->generate_frame(pcm);

  testrun(0 != buffer);
  testrun(640 == buffer->length);

  double sample_double = 0;
  double average = 0;

  int16_t *b16 = (int16_t *)buffer->start;

  /* Check that the buffer has actually been allocated */
  for (size_t i = 0; i < buffer->length / 2; ++i) {

    sample_double = b16[i];
    average += sample_double;
  }

  fprintf(stderr, "Average of signal is %f\n", average / buffer->length);

  buffer = ov_buffer_free(buffer);

  testrun(0 == pcm->free(pcm));

  /*--------------------------------------------------------------------*/
  /* Try different config, produce several chunks */

  /* Yields a frame length of 896 samples , 1792 bytes */
  config = (ov_pcm_gen_config){
      .sample_rate_hertz = 44800,
      .frame_length_usecs = 20 * 1000,
  };

  pcm = ov_pcm_gen_create(OV_SINUSOIDS, config, &sinusoids);
  testrun(0 != pcm);

  /*--------------------------------------------------------------------*/

  for (size_t run = 0; 16 > run; ++run) {

    buffer = pcm->generate_frame(pcm);
    testrun(0 != buffer);

    testrun(1792 == buffer->length);

    double sample_double = 0;
    double average = 0;

    int16_t *b16 = (int16_t *)buffer->start;

    /* Check that the buffer has actually been allocated */
    for (size_t i = 0; i < buffer->length / 2; ++i) {

      sample_double = b16[i];
      average += sample_double;
    }

    fprintf(stderr, "Average of signal is %f\n", average / buffer->length);

    buffer = ov_buffer_free(buffer);
  }

  testrun(0 == buffer);

  testrun(0 == pcm->free(pcm));

  /*--------------------------------------------------------------------*/
  /* Try without wobbling */

  /* Yields a frame length of 2240 samples , 4480 bytes */
  config = (ov_pcm_gen_config){
      .sample_rate_hertz = 44800,
      .frame_length_usecs = 50 * 1000,
  };

  /* Turn off wobbling */
  /* Check caching */

  sinusoids.wobble.period_secs = 0;

  pcm = ov_pcm_gen_create(OV_SINUSOIDS, config, &sinusoids);
  testrun(0 != pcm);

  /*--------------------------------------------------------------------*/

  for (size_t run = 0; 16 > run; ++run) {
    buffer = pcm->generate_frame(pcm);
    testrun(0 != buffer);

    testrun(4480 == buffer->length);

    double sample_double = 0;
    double average = 0;

    int16_t *b16 = (int16_t *)buffer->start;

    /* Check that the buffer has actually been allocated */
    for (size_t i = 0; i < buffer->length / 2; ++i) {

      sample_double = b16[i];
      average += sample_double;
    }

    fprintf(stderr, "Average of signal is %f\n", average / buffer->length);

    testrun(0 == ov_buffer_free(buffer));
  }

  pcm = pcm->free(pcm);
  testrun(0 == pcm);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_generate_white_noise() {

  // 1st test: Invalid amplitude value (too high)

  ov_pcm_gen_white_noise wnoise = {
      .max_amplitude = ((uint16_t)INT16_MAX) + 1,
  };

  /* Yields a frame length of 320 samples , 640 bytes */
  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen *pcm = 0;
  pcm = ov_pcm_gen_create(OV_WHITE_NOISE, config, &wnoise);
  testrun(0 == pcm);

  // Valid amplitude
  wnoise.max_amplitude = 3172;

  pcm = ov_pcm_gen_create(OV_WHITE_NOISE, config, &wnoise);
  testrun(0 != pcm);

  ov_buffer *buffer = buffer = pcm->generate_frame(pcm);

  testrun(0 != buffer);
  testrun(640 == buffer->length);

  int16_t *samples = (int16_t *)buffer->start;

  bool all_zeros = true;

  for (size_t i = 0; i < buffer->length / sizeof(int16_t); ++i) {

    printf("%" PRIi16 " (max %" PRIu16 ") \n", samples[i],
           wnoise.max_amplitude);

    testrun(wnoise.max_amplitude >= samples[i]);

    if (0 != samples[i]) {
      all_zeros = false;
    }
  }

  testrun(!all_zeros);

  buffer = ov_buffer_free(buffer);
  testrun(0 == buffer);

  pcm = pcm->free(pcm);
  testrun(0 == pcm);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_generate_from_file() {

  /* Yields a frame length of 320 samples , 640 bytes */
  ov_pcm_gen_config config = {
      .sample_rate_hertz = 16000,
      .frame_length_usecs = 20 * 1000,
  };

  ov_pcm_gen *pcm = 0;
  testrun(0 == ov_pcm_gen_create(OV_FROM_FILE, config, 0));

  /* Check with non-exisisting file */
  char *file_path = 0;

  ov_utils_init_random_generator();

  testrun(ov_random_string((char **)&file_path, 41, 0));

  struct stat fstate = {0};

  if (0 == stat(file_path, &fstate)) {
    free(file_path);
    file_path = 0;
  }

  ov_pcm_gen_from_file from_file_config = {
      .file_name = file_path,
      .codec_config = 0,
  };

  /* Now, file_path is rather surely not existing */
  testrun(0 == ov_pcm_gen_create(OV_FROM_FILE, config, &from_file_config));

  free(file_path);
  file_path = 0;

  /*--------------------------------------------------------------------*/
  // Now create temp file and check that its content will be produced
  // by ov_pcm_gen
  /*--------------------------------------------------------------------*/

  const size_t FILE_LENGTH_SAMPLES = 292143;
  int8_t *file_content = 0;
  const size_t FILE_LENGTH_BYTES = FILE_LENGTH_SAMPLES * 2;

  testrun(ov_random_string((char **)&file_content, FILE_LENGTH_BYTES, 0));

  /* Write to temp file */

  file_path = calloc(1, sizeof(char) * PATH_MAX);
  const char file_path_pattern[PATH_MAX] = "/tmp/ov_pcm_genXXXXXX";

  memcpy(file_path, file_path_pattern, PATH_MAX);

  int fd = mkstemp(file_path);
  testrun(-1 < fd);

  int retval = write(fd, file_content, FILE_LENGTH_BYTES);
  testrun(retval > -1);
  testrun((unsigned)retval == FILE_LENGTH_BYTES);
  close(fd);

  /* As we write using the "PCM 16s BIG ENDIAN" codec,
   * let's transform our test data to big endian */

  testrun(ov_byteorder_to_big_endian_16_bit((int16_t *)file_content,
                                            FILE_LENGTH_SAMPLES));

  from_file_config.file_name = file_path;
  pcm = ov_pcm_gen_create(OV_FROM_FILE, config, &from_file_config);

  testrun(pcm);

  /* Produce enough frames that a wrap-around should occur */
  const size_t FRAMES_TO_DELIVER = FILE_LENGTH_BYTES / 640 + 21;

  ov_buffer *buffer = 0;

  size_t current_in_file_content = 0;

  for (size_t run = 0; FRAMES_TO_DELIVER > run; ++run) {

    buffer = pcm->generate_frame(pcm);
    testrun(0 != buffer);

    testrun(640 == buffer->length);

    int8_t *pcm_i8 = (int8_t *)buffer->start;
    for (size_t i = 0; buffer->length > i; ++i) {

      testrun(file_content[current_in_file_content] == pcm_i8[i]);

      ++current_in_file_content;

      if (FILE_LENGTH_BYTES <= current_in_file_content) {

        current_in_file_content = 0;
      }
    }

    buffer = ov_buffer_free(buffer);
    testrun(0 == buffer);
  }

  testrun(0 == pcm->free(pcm));

  free(file_content);
  file_content = 0;

  buffer = ov_buffer_free(buffer);

  unlink(file_path);

  free(file_path);
  file_path = 0;

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_pcm_gen_config_print() {

  int spair[] = {-1, -1};
  FILE *test_out = 0;

  ov_pcm_gen_config config = {0};

  testrun(!ov_pcm_gen_config_print(0, 0, 0));
  testrun(!ov_pcm_gen_config_print(0, &config, 0));
  testrun(!ov_pcm_gen_config_print(0, &config, 14));
  testrun(!ov_pcm_gen_config_print(0, &config, (size_t)INT_MAX + 1));

  init_io_pair(spair, &test_out);

  testrun(!ov_pcm_gen_config_print(test_out, 0, 0));
  testrun(ov_pcm_gen_config_print(test_out, &config, 0));
  fclose(test_out);
  test_out = 0;

  testrun(could_read_from_socket(spair[1]));
  close_io_pair(spair, &test_out);

  init_io_pair(spair, &test_out);

  testrun(ov_pcm_gen_config_print(test_out, &config, 14));
  fclose(test_out);
  test_out = 0;

  testrun(could_read_from_socket(spair[1]));
  close_io_pair(spair, &test_out);

  init_io_pair(spair, &test_out);

  testrun(!ov_pcm_gen_config_print(test_out, &config, (size_t)INT_MAX + 1));

  close_io_pair(spair, &test_out);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_pcm_gen", test_ov_pcm_gen_create, test_ov_pcm_gen_free,
            test_ov_pcm_gen_generate_frame, test_impl_generate_sinusoids,
            test_impl_generate_white_noise, test_impl_generate_from_file,
            test_ov_pcm_gen_config_print);

/*----------------------------------------------------------------------------*/
