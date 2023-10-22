#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <omp.h>

#define MAX_WORD_LENGTH 100
#define MAX_FP 0.01

int k;

/**
 *
 * This function calculates a hash value for the input string 'str' using the provided salt value.
 * It employs the APHash algorithm with added salting to generate different hashes for the same string.
 *
 * @param str   The input string for which the hash is computed.
 * @param salt  The salt value used to modify the hash calculation.
 * @param m     bitArray size, since the index will need to be limited to it.
 *
 * @return The computed hash value for the input string with the added salt value, modulo 'm'.
 */
unsigned int APHashWithSalt(char *str, unsigned int salt, int m) {
    unsigned int hash = salt; // Initialize the hash with the provided salt.

    for (int i = 0; str[i]; i++) {
        if (i % 2 == 1) { // Check if the current character's position is odd.
            // Calculate a new hash by XOR-ing the current hash, left-shifting it by 7 bits,
            // XOR-ing it with the current character, and right-shifting it by 3 bits.
            hash ^= ((hash << 7) ^ str[i] ^ (hash >> 3));
        } else {
            // Calculate a new hash by XOR-ing the current hash with the bitwise complement of
            // (left-shifting the hash by 11 bits, XOR-ing it with the current character, and right-shifting it by 5 bits).
            hash ^= (~((hash << 11) ^ str[i] ^ (hash >> 5)));
        }
    }

    // Return the computed hash value, limited to a the range of the bitArray.
    return hash % m;
}

/**
 * Reads words from a file and stores them in an array of strings and updates the arrayLength pointer.
 *
 * This function opens the specified file, reads its contents word by word, and stores
 * them in an array of strings. It also counts the number of words and updates'wordListLength' accordingly.
 *
 * @param filename         The name of the file to read words from.
 * @param wordListLength   A pointer to an integer where the word list length will be stored.
 *
 * @return An array of strings containing the words from the file, or NULL on failure.
 *         Memory for the array and its strings should be freed after use.
 */
char** readWordsFromFile(const char *filename, int *wordListLength) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Declare word, length and array variables
    char word[MAX_WORD_LENGTH];
    char **ppWordListArray = NULL;
    int length = 0;

    // Count the number of words in the file
    while (fscanf(file, "%s", word) == 1) {
        length++; // Increment for each word
    }
    // Reset the file pointer to the beginning of the file
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the word list
    ppWordListArray = (char **)malloc(length * sizeof(char *));
    if (ppWordListArray == NULL) {
        printf("Memory allocation failed for ppWordListArray.\n");
        fclose(file);
        return NULL;
    }

    // Read words from the file and store them in ppWordListArray
    for (int i = 0; i < length; i++) {
        if (fscanf(file, "%s", word) != 1) {
            printf("Error reading word from file.\n");
            fclose(file);
            free(ppWordListArray); // Free previously allocated memory
            return NULL;
        }
        ppWordListArray[i] = strdup(word);
    }

    // Close file
    fclose(file);

    // Update the wordListLength pointer
    *wordListLength = length; // Set the word list length

    return ppWordListArray;
}

/**
 * Inserts words into a Bloom filter represented by a bit array.
 *
 * This function takes an array of words and inserts them into a Bloom filter
 * represented by a bit array. It uses multiple hash functions to set the bits
 * in the filter corresponding to the hashed values of each word.
 *
 * @param ppWordListArray  An array of strings containing words to insert.
 * @param numToInsert      The number of words to insert from the array.
 * @param bitArray         The bit array representing the Bloom filter.
 * @param m                The size of the bit array (modulo value for hashing).
 */
void insertWords(char **ppWordListArray, int numToInsert, int* bitArray, int m) {
    // Passes Bernsteins' condition
    #pragma omp parallel for collapse(2) schedule(static, 1)
    for (int i = 0; i < numToInsert; i++) {
        for (int h = 0; h < k; h++) {
            // Calculate the hash using APHashWithSalt
            unsigned int hash = APHashWithSalt(ppWordListArray[i], h, m);
            
            // Set the corresponding bit in the bit array to 1
            bitArray[hash] = 1;
        }
    }
}

/**
 * Calculates the optimal size of a Bloom filter bit array based on the expected number of elements.
 *
 * This function calculates the optimal size (number of bits) for a Bloom filter's
 * bit array based on the expected number of elements to be inserted and the desired
 * maximum false positive rate.
 *
 * @param n The expected number of elements to be inserted into the Bloom filter.
 * @return The optimal size of the bit array for the given parameters.
 */
int calculateOptimalArraySize(int n) {
    // Calculate the optimal size using the formula for Bloom filter size
    int m = ceil((n * log(MAX_FP)) / log(1 / pow(2, log(2))));
    return m;
}

/**
 * Checks whether a word is possibly in the Bloom filter's set based on its hashes.
 *
 * This function checks whether a given word is possibly in the Bloom filter's set
 * based on its hash values for multiple hash functions.
 *
 * @param word                   The word to check.
 * @param bitArray               The Bloom filter's bit array.
 * @param m                      The size of the Bloom filter's bit array.
 * @return isPossiblyInSet       Returns 1 if the word is possibly in the set, 0 otherwise.
 */
int lookUp(char* word, int* bitArray, int m) {
    int isPossiblyInSet = 1;
    // Iterate through the hash functions and check the corresponding bit
    //#pragma omp parallel for reduction(&&:isPossiblyInSet)
    for (int h = 0; h < k; h++) {
        int index = APHashWithSalt(word, h, m); // Compute the hash index
        isPossiblyInSet = (bitArray[index] && isPossiblyInSet); // Perform a logical AND operation
    }
    return isPossiblyInSet;
}


/**
 * Reads query words and query bits from a file into memory.
 *
 * This function reads words and their corresponding query bits from a file and
 * stores them in dynamically allocated memory. Each word[i] will have its respective
 * bit stored in bits[i], indicating whether the word actually exists in the filter or 
 * not.
 *
 * @param fileName     The name of the file to read.
 * @param wordsBuffer  A pointer to the buffer where word strings will be stored.
 * @param bits         A pointer to the buffer where query bits will be stored for each respective word.
 * @param length       A pointer to an integer that will store the length of the created arrays.
 */
void readQuery(const char *fileName, char ***wordsBuffer, int **bits, int *length) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char word[MAX_WORD_LENGTH];
    int queryBit;
    int fileLength = 0;

    // Count the number of words in the file
    while (fscanf(file, "%s %d", word, &queryBit) == 2) {
        fileLength++; // Increment for each word
    }

    // Reset the file pointer to the beginning of the file
    fseek(file, 0, SEEK_SET);

    *wordsBuffer = (char **)malloc(fileLength * sizeof(char *));
    *bits = (int *)malloc(fileLength * sizeof(int));

    if (*wordsBuffer == NULL || *bits == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        
        // Free memory allocated for words and bits in case of an error
        free(*wordsBuffer);
        free(*bits);
        *wordsBuffer = NULL;
        *bits = NULL;
        
        return;
    }

    // Read words and query bits into the buffer
    for (int i = 0; i < fileLength; i++) {
        if (fscanf(file, "%s %d", word, &queryBit) != 2) {
            perror("Error reading word and bit from file");
            break;
        }
        (*wordsBuffer)[i] = strdup(word);
        (*bits)[i] = queryBit;
    }
    // Close file and then update the length of the query Array
    fclose(file);
    *length = fileLength;
}

/**
 * Test words against a Bloom filter and calculate false positive and false negative percentages.
 *
 * This function tests words against a Bloom filter represented by the bitArray. It calculates
 * the false positive and false negative percentages based on the expected query bits.
 * @param bitArray        The Bloom filter represented as an array of bits.
 * @param words           An array of query words to test.
 * @param bits            An array of expected query bits.
 * @param length          The number of words and bits to test.
 * @param m               The size of the Bloom filter bitArray.
 */
void testBloomWithQueries(int *bitArray, char **words, int *bits, int length, int m) {
    // Declare variables for fp and fn
    int fPositive = 0;          // Number of False Positives
    int fNegative = 0;          // Number of False Negatives
    int totalPositive = 0;      // Total number of positives available inside the words list, not the filter.
    int totalNegative = 0;      // Total number of negatives available inside the words list, not the filter.

    // For-loop to iterate through each word & corresponding bit to test the filter.
    #pragma omp parallel for reduction(+:fPositive, fNegative, totalPositive, totalNegative) schedule(static, 1)
    for (int i = 0; i < length; i++) {
        // Get the word and corresponding bit from the arrays.
        char *tempWord = words[i];
        int expectedBit = bits[i];
        // Get the lookup result from the bloom filter.
        int lookupResult = lookUp(tempWord, bitArray, m);

        // Test whether the ressult is true or false and check for any false positives or false negatives
        if (expectedBit == 1) {
            totalPositive++;
            if (lookupResult == 0) {
                printf("Word is %s \n", tempWord);
                fNegative++;
            }
        } else if (expectedBit == 0) {
            totalNegative++;
            if (lookupResult == 1) {
                fPositive++;
            }
        }

    }
    // Print the test result
    printf("False Negative Percentage: %lf%%\n", (double)fNegative / totalPositive * 100);
    printf("False Positive Percentage: %lf%%\n", (double)fPositive / totalNegative * 100);
}

int main(int argc, char *argv[]) {
    // Initialize timing for the whole program
    struct timespec all_start, all_end;
    double all_time;
    clock_gettime(CLOCK_MONOTONIC, &all_start);

    // Check number of program arguments
    if (argc != 3) {
        printf("Usage: %s <words.txt> <query.txt>\n", argv[0]);
        return -1;
    }

    // Get the file names from program arguments
    char *insertFilename = argv[1]; 
    char *testFilename = argv[2];   
    
    struct timespec start, end;
    double time_taken;
    
    // Declare variables for words array, query array, bits array and their sizes
    int numToInsert = 0;
    char **ppInsertWordListArray = NULL;

    char **queries = NULL;
    int *bits = NULL;
    int querySize = 0;

    // Time reading from file(s)
    clock_gettime(CLOCK_MONOTONIC, &start);

    #pragma omp parallel sections
    {   
        #pragma omp section
        {
            readQuery(testFilename, &queries, &bits, &querySize);
        }

        #pragma omp section
        {
            ppInsertWordListArray = readWordsFromFile(insertFilename, &numToInsert);
        }
    }

    if (ppInsertWordListArray == NULL) {
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Reading time (s): %lf \n", time_taken);

    int m = calculateOptimalArraySize(numToInsert);
    // update k global variable
    k = (m/numToInsert) * log(2);

    int *bitArray = (int *)malloc(m*sizeof(int));
    if (bitArray == NULL) {
        printf("Memory allocation failed for bitArray.\n");
        free(ppInsertWordListArray);
        return 1;
    }

    // Time insertion of words into the Bloom filter
    clock_gettime(CLOCK_MONOTONIC, &start);
    // Insert all the words from ppInsertWordListArray into bitArray
    insertWords(ppInsertWordListArray, numToInsert, bitArray, m);   
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Inserting time (s): %lf \n", time_taken);

    // Test words against the Bloom filter
    // Measure Bloom Filter Testing Time
    clock_gettime(CLOCK_MONOTONIC, &start);
    testBloomWithQueries(bitArray, queries, bits, querySize, m);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Testing time (s): %lf \n", time_taken);

   // Free memory for query words and bits
    for (int i = 0; i < querySize; i++) {
        free(queries[i]);
    }
    free(queries);
    free(bits);
    free(bitArray);

    // Free memory allocated for ppInsertWordListArray
    for (int i = 0; i < numToInsert; i++) {
        free(ppInsertWordListArray[i]);
    }
    free(ppInsertWordListArray);

    clock_gettime(CLOCK_MONOTONIC, &all_end);
    all_time = (all_end.tv_sec - all_start.tv_sec) * 1e9;
    all_time = (all_time + (all_end.tv_nsec - all_start.tv_nsec)) * 1e-9;
    printf("Total time (s): %lf \n", all_time);
    return 0;
}
