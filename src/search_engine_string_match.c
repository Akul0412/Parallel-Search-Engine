#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "string_match.h"
#ifdef USE_MPI
#include <mpi.h>
#endif

int main(int argc, char *argv[]) {
#ifdef USE_MPI
    MPI_Init(&argc, &argv);
#endif
    if (argc == 2 && (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--interactive") == 0)) {
        char metadata_file[256] = "documents/wikitext_metadata.txt";
        char pattern[1024];
        char mode_choice[10];
        
        printf("=== Interactive String Matching Test ===\n\n");
        printf("Enter metadata file [%s]: ", metadata_file);
        fflush(stdout);
        char input[256];
        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\n")] = 0;
            if (strlen(input) > 0) {
                strncpy(metadata_file, input, sizeof(metadata_file) - 1);
                metadata_file[sizeof(metadata_file) - 1] = '\0';
            }
        }
        
        while (1) {
            fflush(stdout);
            
            if (!fgets(pattern, sizeof(pattern), stdin)) break;
            pattern[strcspn(pattern, "\n")] = 0; 
            
            if (strcmp(pattern, "quit") == 0 || strcmp(pattern, "exit") == 0 || strcmp(pattern, "q") == 0) {
                break;
            }
            
            if (strlen(pattern) == 0) continue;
            
            printf("Mode [serial/mpi] (default: mpi): ");
            fflush(stdout);
            if (!fgets(mode_choice, sizeof(mode_choice), stdin)) break;
            mode_choice[strcspn(mode_choice, "\n")] = 0;
            
            const char *mode;
            if (strlen(mode_choice) == 0 || strcmp(mode_choice, "mpi") == 0) {
                mode = "mpi";
            } else {
                mode = "serial";
            }

            FILE *test_file = fopen(metadata_file, "r");
            if (!test_file) {
                continue;
            }
            fclose(test_file);
            
            printf("\n");
            double start_time = omp_get_wtime();
            MatchResultSet *results = NULL;
            
            if (strcmp(mode, "serial") == 0) {
                printf("Running serial exact string matching...\n");
                results = exact_match_serial(pattern, metadata_file);
            } else {
                results = exact_match_mpi(pattern, metadata_file, argc, argv);
            }
            
            double end_time = omp_get_wtime();
            double elapsed = (end_time - start_time) * 1000.0;
            
            if (!results) {
                printf("Error: Failed to perform search\n");
                printf("  Check that metadata file exists: %s\n", metadata_file);
                printf("  Check that pattern is not empty\n");
                continue;
            }
            
            printf("\nPattern: \"%s\"\n", pattern);
            printf("Execution time: %.2f ms\n", elapsed);
            printf("Documents with matches: %zu\n\n", results->count);
            
            int display_count = results->count < 10 ? results->count : 10;
            for (int i = 0; i < display_count; i++) {
                printf("%d. Document ID: %d, Matches: %d\n",
                       i + 1, results->results[i].doc_id, results->results[i].match_count);
            }
            
            if (results->count > 10) {
                printf("... and %zu more documents\n", results->count - 10);
            }
            
            free_match_results(results);
        }
        return 0;
    }
    
    if (argc < 4) {
        printf("Error: Not enough arguments\n");
        return 1;
    }
    
    const char *mode = argv[1];
    const char *metadata_file = argv[2];
    const char *pattern = argv[3];
    int benchmark = 0;
    
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0) {
            benchmark = 1;
        }
    }
    
    double start_time = omp_get_wtime();
    MatchResultSet *results = NULL;
    
    if (strcmp(mode, "serial") == 0) {
        if (benchmark) printf("Running serial exact string matching...\n");
        results = exact_match_serial(pattern, metadata_file);
    } else if (strcmp(mode, "mpi") == 0) {
        results = exact_match_mpi(pattern, metadata_file, argc, argv);
    } else {
        printf("Unknown mode: %s\n", mode);
        return 1;
    }
    
    double end_time = omp_get_wtime();
    double elapsed = (end_time - start_time) * 1000.0;
    
#ifdef USE_MPI
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) {
              free_match_results(results);
        MPI_Finalize();
        return 0;
    }
#endif
    
    if (!results) {
        printf("Error: Failed to perform search\n");
#ifdef USE_MPI
        MPI_Finalize();
#endif
        return 1;
    }
    
    printf("\nPattern: \"%s\"\n", pattern);
    printf("Execution time: %.2f ms\n", elapsed);
    printf("Documents with matches: %zu\n\n", results->count);
    
    int display_count = results->count < 10 ? results->count : 10;
    for (int i = 0; i < display_count; i++) {
        printf("%d. Document ID: %d, Matches: %d\n",
               i + 1, results->results[i].doc_id, results->results[i].match_count);
    }
    
    free_match_results(results);
    
#ifdef USE_MPI
    MPI_Finalize();
#endif
    return 0;
}

