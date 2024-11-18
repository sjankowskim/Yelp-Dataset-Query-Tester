#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <time.h>

int execute_sql(sqlite3 *db, const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }
    return SQLITE_OK;
}

int load_tsv_to_db(sqlite3 *db, const char *filepath, const char *table_name, const char *schema, int num_columns) {
    int rc;
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", filepath);
        return EXIT_FAILURE;
    }

    rc = execute_sql(db, schema);
    if (rc != SQLITE_OK) {
        fclose(file);
        return rc;
    }

    // Optimize SQLite settings for bulk insertion
    execute_sql(db, "PRAGMA synchronous = OFF;");
    execute_sql(db, "PRAGMA journal_mode = MEMORY;");

    // Start transaction
    execute_sql(db, "BEGIN TRANSACTION;");

    // Dynamically create the placeholders for the INSERT statement
    char placeholders[256];
    char *ph = placeholders;
    for (int i = 0; i < num_columns; i++) {
        if (i > 0) {
            *ph++ = ',';
            *ph++ = ' ';
        }
        *ph++ = '?';
    }
    *ph = '\0';

    char insert_sql[10000];
    snprintf(insert_sql, sizeof(insert_sql),
             "INSERT INTO %s VALUES (%s);", table_name, placeholders);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        fclose(file);
        return rc;
    }

    char line[10000];

    // Skip the header line
    if (!fgets(line, sizeof(line), file)) {
        fprintf(stderr, "Failed to read header from file: %s\n", filepath);
        sqlite3_finalize(stmt);
        fclose(file);
        return EXIT_FAILURE;
    }

    long total_lines = 0;
    while (fgets(line, sizeof(line), file)) {
        total_lines++;
    }
    rewind(file);
    fgets(line, sizeof(line), file);  // Re-read header line

    printf("Loading table %s with %ld entries:\n", table_name, total_lines);

    long current_line = 0;
    while (fgets(line, sizeof(line), file)) {
        current_line++;
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        char *token = strtok(line, "\t");
        int idx = 1;
        while (token != NULL && idx <= num_columns) {
            sqlite3_bind_text(stmt, idx, token, -1, SQLITE_TRANSIENT);
            token = strtok(NULL, "\t");
            idx++;
        }

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
            printf("%s\n", line);
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            continue;
        }
        sqlite3_reset(stmt);

        if (current_line % (total_lines / 1000 + 1) == 0 || current_line == total_lines) {
            printf("\rLoading: %.2f%%", (double)current_line / total_lines * 100);
            fflush(stdout);
        }
    }

    printf("\nFinished loading %s\n", table_name);
    sqlite3_finalize(stmt);
    fclose(file);

    // End transaction
    execute_sql(db, "COMMIT;");

    // Reset settings
    execute_sql(db, "PRAGMA synchronous = FULL;");
    execute_sql(db, "PRAGMA journal_mode = DELETE;");

    return SQLITE_OK;
}

int execute_query_from_file(sqlite3 *db, const char *query_file, const char *result_file) {
    FILE *file = fopen(query_file, "r");
    if (!file) {
        fprintf(stderr, "Cannot open query file: %s\n", query_file);
        return EXIT_FAILURE;
    }

    // Read the entire query file into a single buffer
    char query[1024] = "";  // Assuming a max length; adjust if necessary
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        strcat(query, line);
    }
    fclose(file);

    printf("Executing query: %s\n", query);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare query: %s\n", sqlite3_errmsg(db));
        return rc;
    }

    file = fopen(result_file, "w");
    if (!file) {
        fprintf(stderr, "Cannot open result file: %s\n", result_file);
        sqlite3_finalize(stmt);
        return EXIT_FAILURE;
    }

    int col_count = sqlite3_column_count(stmt);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (int col = 0; col < col_count; col++) {
            const char *text = (const char *)sqlite3_column_text(stmt, col);
            fprintf(file, "%s%s", text ? text : "NULL", col < col_count - 1 ? "\t" : "\n");
        }
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute query: %s\n", sqlite3_errmsg(db));
    }

    fclose(file);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE ? SQLITE_OK : rc;
}

int main(int argc, char *argv[]) {
    const char *db_filename = "yelpdb.sqlite";
    int preserve = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--preserve") == 0) {
            preserve = 1;
            break;
        }
    }

    if (!preserve) {
        if (access(db_filename, F_OK) == 0) {
            if (unlink(db_filename) != 0) {
                fprintf(stderr, "Failed to delete existing database\n");
                return EXIT_FAILURE;
            }
        }
    }

    sqlite3 *db;
    int rc = sqlite3_open(db_filename, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    if (!preserve) {
        if (load_tsv_to_db(db, "data/yelp_academic_dataset_business.tsv", "business",
                           "CREATE TABLE business (business_id TEXT, name TEXT, address TEXT, city TEXT, state TEXT, postal_code TEXT, latitude REAL, longitude REAL, stars INTEGER, review_count INTEGER, is_open INTEGER, PRIMARY KEY (business_id));", 11) != SQLITE_OK) {
            fprintf(stderr, "Failed to load yelp_academic_dataset_business.tsv into database\n");
            sqlite3_close(db);
            return EXIT_FAILURE;
        }

        if (load_tsv_to_db(db, "data/yelp_academic_dataset_user.tsv", "user",
                           "CREATE TABLE user (user_id TEXT, name TEXT, review_count INTEGER, yelping_since TEXT, friends TEXT, useful INTEGER, funny INTEGER, cool INTEGER, fans INTEGER, elite TEXT, average_stars REAL, total_compliments INTEGER, PRIMARY KEY (user_id));", 12) != SQLITE_OK) {
            fprintf(stderr, "Failed to load yelp_academic_dataset_user.tsv into database\n");
            sqlite3_close(db);
            return EXIT_FAILURE;
        }

         if (load_tsv_to_db(db, "data/yelp_academic_dataset_tip.tsv", "tip",
                           "CREATE TABLE tip (text TEXT, date DATETIME, compliment_count INTEGER, business_id TEXT, user_id TEXT, PRIMARY KEY(business_id, user_id, date, text), FOREIGN KEY(user_id) REFERENCES user(user_id));", 5) != SQLITE_OK) {
            fprintf(stderr, "Failed to load title.basics.tsv into database\n");
            sqlite3_close(db);
            return EXIT_FAILURE;
        }

        if (load_tsv_to_db(db, "data/yelp_academic_dataset_review.tsv", "review",
                           "CREATE TABLE review (review_id TEXT, user_id TEXT, business_id TEXT, stars REAL, date DATETIME, text TEXT, useful INTEGER, funny INTEGER, cool INTEGER, PRIMARY KEY (review_id), FOREIGN KEY (user_id) REFERENCES user(user_id), FOREIGN KEY (business_id) REFERENCES business(business_id));", 9) != SQLITE_OK) {
            fprintf(stderr, "Failed to load title.episode.tsv into database\n");
            sqlite3_close(db);
            return EXIT_FAILURE;
        }
    }

    char user_input[10];
    while (1) {
        printf("Type 'y' to execute the stored query, or 'n' to exit the program: ");
        
        if (fgets(user_input, sizeof(user_input), stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            continue;
        }
        
        size_t len = strlen(user_input);
        if (len > 0 && user_input[len - 1] == '\n') {
            user_input[len - 1] = '\0';
        }

        if (strcmp(user_input, "y") == 0) {
            clock_t start_time = clock();

            if (execute_query_from_file(db, "query.txt", "result.txt") != SQLITE_OK) {
                fprintf(stderr, "Failed to execute query and write results\n");
                sqlite3_close(db);
                return EXIT_FAILURE;
            }

            clock_t end_time = clock();

            double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
            printf("Query executed in %f seconds\n", time_taken);

        } else if (strcmp(user_input, "n") == 0) {
            printf("Exiting the program.\n");
            break;
        } else {
            printf("Invalid input. Please enter 'y' or 'n'.\n");
        }
    }

    sqlite3_close(db);
    return EXIT_SUCCESS;
}