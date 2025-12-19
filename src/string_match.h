#ifndef STRING_MATCH_H
#define STRING_MATCH_H

#include <stddef.h>

typedef struct {
    int doc_id;
    int match_count;
    int *line_numbers;
    size_t line_count;
} MatchResult;

typedef struct {
    MatchResult *results;
    size_t count;
    size_t capacity;
} MatchResultSet;

MatchResultSet *exact_match_serial(const char *pattern, const char *metadata_file);

MatchResultSet *exact_match_mpi(const char *pattern, const char *metadata_file, int argc, char **argv);

void free_match_results(MatchResultSet *results);

int count_matches_in_document(const char *filename, const char *pattern);

#endif


