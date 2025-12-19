#include "string_match.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#ifdef USE_MPI
#include <mpi.h>
#endif

#define INITIAL_CAPACITY 100
#define MAX_LINE_LENGTH 4096

static int find_substring(const char *text, size_t text_len, const char *pattern, size_t pattern_len) {
    if (pattern_len == 0 || pattern_len > text_len) return 0;

    for (size_t i = 0; i <= text_len - pattern_len; i++) {
        if (memcmp(text + i, pattern, pattern_len) == 0) {
            return 1;
        }
    }
    return 0;
}

int count_matches_in_document(const char *filename, const char *pattern) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) return 0;
    
    int match_count = 0;
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        size_t line_len = strlen(line);

        for (size_t i = 0; i <= line_len - pattern_len; i++) {
            if (memcmp(line + i, pattern, pattern_len) == 0) {
                match_count++;
            }
        }
    }
    
    fclose(file);
    return match_count;
}

MatchResultSet *exact_match_serial(const char *pattern, const char *metadata_file) {
    MatchResultSet *result_set = malloc(sizeof(MatchResultSet));
    if (!result_set) return NULL;
    
    result_set->results = malloc(INITIAL_CAPACITY * sizeof(MatchResult));
    result_set->count = 0;
    result_set->capacity = INITIAL_CAPACITY;
    
    FILE *meta = fopen(metadata_file, "r");
    if (!meta) {
        free(result_set->results);
        free(result_set);
        return NULL;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), meta)) {
        int doc_id;
        char filename[256];
        if (sscanf(line, "%d %255s", &doc_id, filename) == 2) {
            int match_count = count_matches_in_document(filename, pattern);
            
            if (match_count > 0) {
                if (result_set->count >= result_set->capacity) {
                    result_set->capacity *= 2;
                    result_set->results = realloc(result_set->results, result_set->capacity * sizeof(MatchResult)); // we got this from https://linux.die.net/man/3/realloc
                }
                
                result_set->results[result_set->count].doc_id = doc_id;
                result_set->results[result_set->count].match_count = match_count;
                result_set->results[result_set->count].line_numbers = NULL;
                result_set->results[result_set->count].line_count = 0;
                result_set->count++;
            }
        }
    }
    
    fclose(meta);
    return result_set;
}

MatchResultSet *exact_match_mpi(const char *pattern, const char *metadata_file, int argc, char **argv) {
#ifdef USE_MPI
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    FILE *meta = fopen(metadata_file, "r");
    if (!meta) {
        if (rank == 0) {
            fprintf(stderr, "Error: Cannot open metadata file: %s\n", metadata_file);
        }
        return NULL;
    }
    
    typedef struct {
        int doc_id;
        char filename[256];
    } document_info;
    
    document_info *all_docs = malloc(INITIAL_CAPACITY * sizeof(document_info));
    int doc_count = 0;
    int capacity = INITIAL_CAPACITY;
    char line[512];
    
    while (fgets(line, sizeof(line), meta)) {
        if (doc_count >= capacity) {
            capacity *= 2;
            all_docs = realloc(all_docs, capacity * sizeof(document_info));
        }
        if (sscanf(line, "%d %255s", &all_docs[doc_count].doc_id, all_docs[doc_count].filename) == 2) {
            doc_count++;
        }
    }
    fclose(meta);
    
    int docs_per_process = doc_count / size;
    int remainder = doc_count % size;
    int my_start = rank * docs_per_process + (rank < remainder ? rank : remainder);
    int my_end = my_start + docs_per_process + (rank < remainder ? 1 : 0);
    int my_doc_count = my_end - my_start;
    
    int *match_counts = calloc(my_doc_count, sizeof(int));
    
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < my_doc_count; i++) {
        int global_idx = my_start + i;
        match_counts[i] = count_matches_in_document(all_docs[global_idx].filename, pattern);
    }
    
    MatchResult *local_results = malloc(my_doc_count * sizeof(MatchResult));
    int local_count = 0;
    for (int i = 0; i < my_doc_count; i++) {
        if (match_counts[i] > 0) {
            local_results[local_count].doc_id = all_docs[my_start + i].doc_id;
            local_results[local_count].match_count = match_counts[i];
            local_results[local_count].line_numbers = NULL;
            local_results[local_count].line_count = 0;
            local_count++;
        }
    }
    
    int *result_counts = NULL;
    int *displacements = NULL;
    if (rank == 0) {
        result_counts = malloc(size * sizeof(int));
        displacements = malloc(size * sizeof(int));
    }
    
    MPI_Gather(&local_count, 1, MPI_INT, result_counts, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    int total_results = 0;
    if (rank == 0) {
        displacements[0] = 0;
        for (int i = 0; i < size; i++) {
            if (i > 0) {
                displacements[i] = displacements[i-1] + result_counts[i-1];
            }
            total_results += result_counts[i];
        }
    }
    MatchResult *all_results = NULL;
    if (rank == 0) {
        all_results = malloc(total_results * sizeof(MatchResult));
    }
    
    int *local_doc_ids = malloc(local_count * sizeof(int));
    int *local_match_counts = malloc(local_count * sizeof(int));
    for (int i = 0; i < local_count; i++) {
        local_doc_ids[i] = local_results[i].doc_id;
        local_match_counts[i] = local_results[i].match_count;
    }
    
    int *all_doc_ids = NULL;
    int *all_match_counts = NULL;
    if (rank == 0) {
        all_doc_ids = malloc(total_results * sizeof(int));
        all_match_counts = malloc(total_results * sizeof(int));
    }
    
    MPI_Gatherv(local_doc_ids, local_count, MPI_INT, 
                all_doc_ids, result_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gatherv(local_match_counts, local_count, MPI_INT,
                all_match_counts, result_counts, displacements, MPI_INT, 0, MPI_COMM_WORLD);
    
    MatchResultSet *result_set = NULL;
    if (rank == 0) {
        result_set = malloc(sizeof(MatchResultSet));
        result_set->results = malloc(total_results * sizeof(MatchResult));
        result_set->count = total_results;
        result_set->capacity = total_results;
        
        for (int i = 0; i < total_results; i++) {
            result_set->results[i].doc_id = all_doc_ids[i];
            result_set->results[i].match_count = all_match_counts[i];
            result_set->results[i].line_numbers = NULL;
            result_set->results[i].line_count = 0;
        }
        
        free(all_doc_ids);
        free(all_match_counts);
        free(result_counts);
        free(displacements);
    }
    
    free(all_docs);
    free(match_counts);
    free(local_results);
    free(local_doc_ids);
    free(local_match_counts);
    

    return result_set;
#else
    return NULL;
#endif
}

void free_match_results(MatchResultSet *results) {
    if (!results) return;
    
    for (size_t i = 0; i < results->count; i++) {
        if (results->results[i].line_numbers) {
            free(results->results[i].line_numbers);
        }
    }
    
    free(results->results);
    free(results);
}

