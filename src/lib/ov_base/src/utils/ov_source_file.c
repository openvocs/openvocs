/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_source_file.c
        @author         Markus Toepfer

        @date           2019-02-10

        ------------------------------------------------------------------------
*/

#include "../../include/ov_source_file.h"
#include "../../include/ov_time.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>

#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

#include <getopt.h>

#include <sys/stat.h>
#include <sys/types.h>

#define COPYRIGHT_PREFIX                                                       \
    "/***\n"                                                                   \
    "        "                                                                 \
    "--------------------------------------------------------------------"     \
    "----\n"                                                                   \
    "\n"

#define COPYRIGHT_SUFFIX                                                       \
    "\n"                                                                       \
    "        "                                                                 \
    "--------------------------------------------------------------------"     \
    "----\n"                                                                   \
    "*/"
#define DOCUMENTATION_PREFIX "/**\n"

#define DOCUMENTATION_SUFFIX                                                   \
    "\n"                                                                       \
    "        "                                                                 \
    "--------------------------------------------------------------------"     \
    "----\n"                                                                   \
    "*/"

/*----------------------------------------------------------------------------*/

static bool path_is_project_top_dir(char const *const path) {

    if (!path)
        goto error;

    char include[PATH_MAX] = {0};
    char source[PATH_MAX] = {0};

    ssize_t bytes = snprintf(include, PATH_MAX, "%s/%s", path,
                             OV_SOURCE_FILE_FOLDER_INCLUDE);

    if (bytes < 0 || bytes == PATH_MAX)
        goto error;

    bytes =
        snprintf(source, PATH_MAX, "%s/%s", path, OV_SOURCE_FILE_FOLDER_SOURCE);

    if (bytes < 0 || bytes == PATH_MAX)
        goto error;

    struct stat stat_include = {0};
    struct stat stat_source = {0};

    if (0 != stat(include, &stat_include))
        goto error;

    if (0 != stat(source, &stat_source))
        goto error;

    return (S_ISDIR(stat_include.st_mode) && S_ISDIR(stat_source.st_mode));
error:
    return false;
}

/*----------------------------------------------------------------------------*/

char *ov_source_file_search_project_path(const char *input) {

    char *parent = NULL;
    char *current = NULL;

    if (!input)
        return NULL;

    char path[PATH_MAX];

    current = realpath(input, current);
    if (!current)
        goto error;

    if (!path_is_project_top_dir(current)) {

        /* walk up one level */
        sprintf(path, "%s/%s/", current, "..");
        parent = realpath(path, parent);
        if (!parent)
            goto error;

        if (strcmp(current, parent) == 0) {

            goto error;

        } else {

            free(current);
            current = ov_source_file_search_project_path(parent);

            free(parent);
            parent = NULL;
        }
    }

    return current;

error:
    if (current)
        free(current);
    if (parent)
        free(parent);
    return NULL;
}

/*----------------------------------------------------------------------------*/

char *ov_source_file_insert_at_each_line(const char *text, const char *intro,
                                         const char *outro) {

    if (!text)
        return NULL;

    char *result = NULL;
    char *ptr = NULL;
    char *nxt = NULL;
    char *cur = NULL;

    size_t len = 0;
    size_t tlen = 0;
    size_t ilen = 0;
    size_t olen = 0;
    size_t lines = 0;
    size_t size = 0;

    if (text)
        tlen = strlen(text);

    if (intro)
        ilen = strlen(intro);

    if (outro)
        olen = strlen(outro);

    cur = (char *)text;
    nxt = memchr(cur, '\n', tlen);
    while (nxt) {

        lines++;
        cur = nxt + 1;
        nxt = memchr(cur, '\n', tlen - (cur - text));
    }

    if (0 == lines)
        goto error;

    size = (lines * ilen) + (lines * olen) + tlen + 1;
    result = calloc(size, sizeof(char));
    if (!result)
        goto error;

    ptr = result;
    cur = (char *)text;
    nxt = memchr(cur, '\n', tlen);

    while (nxt) {

        len = nxt - cur;

        if (len > 0) {

            if (!strncat(ptr, intro, ilen))
                goto error;

            ptr += ilen;

            if (!strncat(ptr, cur, len))
                goto error;

            ptr += len;

            if (!strncat(ptr, outro, olen))
                goto error;

            ptr += olen;
        }

        if (!strcat(ptr, "\n"))
            goto error;

        ptr++;

        cur = nxt + 1;
        nxt = memchr(cur, '\n', tlen - (cur - text));
    }

    // copy rest after last linebreak
    len = tlen - (cur - text);
    if (len > 0)
        if (!strncat(ptr, cur, len))
            goto error;

    return result;

error:
    if (result)
        free(result);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_source_file_get_git_author(char *buffer, size_t size) {

    if (!buffer || size < 1)
        return false;

    bool set = false;
    FILE *in;

    if ((in = popen("git config user.name", "r")) != NULL) {

        if (fgets(buffer, size, in) != NULL) {

            fprintf(stdout, "... using git username %s as AUTHOR\n", buffer);
            set = true;
        }
        pclose(in);
    }

    if (!set) {

        // FALLBACK default AUTHOR TAG
        if (snprintf(buffer, size, "%s", OV_SOURCE_FILE_TAG_AUTHOR) < 0) {
            goto error;
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_path(const char *path) {

    if (!path)
        return false;

    size_t size = strlen(path);
    if (size > PATH_MAX)
        return false;

    bool created = false;

    DIR *dp;

    char *slash = NULL;
    char *pointer = NULL;
    size_t open = PATH_MAX;

    char new_path[open];
    memset(new_path, 0, open);

    pointer = (char *)path;
    while (size > (size_t)(pointer - path)) {

        slash = memchr(pointer, '/', size - (pointer - path));
        if (slash == path) {
            // creating absolute path
            pointer++;
            continue;
        }

        if (slash >= path + size)
            break;

        if (!slash) {

            if (0 > snprintf(new_path, size + 1, "%s", path))
                goto error;

        } else {

            if (0 > snprintf(new_path, (slash - path) + 1, "%s", path))
                goto error;
        }

        dp = opendir(new_path);
        if (dp) {
            // PATH exists access OK
            (void)closedir(dp);

        } else if (mkdir(new_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
                                       S_IXGRP | S_IROTH | S_IXOTH) != 0) {

            fprintf(stderr, "NO ACCESS TO PATH %s", new_path);
            goto error;

        } else {
            created = true;
        }

        if (slash) {
            pointer = slash + 1;
        } else {
            break;
        }
    }

    fprintf(stdout, "PATH OK %s (%s)\n", new_path,
            created ? "created" : "existing");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_file(const char *filename, const char *ext,
                        const char *content, const char *root_path,
                        const char *relative_path) {

    if (!filename || !content || !root_path)
        return false;

    FILE *file;

    int r;

    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);

    char file_name[PATH_MAX];
    memset(file_name, 0, PATH_MAX);

    if (ext) {
        snprintf(file_name, PATH_MAX, "%s%s", filename, ext);
    } else {
        strcat(file_name, filename);
    }

    if (relative_path) {

        if (snprintf(path, PATH_MAX, "%s/%s", root_path, relative_path) < 0)
            return false;
    } else {

        if (snprintf(path, PATH_MAX, "%s", root_path) < 0)
            return false;
    }

    // ensure create path
    if (!create_path(path))
        return false;

    // add file to path
    if (relative_path) {

        if (snprintf(path, PATH_MAX, "%s/%s/%s", root_path, relative_path,
                     file_name) < 0)
            return false;
    } else {

        if (snprintf(path, PATH_MAX, "%s/%s", root_path, file_name) < 0)
            return false;
    }

    if (access(path, F_OK) != -1) {
        fprintf(stderr, "File exists %s\n", path);
        return false;
    }

    file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Could not create/open file %s\n", path);
        return false;
    }

    r = fputs(content, file);
    if (r < 0)
        return false;

    fclose(file);

    fprintf(stdout, "Wrote file %s\n", path);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool create_c_header_file(const char *project_root_path,
                                 const ov_source_file_config *config,
                                 const char *copyright_header,
                                 const char *date) {

    if (!project_root_path || !config || !copyright_header || !date)
        return false;

    char content[5000];
    memset(content, 0, 5000);

    if (!snprintf(content, 5000,
                  "%s%s"
                  "        @file           %s.h\n"
                  "        @author         %s\n"
                  "        @date           %s\n"
                  "\n"
                  "%s\n"
                  "#ifndef %s_h\n"
                  "#define %s_h\n"
                  "\n"
                  "\n"
                  "/*\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " *\n"
                  " *      GENERIC FUNCTIONS\n"
                  " *\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " */\n"
                  "\n"
                  "\n"
                  "#endif /* %s_h */\n",
                  copyright_header, DOCUMENTATION_PREFIX, config->project.name,
                  config->copyright.author, date, DOCUMENTATION_SUFFIX,
                  config->project.name, config->project.name,
                  config->project.name))
        goto error;

    if (!create_file(config->project.name, ".h", content, project_root_path,
                     OV_SOURCE_FILE_FOLDER_INCLUDE))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_c_source_file(const char *project_root_path,
                                 const ov_source_file_config *config,
                                 const char *copyright_header,
                                 const char *date) {

    if (!project_root_path || !config || !copyright_header || !date)
        return false;

    char content[5000];
    memset(content, 0, 5000);

    if (!snprintf(content, 5000,
                  "%s%s"
                  "        @file           %s.c\n"
                  "        @author         %s\n"
                  "        @date           %s\n"
                  "\n"
                  "%s\n"
                  "#include \"%s%s/%s.h\"\n",
                  copyright_header, DOCUMENTATION_PREFIX, config->project.name,
                  config->copyright.author, date, DOCUMENTATION_SUFFIX, "../",
                  OV_SOURCE_FILE_FOLDER_INCLUDE, config->project.name))
        goto error;

    if (!create_file(config->project.name, ".c", content, project_root_path,
                     OV_SOURCE_FILE_FOLDER_SOURCE))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_c_test_file(const char *project_root_path,
                               const ov_source_file_config *config,
                               const char *copyright_header, const char *date) {

    if (!project_root_path || !config || !copyright_header || !date)
        return false;

    char content[5000];
    memset(content, 0, 5000);

    if (!snprintf(content, 5000,
                  "%s%s"
                  "        @file           %s_test.c\n"
                  "        @author         %s\n"
                  "        @date           %s\n"
                  "\n"
                  "%s\n"
                  "#include <ov_test/testrun.h>\n"
                  "#include \"%s.c\"\n"
                  "\n"
                  "/*\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " *\n"
                  " *      TEST CASES                                      "
                  "                #CASES\n"
                  " *\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " */\n"
                  "\n"
                  "int test_case(){\n"
                  "        testrun(1 == 1);\n"
                  "\n"
                  "        return testrun_log_success();\n"
                  "}\n"
                  "\n"
                  "/*------------------------------------------------------"
                  "----------------------*/\n"
                  "\n"
                  "/*\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " *\n"
                  " *      TEST CLUSTER                                    "
                  "                #CLUSTER\n"
                  " *\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " */\n"
                  "\n"
                  "int all_tests() {\n"
                  "\n"
                  "        testrun_init();\n"
                  "        testrun_test(test_case);\n"
                  "\n"
                  "        return testrun_counter;\n"
                  "}\n"
                  "\n"
                  "/*\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " *\n"
                  " *      TEST EXECUTION                                  "
                  "                #EXEC\n"
                  " *\n"
                  " *      "
                  "--------------------------------------------------------"
                  "----------------\n"
                  " */\n"
                  "\n"
                  "testrun_run(all_tests);\n",
                  copyright_header, DOCUMENTATION_PREFIX, config->project.name,
                  config->copyright.author, date, DOCUMENTATION_SUFFIX,
                  config->project.name))
        goto error;

    if (!create_file(config->project.name, "_test.c", content,
                     project_root_path, OV_SOURCE_FILE_FOLDER_SOURCE))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_makefile(const char *project_root_path,
                            const ov_source_file_config *config,
                            const char *copyright_header, const char *date) {

    if (!project_root_path || !config || !copyright_header || !date)
        return false;

    char content[10000];
    memset(content, 0, 10000);

    if (!snprintf(content, 10000,
                  "# -*- Makefile -*-\n"
                  "#       "
                  "--------------------------------------------------------"
                  "----------------\n"
                  "#\n"
                  "#       Copyright 2020 German Aerospace Center DLR e.V. "
                  "(GSOC)\n"
                  "#\n"
                  "#       Licensed under the Apache License, Version 2.0 "
                  "(the \"License\");\n"
                  "#       you may not use this file except in compliance "
                  "with the License.\n"
                  "#       You may obtain a copy of the License at\n"
                  "#\n"
                  "#               "
                  "http://www.apache.org/licenses/LICENSE-2.0\n"
                  "#\n"
                  "#       Unless required by applicable law or agreed to "
                  "in writing, software\n"
                  "#       distributed under the License is distributed on "
                  "an \"AS IS\" BASIS,\n"
                  "#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, "
                  "either express or implied.\n"
                  "#       See the License for the specific language "
                  "governing permissions and\n"
                  "#       limitations under the License.\n"
                  "#\n"
                  "#       This file is part of the openvocs project. "
                  "http://openvocs.org\n"
                  "#\n"
                  "#       "
                  "--------------------------------------------------------"
                  "----------------\n"
                  "#\n"
                  "#       Authors         Udo Haering, Michael J. Beer, "
                  "Markus Töpfer\n"
                  "#       Date            2020-01-21\n"
                  "#\n"
                  "#       "
                  "--------------------------------------------------------"
                  "----------------\n"
                  "\n"
                  "include $(OPENVOCS_ROOT)/makefile_const.mk\n"
                  "\n"
                  "OV_EXECUTABLE        = $(OV_BINDIR)/$(OV_DIRNAME)\n"
                  "OV_TARGET            = $(OV_EXECUTABLE)\n"
                  "\n"
                  "#-------------------------------------------------------"
                  "----------------------\n"
                  "\n"
                  "OV_FLAGS       =\n"
                  "\n"
                  "#-------------------------------------------------------"
                  "----------------------\n"
                  "\n"
                  "OV_LIBS        =\n"
                  "\n"
                  "#-------------------------------------------------------"
                  "----------------------\n"
                  "\n"
                  "include $(OPENVOCS_ROOT)/makefile_targets.mk\n"))
        goto error;

    if (!create_file("makefile", NULL, content, project_root_path, NULL))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool generate_project_root_path(ov_source_file_config *config,
                                       char *buffer, size_t size) {

    if (!config || !buffer)
        return false;

    if (!config->project.path || !config->project.name)
        return false;

    if (strlen(config->project.path) + strlen(config->project.name) + 2 > size)
        return false;

    if (!snprintf(buffer, size, "%s/%s", config->project.path,
                  config->project.name))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool create_module_files(ov_source_file_config *config, bool project) {

    char *date = ov_time_string(TIME_SCOPE_DAY);

    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);

    if (!config)
        return false;

    char *time_string = ov_time_string(TIME_SCOPE_YEAR);
    char *copyright_string = config->copyright.copyright.generate_header_string(
        COPYRIGHT_PREFIX, COPYRIGHT_DEFAULT_INTRO, time_string,
        config->copyright.owner, config->copyright.note, COPYRIGHT_SUFFIX, 8,
        true, config->copyright.gpl_parameter);

    free(time_string);

    if (!copyright_string)
        goto error;

    if (!create_c_header_file(config->project.path, config, copyright_string,
                              date))
        goto error;

    if (!create_c_source_file(config->project.path, config, copyright_string,
                              date))
        goto error;

    if (!create_c_test_file(config->project.path, config, copyright_string,
                            date))
        goto error;

    if (project) {

        if (!create_makefile(config->project.path, config, copyright_string,
                             date))
            goto error;

        if (!snprintf(path, PATH_MAX, "%s/copyright", config->project.path))
            goto error;

        if (!create_path(path))
            goto error;

        if (!create_file("copyright", NULL, copyright_string, path, NULL))
            goto error;

        if (config->copyright.copyright.generate_full_text_licence) {

            free(copyright_string);
            copyright_string =
                config->copyright.copyright.generate_full_text_licence(
                    &config->copyright.gpl_parameter);

            if (!copyright_string)
                goto error;

            if (!create_file("license.txt", NULL, copyright_string, path, NULL))
                goto error;
        }
    }

    free(copyright_string);
    free(date);
    return true;
error:
    if (date)
        free(date);
    if (copyright_string)
        free(copyright_string);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_source_file_create_source_files(ov_source_file_config *config) {

    if (!config)
        goto error;

    char *project_path =
        ov_source_file_search_project_path(config->project.path);
    if (!project_path) {
        fprintf(stderr,
                "\nFailed to identify a project root path for PATH "
                "%s\n",
                config->project.path);
        goto error;
    }

    fprintf(stdout, "Using project PATH %s\n", project_path);

    // config->project.name = basename(project_path);
    config->project.path = project_path;

    return create_module_files(config, false);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_source_file_create_project(ov_source_file_config *config) {

    if (!config)
        goto error;

    char project_root[PATH_MAX];
    memset(project_root, 0, PATH_MAX);

    if (!generate_project_root_path(config, project_root, PATH_MAX))
        goto error;

    if (!create_path(project_root))
        goto error;

    config->project.path = project_root;
    if (!create_module_files(config, true))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void print_usage(const char *name) {

    fprintf(stdout, "\n");
    fprintf(stdout, "USAGE          %s [OPTIONS]... [TARGET_NAME]\n", name);
    fprintf(stdout, "\n");
    fprintf(stdout,
            "EXAMPLES       %s --project --dir /home/project "
            "[TARGET_NAME]\n",
            name);
    fprintf(stdout, "               %s -pr [TARGET_NAME]\n", name);
    fprintf(stdout, "               %s [TARGET_NAME]\n", name);
    fprintf(stdout, "\n");
    fprintf(stdout, "OPTIONS        (GENERAL)\n");
    fprintf(stdout, "\n");
    fprintf(stdout,
            "               -p,     --project       create a new project "
            "instead of a module\n");
    fprintf(stdout,
            "               -n,     --name          define the target name "
            "explizit as argument\n");
    fprintf(stdout, "               -o,     --owner         define the target "
                    "owner explizit as argument\n");
    fprintf(stdout, "               -u,     --author        define an author "
                    "explizit as argument\n");
    fprintf(stdout,
            "               -x,     --note          define an copyright "
            "end note as argument\n");
    fprintf(stdout, "               -w,     --web           define a webpage "
                    "explizit as argument\n");
    fprintf(stdout,
            "               -d,     --dir           set the project's top "
            "dir (e.g. ~/home/projects)\n");
    fprintf(stdout,
            "               -v      --version       print the file version "
            "\n");
    fprintf(stdout,
            "               -h      --help          print this text \n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               (COPRIGHT)\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "               -r       --res           set copyright to "
                    "\"All rights reserved.\" \n");
    fprintf(stdout,
            "               -g       --gpl           set copyright to GPL "
            "v3 \n");
    fprintf(stdout, "               -a       --apache        set copyright to "
                    "APACHE v2 \n");
    fprintf(stdout,
            "               -b       --bsd           set copyright to BSD "
            "3Clause \n");
    fprintf(stdout,
            "               -m       --mit           set copyright to MIT "
            "\n");
    fprintf(stdout, "\n");
}

/*----------------------------------------------------------------------------*/

bool ov_source_file_read_user_input(int argc, char *argv[],
                                    ov_source_file_config *config,
                                    const char *app_name) {

    if (!config || !argv || (argc < 2))
        goto error;

    int c;
    int option_index = 0;

    static int flag_project = 0;

    static int flag_gpl = 0;
    static int flag_apache = 0;
    static int flag_bsd3 = 0;
    static int flag_mit = 0;
    static int flag_res = 0;

    config->project.path = NULL;
    config->project.name = NULL;
    config->project.url = NULL;

    if (!app_name)
        app_name = OV_SOURCE_FILE_NAME;

    char *string = NULL;

    while (1) {

        static struct option long_options[] = {

            /* These options set a flag. */

            /* Project  ... distinguish between project & module */
            {"project", no_argument, &flag_project, 1},
            {"module", no_argument, &flag_project, 0},

            /* Licence  ... only one MAY be set */
            {"gpl", no_argument, &flag_gpl, 1},
            {"apache", no_argument, &flag_apache, 1},
            {"bsd", no_argument, &flag_bsd3, 1},
            {"mit", no_argument, &flag_mit, 1},
            {"res", no_argument, &flag_res, 1},

            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"help", optional_argument, 0, 'h'},
            {"version", optional_argument, 0, 'v'},
            {"owner", required_argument, 0, 'o'},
            {"author", required_argument, 0, 'u'},
            {"note", required_argument, 0, 'x'},
            {"webpage", required_argument, 0, 'w'},
            {"name", required_argument, 0, 'n'},
            {"dir", required_argument, 0, 'd'},
            {0, 0, 0, 0}};

        /* getopt_long stores the option index here. */

        c = getopt_long(argc, argv, "n:o:u:x:w:n:d:?hpabgmr", long_options,
                        &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else
             * now. */
            if (long_options[option_index].flag != 0)
                break;

            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'h':
            print_usage(app_name);
            goto error;
            break;

        case '?':
            print_usage(app_name);
            goto error;
            break;

        case 'p':
            flag_project = 1;
            break;

        case 'n':
            printf("option -n (NAME of PROJECT) `%s'\n", optarg);
            config->project.name = optarg;
            break;

        case 'o':
            printf("option -o (OWNER of PROJECT) `%s'\n", optarg);
            config->copyright.owner = optarg;
            break;

        case 'u':
            printf("option -u (AUTHOR of PROJECT) `%s'\n", optarg);
            config->copyright.author = optarg;
            break;

        case 'x':
            printf("option -x (NOTE to PROJECT) `%s'\n", optarg);
            config->copyright.note = optarg;
            break;

        case 'w':
            printf("option -w (WEBPAGE of PROJECT) `%s'\n", optarg);
            config->project.url = optarg;
            break;

        case 'd':
            printf("option -d (DIR) `%s'\n", optarg);
            config->project.path = optarg;
            break;

        case 'v':
            OV_VERSION_PRINT(stdout);
            goto error;
            break;

        case 'a':
            flag_apache = 1;
            break;

        case 'b':
            flag_bsd3 = 1;
            break;

        case 'g':
            flag_gpl = 1;
            break;

        case 'm':
            flag_mit = 1;
            break;

        case 'r':
            flag_res = 1;
            break;

        default:
            print_usage(app_name);
            goto error;
        }
    }

    /* Validate input */

    fprintf(stdout, "Going to create a %s \n",
            (flag_project == 1) ? "PROJECT" : "MODULE");

    /* ... at max 1 copyright statement is selected */
    if ((flag_gpl + flag_apache + flag_mit + flag_res + flag_bsd3) > 1) {
        fprintf(stderr, "ERROR, multiple copyright statements selected\n");
        goto error;
    }

    /* ... a project name is set */
    if (config->project.name == NULL) {

        /* project name may be unspecified input parameter */
        if (optind < argc) {
            string = argv[optind++];
            fprintf(stdout, "Using name: %s\n", string);
            config->project.name = string;

        } else {
            fprintf(stderr, "ERROR, no name given, add -n \"name\" or "
                            "add a target_name as argument.\n");
            print_usage(app_name);
            goto error;
        }
    }

    fprintf(stdout, "... using name %s \n", config->project.name);

    /* ... set desired copyright */

    if (flag_gpl != 0) {

        struct ov_copyright_gpl_v3_parameter *parameter =
            calloc(1, sizeof(struct ov_copyright_gpl_v3_parameter));

        parameter->type = GENERAL;
        parameter->program_name = config->project.name;
        config->copyright.copyright = ov_copyright_gpl_version_3();
        config->copyright.gpl_parameter = parameter;

    } else if (flag_bsd3 != 0) {

        config->copyright.copyright = ov_copyright_bsd_3clause();

    } else if (flag_mit != 0) {

        config->copyright.copyright = ov_copyright_mit();

    } else if (flag_res != 0) {

        config->copyright.copyright = ov_copyright_reserved();

    } else {

        config->copyright.copyright = ov_copyright_apache_version_2();
    }

    if (flag_project == 1) {
        config->project.create = true;
    } else {
        config->project.create = false;
    }

    return true;
error:
    if (config)
        (*config) = (ov_source_file_config){0};
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_source_file_config_dump(FILE *stream,
                                const ov_source_file_config *config) {

    if (!stream || !config)
        goto error;

    if (!fprintf(stream,
                 "\n"
                 "CONFIG\n"
                 "\n"
                 "Project\n"
                 "   name:%s\n"
                 "   path:%s\n"
                 "    url:%s\n"
                 " create:%i\n"
                 "\n"
                 "Copyright\n"
                 "   author:%s\n"
                 "    owner:%s\n"
                 "     note:%s\n"
                 "\n",
                 config->project.name, config->project.path,
                 config->project.url, config->project.create,
                 config->copyright.author, config->copyright.owner,
                 config->copyright.note))
        goto error;

    return true;
error:
    return false;
}
