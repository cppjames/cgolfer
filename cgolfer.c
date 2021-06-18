#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define check_ptr(ptr, ...) \
    do { if (!ptr) { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } } while (0)

void parse_cmdline_args(int count, char** args);
void add_test(const char* input, const char* output);
void test_all_of_length(size_t length);
void source_indices_to_text(const size_t* indices, char* text, size_t length);
void source_text_to_indices(const char* text, size_t* indices, size_t length);
void test_source(char* source);
bool run_test(size_t test);
bool are_files_equal(FILE* file1, FILE* file2);
void get_next_source(size_t* source, size_t length);
bool is_last_source(size_t* source, size_t length);

const char char_set[] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    " !\"#%&'()*+,-./:;<=>?[\\]^-{|}~"
};

const size_t char_set_length = sizeof char_set - 1;
size_t max_code_length = 1000;
size_t test_count = 0;
size_t start_length = 0;
const char* start_source = "";
bool verbose_mode = false;

int main(int argc, char** argv) {
    parse_cmdline_args(argc, argv);

    for (size_t code_len = start_length; code_len <= max_code_length; code_len++)
        test_all_of_length(code_len);

    exit(EXIT_SUCCESS);
}

void parse_cmdline_args(int count, char** args) {
    bool expecting_max_length = false;
    bool expecting_input = false;
    bool expecting_output = false;
    bool expecting_start = false;

    const char* test_input = NULL;

#define arg_fail(...) do { fprintf(stderr, __VA_ARGS__); exit(EXIT_FAILURE); } while (0)

    for (int arg_index = 1; arg_index < count; arg_index++) {
        const char* arg = args[arg_index];

        if (expecting_max_length) {
            max_code_length = atoi(arg);
            expecting_max_length = false;
        } else if (expecting_input) {
            test_input = arg;
            expecting_input = false;
            expecting_output = true;
        } else if (expecting_output) {
            add_test(test_input, arg);
            expecting_output = false;
        } else if (expecting_start) {
            start_length = strlen(arg);
            start_source = arg;
            expecting_start = false;
        } else if (arg[0] != '-') {
            arg_fail("Invalid argument: %s\n", arg);
        } else if (arg[1] == 'n') {
            expecting_max_length = true;
        } else if (arg[1] == 't') {
            expecting_input = true;
        } else if (arg[1] == 's') {
            expecting_start = true;
        } else if (arg[1] == 'v') {
            verbose_mode = true;
        } else
            arg_fail("Invalid argument: %s\n", arg);
    }

    if (expecting_max_length) arg_fail("Length for code length parameter not specified.\n");
    if (expecting_input) arg_fail("Input and output for last test not specified.\n");
    if (expecting_output) arg_fail("Output for last test not specified.\n");
    if (expecting_start) arg_fail("Starting point parameter not specified.\n");
}

void add_test(const char* input, const char* output) {
    int index_len = snprintf(NULL, 0, "%zu", test_count);
    
    // /tmp/test<>_in
    char* input_filename = calloc(index_len + 13, 1);
    sprintf(input_filename, "/tmp/test%zu_in", test_count);
    FILE* input_file = fopen(input_filename, "wb+");

    check_ptr(input_file, "Could not input open file for test case %zu.\n", test_count);

    fwrite(input, 1, strlen(input), input_file);
    fclose(input_file);
    free(input_filename);

    // /tmp/test<>_out
    char* output_filename = calloc(index_len + 14, 1);
    sprintf(output_filename, "/tmp/test%zu_out", test_count);
    FILE* output_file = fopen(output_filename, "wb+");

    check_ptr(output_file, "Could not output open file for test case %zu.\n", test_count);

    fwrite(output, 1, strlen(output), output_file);
    fclose(output_file);
    free(output_filename);

    ++test_count;
}

void test_all_of_length(size_t length) {
    size_t* source_indices = calloc(length, sizeof(size_t));
    char* source_text = calloc(length + 1, 1);

    if (length == start_length)
        source_text_to_indices(start_source, source_indices, length);

    while (true) {
        source_indices_to_text(source_indices, source_text, length);
        test_source(source_text);
        get_next_source(source_indices, length);
        if (is_last_source(source_indices, length))
            break;
    }
}

void source_indices_to_text(const size_t* indices, char* text, size_t length) {
    for (size_t i = 0; i < length; i++)
        text[i] = char_set[indices[i]];
}

void source_text_to_indices(const char* text, size_t* indices, size_t length) {
    for (size_t i = 0; i < length; i++) {
        for (size_t ch = 0; ch < char_set_length; ch++) {
            if (char_set[ch] == text[i]) {
                indices[i] = ch;
                break;
            }
        }
    }
}

void test_source(char* source) {
    const char* filename = "/tmp/test_source.c";
    FILE* source_file = fopen(filename, "wb+");

    check_ptr(source_file, "Could not output open source file.\n");

    fwrite(source, 1, strlen(source), source_file);
    fclose(source_file);

    FILE* gcc_process = popen("gcc /tmp/test_source.c -o /tmp/test_program.out 2> /dev/null", "r");

    check_ptr(gcc_process, "Could not run the compiler.\n");

    int exit_code = pclose(gcc_process);
    if (!exit_code) {
        size_t successful_tests = 0;

        for (size_t i = 0; i < test_count; i++)
            successful_tests += run_test(i);

        if (successful_tests == test_count) {
            printf("%s\n", source);
            exit(EXIT_SUCCESS);
        } else if (verbose_mode)
            printf("[  Not Passing  ] %s\n", source);
    } else if (verbose_mode)
        printf("[ Compile Error ] %s\n", source);
}

bool run_test(size_t test) {
    const char* command_format = "/tmp/test_program.out < /tmp/test%zu_in > /tmp/test_program_output";
    int command_size = snprintf(NULL, 0, command_format, test);
    char* command = calloc(command_size, 1);
    sprintf(command, command_format, test);

    FILE* test_process = popen(command, "r");
    check_ptr(test_process, "Could not run the compiled program.\n");
    pclose(test_process);

    int index_len = snprintf(NULL, 0, "%zu", test);
    char* test_output_filename = calloc(index_len + 14, 1);
    sprintf(test_output_filename, "/tmp/test%zu_out", test);

    FILE* test_output_file = fopen(test_output_filename, "rb+");
    FILE* program_output_file = fopen("/tmp/test_program_output", "rb+");

    check_ptr(test_output_file, "Could not open the test's output file: %s\n", test_output_filename);
    check_ptr(program_output_file, "Could not open the compiled program's output file.\n");

    bool is_successful = are_files_equal(test_output_file, program_output_file);

    fclose(test_output_file);
    fclose(program_output_file);

    free(command);
    free(test_output_file);

    return is_successful;
}

bool are_files_equal(FILE* file1, FILE* file2) {
    char ch1 = getc(file1);
    char ch2 = getc(file2);

    while (ch1 != EOF && ch2 != EOF) {
        if (ch1 != ch2)
            return false;
        
        ch1 = getc(file1);
        ch2 = getc(file2);
    }

    return ch1 == ch2;
}

void get_next_source(size_t* source, size_t length) {
    bool carry = true;
    size_t i = length - 1;
    
    while (carry) {
        ++source[i];

        if (source[i] == char_set_length)
            source[i] = 0;
        else
            carry = false;

        if (i == 0)
            break;
        
        --i;
    }
}

bool is_last_source(size_t* source, size_t length) {
    for (size_t i = 0; i < length; i++)
        if (source[i])
            return false;

    return true;
}