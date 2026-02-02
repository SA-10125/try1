//hill cipher impliment. make it work for spaces, numbers, uppercase, etc. (some time obscurification thing). impliment proper error messages.

/*******************************************************************************
 * ShatterLock - Enhanced File Encryption System
 * 
 * DESCRIPTION:
 *   A multi-layer encryption system that combines Vigenère cipher with Hill 
 *   cipher and distributes encrypted data across multiple packets with junk 
 *   data to obscure patterns and increase security against cryptanalysis.
 * 
 * FEATURES:
 *   - Triple-key encryption (username/password + 2 security questions)
 *   - Vigenère cipher for stream encryption
 *   - Hill cipher for matrix-based encryption (26x26)
 *   - Junk data insertion to increase packet count and obscure patterns
 *   - Distributed packet storage across multiple subdirectories
 *   - Metadata encryption for packet information
 * 
 * SECURITY MODEL:
 *   - Key hierarchy: main keystream -> first_key -> second_key
 *   - Each layer encrypted with combination of previous keys
 *   - Reproducible packet naming based on seeds
 *   - Junk packets intermixed with real packets
 
 * 
 * KNOWN ISSUES:
 *   - Metadata format (001A002) incompatible with Hill cipher (a-z only)
 *   - Packet contents ending in .txt and too long (null termination issue)
 *   - Delete function needs testing and fixes
 *   - Directory order dependency can break encryption if dirs are added
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE FILES
 ******************************************************************************/
#include <stdio.h>      // for printf and scanf
#include <string.h>     // for strlen, strcpy, memcpy
#include <math.h>       // for ceil
#include <stdlib.h>     // for rand functions, malloc, free and exit
#include <time.h>       // for time(0) when randomness needed
#include <dirent.h>     // for scanning of subdirectories in linux/posix systems
#include <sys/stat.h>   // lets us access file/directory metadata like permissions and type

/*******************************************************************************
 * UTILITY FUNCTIONS
 ******************************************************************************/

/**
 * Prompts user for input and stores result in the provided string
 * @param what - Description of what to input (e.g., "Username")
 * @param input_to_this_string - Buffer to store the input
 */

/**
 * Prompts user for input and stores result in the provided string
 * @param what - Description of what to input (e.g., "Username")
 * @param input_to_this_string - Buffer to store the input
 */
void inputstring(char *what, char *input_to_this_string){
    printf("Enter %s:",what);
    scanf("%s",input_to_this_string);
}

/**
 * Extracts a substring from a string
 * @param sliced_out - Output buffer for the sliced string
 * @param to_slice - Source string to slice from
 * @param from - Starting index (inclusive)
 * @param to - Ending index (inclusive)
 * Note: Assumes sliced_out has sufficient space allocated
 */

/**
 * Extracts a substring from a string
 * @param sliced_out - Output buffer for the sliced string
 * @param to_slice - Source string to slice from
 * @param from - Starting index (inclusive)
 * @param to - Ending index (inclusive)
 * Note: Assumes sliced_out has sufficient space allocated
 */
void slice(char *sliced_out, char *to_slice, int from, int to){
    int count=0; //we assume size of sliced_out alloted is greater than or equal to (size of the slice)+1
    for(int i=from;i<=to;i++){
        sliced_out[count++]=to_slice[i];
    }
    sliced_out[count]='\0';
}

/*******************************************************************************
 * FILE I/O FUNCTIONS
 ******************************************************************************/

/**
 * Writes contents to a file, checking for duplicate filenames
 * @param filepath - Full path to the file to create
 * @param contents - String content to write to the file
 * Note: Exits with error if file write fails
 */

/**
 * Writes contents to a file, checking for duplicate filenames
 * @param filepath - Full path to the file to create
 * @param contents - String content to write to the file
 * Note: Exits with error if file write fails
 */
void writetofile(char *filepath, char *contents) { 

    //just checking for error
    FILE *check = fopen(filepath, "r");
    if (check) {
        fclose(check);
        printf("FILE IS REPEATING ERROR: %s\n", filepath);
        return;
    }
    
    FILE *pF = fopen(filepath, "w"); 
    if (!pF) { //just checking for error
        printf("File open error for %s\n", filepath);
        exit(1);
    }

    //-shashi till here
    fprintf(pF, "%s", contents); //puts it in and closes.
    fclose(pF);
}

/**
 * Reads a 26-byte packet from a file
 * @param filepath - Path to the file to read from
 * @param contents_out - Buffer to store the read content (must be >= 26 bytes)
 * Note: Exits with error if file cannot be opened
 */

/**
 * Reads a 26-byte packet from a file
 * @param filepath - Path to the file to read from
 * @param contents_out - Buffer to store the read content (must be >= 26 bytes)
 * Note: Exits with error if file cannot be opened
 */
void readcontents(char *filepath, char *contents_out){ //-many changes by shashi
    
    FILE *pF = fopen(filepath, "r"); //just error handling
    if (!pF) {
        printf("File open error: %s\n", filepath);
        exit(1);
    }

    fgets(contents_out, 26, pF);  // 26-byte packet put in contents out and sent.
    fclose(pF);
}

/**
 * Deletes multiple packet files
 * @param packetpath - Array of file paths to delete
 * @param numpackets - Number of packets to delete
 */

/**
 * Deletes multiple packet files
 * @param packetpath - Array of file paths to delete
 * @param numpackets - Number of packets to delete
 */
void deletepackets(char packetpath[][513], int numpackets){
    for(int i=0;i<numpackets;i++){
        if (remove(packetpath[i]) == 0) {
            printf("File deleted successfully.\n");
        } else {
            printf("Error: Unable to delete the file: %s\n",packetpath[i]);
        }
    }
}

/*******************************************************************************
 * DIRECTORY MANAGEMENT FUNCTIONS
 ******************************************************************************/

/**
 * Retrieves subdirectories from a parent directory
 * @param parent - Path to the parent directory
 * @param subdirs_out - Array to store subdirectory names
 * @param max_subdirs - Maximum number of subdirectories to retrieve
 * @return Number of subdirectories found
 * Note: Skips "." and ".." directories
 */
int get_subdirectories(const char *parent, char subdirs_out[][256], int max_subdirs) { //-shashi
    DIR *dir = opendir(parent); 
    if (!dir) {
        perror("opendir failed");
        return 1;
    }
    //above lines open the parent directory, throws error 1 if fail.

    struct dirent *entry; //used to iterte through each object in directory.
    int count = 0; //how many subdirs found
    char path[512]; //full path names
    struct stat statbuf; //is the entry a diretory? 

    while ((entry = readdir(dir)) != NULL && count < max_subdirs) { //reading each object in parent till max_subdirs found.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){continue;} //if directory is . or .., skip it. (those are current and parent dirs)

        snprintf(path, sizeof(path), "%s/%s", parent, entry->d_name); //constructs full path
        if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) { //if metadata and if its a directory, put into subdirs[]
            strncpy(subdirs_out[count], entry->d_name, 255);
            subdirs_out[count][255] = '\0';  // null terminate just in case
            count++; 
        }
    }

    closedir(dir); //closes parent
    return count; //returns number of (useful) subdirs put into subdirs[].
}

/**
 * Generates file paths for packets by combining directory paths and packet names
 * @param packetpaths_out - Array to store generated full paths
 * @param packetnames - Array of packet filenames
 * @param numpacks - Number of packets to generate paths for
 * @param seed - Random seed for reproducible directory selection
 * Note: Creates 'default' directory if no subdirectories exist
 */

void getpaths(char packetpaths_out[][513],char packetnames[][100], int numpacks, int seed){ 
//-shashi from here
    char subdirs[100][256];
    int num_subdirs = get_subdirectories("storage", subdirs, 100); //gets (up to) 100 subdirs from storage.

    //TODO: check and make sure reproducibility.
    //PROBLEM THIS IS NOT REPRODUCIBLE. MAKE SURE IT DOESNT HAPPEN!!
    if (num_subdirs == 0) { //if no subdirectories found. failsafe. consider changing it to make dirs based on key instead of just using default.
        printf("ERROR: No subdirectories found. Creating 'default'...\n");
        mkdir("storage/default");
        strcpy(subdirs[0], "default");
        num_subdirs = 1;
    }

    srand(seed);
    for(int i=0;i<numpacks;i++){//we go through each packet and take the filename and path and combine to put in list of packetpaths
        int subdir_index = rand() % num_subdirs;  //chooses a number and takes subdir[that num]
        sprintf(packetpaths_out[i],"storage/%s/%s.txt", subdirs[subdir_index], packetnames[i]); //prints directory name into filepath   
    }
}

/*******************************************************************************
 * KEY GENERATION FUNCTIONS
 ******************************************************************************/

/**
 * Generates a unique seed from a keystream for reproducible randomness
 * @param keystream - Array of key values
 * @param len_of_keystream - Length of the keystream array
 * @return Integer seed value (0-99990)
 * Note: Uses modulo with large prime (99991) to prevent overflow
 */
int getuniquetouserseed(int *keystream, int len_of_keystream){
    if (keystream == NULL || len_of_keystream <= 0) {
        return 0;
    }
    long long int sum=0;
    for (int i = 0; i < len_of_keystream; i++) {
        // Add the keystream element. Modulo inside the loop helps prevent overflow
        // if keystream values or length are very large, keeping the sum within long long limits.
        // Using a large prime helps distribute the sum.
        sum = (sum + keystream[i]) % 99991; //99991 is a large prime number.
    }
    sum = (sum + len_of_keystream) % 99991;
    if (sum < 0) {
        sum += 99991; // Add the prime to make it positive
    }
    return ((int)sum);
}

/**
 * Generates main encryption keystream from username and password
 * @param usn - Username string
 * @param pwd - Password string
 * @param keystream_out - Output array to store generated keystream
 * @return Length of the generated key
 * Note: Key length is the minimum of username and password lengths
 */

int makekey(char *usn, char *pwd, int *keystream_out){ //here, key depends on the element in front of it.
    //i know this is innefficient, just roll with it for now.
    int len_of_key=(strlen(pwd)<strlen(usn))?strlen(pwd):strlen(usn);
    for(int i=0;i<len_of_key;i++){
        srand((int)usn[i]*(int)pwd[i]);
        keystream_out[i]=(rand())%26;
    }
    for(int i=0;i<(len_of_key-1);i++){
        srand(keystream_out[i]*keystream_out[i+1]); //i know its usually i-1 but i thought why not
        keystream_out[i]=(rand())%26;
    }
    srand(keystream_out[len_of_key-1]*keystream_out[0]);
    keystream_out[len_of_key-1]=rand()%26;
    return (len_of_key);
}

/*******************************************************************************
 * VIGENÈRE CIPHER FUNCTIONS
 ******************************************************************************/

/**
 * Encrypts plaintext using Vigenère cipher
 * @param plaintext - Input text to encrypt (lowercase a-z)
 * @param ciphertext_out - Output buffer for encrypted text
 * @param keystream - Array of key values (0-25)
 * @param len - Length of the keystream
 * Note: Assumes input is lowercase letters only
 */
void vigenerre_encrypt(char *plaintext, char *ciphertext_out, int *keystream, int len){
    for(int i=0;i<strlen(plaintext);i++){
        ciphertext_out[i]=(char)(((((int)plaintext[i])+keystream[i%len])+26-97)%26)+97;
    }
    ciphertext_out[strlen(plaintext)]='\0'; //null_terminating
}

/**
 * Decrypts ciphertext using Vigenère cipher
 * @param ciphertext - Input text to decrypt (lowercase a-z)
 * @param plaintext_out - Output buffer for decrypted text
 * @param keystream - Array of key values (0-25)
 * @param len - Length of the keystream
 * Note: Reverse operation of vigenerre_encrypt
 */
void vigenerre_decrypt(char *ciphertext, char *plaintext_out, int *keystream, int len){
    for(int i=0;i<strlen(ciphertext);i++){
        plaintext_out[i]=(char)(((((int)ciphertext[i])-keystream[i%len])+26-97)%26)+97;
    }
    plaintext_out[strlen(ciphertext)]='\0'; //null_terminating
}

/*******************************************************************************
 * JUNK DATA INSERTION/REMOVAL FUNCTIONS
 ******************************************************************************/

/**
 * Inserts random junk characters between real characters
 * @param ciphertext_in - Input ciphertext
 * @param ciphertext_out - Output with junk inserted (alternating junk and real)
 * Note: Doubles the length of the text; used to obscure patterns
 */

void insertjunkintostream(char *ciphertext_in, char *ciphertext_out){
    char letters[]="abcdefghijklmnopqrstuvwxyz";
    srand(time(0));
    int len=strlen(ciphertext_in);
    int count=0, j=0;
    for(int i=0;i<len;i++){
            ciphertext_out[count++]=letters[rand()%strlen(letters)]; //junk character
            ciphertext_out[count++]=ciphertext_in[i]; //real character
    }
    ciphertext_out[count]='\0';
}
void removejunkfromstream(char *ciphertext_in, char *ciphertext_out){  //TODO: IMP might not work. check.
    // takes every other character only.
//please note that this has no cryptographic value. it is only to increase the number of packets.
//this is ovious from the fact that it is not relying on the key.
//TODO: Take a call on wether this is actually useful or if the number of packets dont matter.
    int len=strlen(ciphertext_in);
    int count=0;
    if(len%2!=0){printf("ERROR: len in remove junk is not even!");}
    for(int i=0;i<len;i++){ //len is even
            i++; //junk character.
            ciphertext_out[count++]=ciphertext_in[i]; //real character
    }
    ciphertext_out[count]='\0';
}

/**
 * Generates a 26x26 invertible Hill cipher key matrix
 * @param seed - Random seed for reproducible key generation
 * @param hillkey_out - Output 26x26 matrix
 * Note: Uses L*U construction (lower-triangular * upper-triangular) to
 *       guarantee invertibility modulo 26 on first attempt
 */
void makehillkey(int seed, int hillkey_out[26][26]) { //github copilot made this. pls forgive.
    srand(seed); // Seed the random number generator
    
    // This function generates a 26x26 invertible matrix modulo 26 using the L*U construction method.
    // The matrix is always invertible mod 26, and the process is deterministic for a given seed.
    // L: lower-triangular with 1s on the diagonal (invertible mod 26)
    // U: upper-triangular with random nonzero diagonal (invertible mod 26)
    int L[26][26] = {0};
    int U[26][26] = {0};
    // Fill L (lower-triangular, 1s on diagonal)
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j <= i; j++) {
            if (i == j) {
                L[i][j] = 1; // 1s on diagonal
            } else {
                L[i][j] = rand() % 26; // random element in Z26
            }
        }
    }
    // Fill U (upper-triangular, random nonzero diagonal)
    for (int i = 0; i < 26; i++) {
        for (int j = i; j < 26; j++) {
            if (i == j) {
                // Diagonal must be coprime to 26 (i.e., odd and not divisible by 13)
                int val = (rand() % 25) + 1; // 1..25
                while (val % 2 == 0 || val % 13 == 0) {
                    val = (rand() % 25) + 1;
                }
                U[i][j] = val;
            } else {
                U[i][j] = rand() % 26;
            }
        }
    }
    // Multiply L and U to get the final key matrix (mod 26)
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < 26; j++) {
            int sum = 0;
            for (int k = 0; k < 26; k++) {
                sum = (sum + L[i][k] * U[k][j]) % 26;
            }
            hillkey_out[i][j] = sum;
        }
    }
    // Now hillkey_out is a random invertible matrix mod 26, reproducible from the seed.
}

int makefirstanswerkey(int *keystream, int len_of_keystream, char *first_answer, int *keystream_out, int seed){
    int len_out=(strlen(first_answer)<len_of_keystream)?strlen(first_answer):len_of_keystream;
    int randstream[len_out];
    srand(seed);
    for(int i=0;i<len_out;i++){randstream[i]=rand();}
    
    for(int i=0;i<len_out;i++){
        srand((int)first_answer[i]*(keystream[i]+1)*(randstream[i]+1));
        keystream_out[i]=(rand()+seed)%26;
    }
    for(int i=0;i<(len_out-1);i++){
        srand(keystream_out[i]*keystream_out[i+1]); //i know its usually i-1 but i thought why not
        keystream_out[i]=(rand()*seed)%26;
    }
    srand(keystream_out[len_out-1]*keystream_out[0]);
    keystream_out[len_out-1]=(rand()*seed)%26;

    return (len_out);
}

int makesecondanswerkey(int *keystream,int *key1, int key1len, char *second_answer, int *keystream_out, int seed, int seed1){
    int len_out=(strlen(second_answer)<key1len)?strlen(second_answer):key1len;
    //key 1 is as long as the shorter bw keystream and answer 1. so we dont need to compare those 2 seperately.
    int randstream[len_out];
    srand(seed*seed1);
    for(int i=0;i<len_out;i++){randstream[i]=rand();}
    
    for(int i=0;i<(len_out);i++){
        srand((int)second_answer[i]*(keystream[i]+1)*(key1[i]+1)*(randstream[i]+1));
        keystream_out[i]=(rand()+seed+seed1)%26;
    }
    for(int i=0;i<(len_out-1);i++){
        srand(keystream_out[i]*keystream_out[i+1]); //i know its usually i-1 but i thought why not
        keystream_out[i]=(rand()*seed*seed1)%26;
    }
    srand(keystream_out[len_out-1]*keystream_out[0]);
    keystream_out[len_out-1]=(rand()*seed*seed1)%26;

    return (len_out);
}

void makepackets(char *ciphertext, char packets_out[][26]){ //only works for less than 26 packets.
    //i know im wasting 2 characters per packet. thats in case i need any other addition.
    //first two letters are the number of junk characters added at the end for the packing to suit hill cipher usage.
    char letters[]="abcdefghijklmnopqrstuvwxyz";
    int j=0;
    char temp[26]={'\0'};
    int counter=7;
    sprintf(temp, "aa%c%caa%c", letters[(int)ceil(strlen(ciphertext)/18.0)], ciphertext[0], letters[j+1]); //metadata that tells: num_of_packets, identifying attribute, packet_num
    for(int i=0;i<strlen(ciphertext);i++){
        temp[counter++]=ciphertext[i];
        if(counter==25){
            temp[25]='\0';
            strcpy(packets_out[j++],temp);
            if (j>98){return;}
            for(int k=0;k<26;k++){temp[k]='\0';}
            sprintf(temp, "aa%c%caa%c", letters[(int)ceil(strlen(ciphertext)/18.0)], ciphertext[0], letters[j+1]); //metadata that tells: num_of_packets, identifying attribute, packet_num
            counter=7;
        }
    }
    char onlyletters[]="abcdefghijklmnopqrstuvwxyz";
    if (strlen(ciphertext)%18!=0){ //handling last packet if incomplete
        //putting junk 
        int no_of_junk=25-counter;
        temp[0]=onlyletters[no_of_junk/10]; //so 3 means d, 0 means a etc.
        temp[1]=onlyletters[no_of_junk%10]; //so 3 means d, 0 means a etc.
        for(int k=strlen(temp);k<25;k++){ //now for the junk.
        temp[k]=onlyletters[rand()%26];
        }
        temp[25]='\0';
        strcpy(packets_out[j],temp);
    }
}

int makejunk(char packets_out[][26], int hillkey[][26]){ 
    char randtext[]="philosophyofeducationisalabelappliedtothestudyofthepurposeprocessnatureandidealsofeducationitcanbeconsideredabranchofbothphilosophyandeducationeducationcanbedefinedastheteachingandlearningofspecificskillsandtheimpartingofknowledgejudgmentandwisdomandissomethingbroaderthanthesocietalinstitutionofeducationweoftenspeakofmanyeducationalistsconsideritaweakandwoollyfieldtoofarremovedfromthepracticalapplicationsoftherealworldtobeusefulbutphilosophersdatingbacktoplatoandtheancientgreekshavegiventheareamuchthoughtandemphasisandthereislittledoubtthattheirworkhashelpedshapethepracticeofeducationoverthemillenniaplatoistheearliestimportanteducationalthinkerandeducationisanessentialelementintherepublichismostimportantworkonphilosophyandpoliticaltheorywrittenaroundbcinitheadvocatessomeratherextrememethodsremovingchildrenfromtheirmotherscareandraisingthemaswardsofthestateanddifferentiatingchildrensuitabletothevariouscastesthehighestreceivingthemosteducationsothattheycouldactasguardiansofthecityandcareforthelessablehebelievedthateducationshouldbeholisticincludingfactsskillsphysicaldisciplinemusicandartplatobelievedthattalentandintelligenceisnotdistributedgeneticallyandthusisbefoundinchildrenborntoallclassesalthoughhisproposedsystemofselectivepubliceducationforaneducatedminorityofthepopulationdoesnotreallyfollowademocraticmodelaristotleconsideredhumannaturehabitandreasontobeequallyimportantforcestobecultivatedineducationtheultimateaimofwhichshouldbetoproducegoodandvirtuouscitizensheproposedthatteachersleadtheirstudentssystematicallyandthatrepetitionbeusedasakeytooltodevelopgoodhabitsunlikesocratesemphasisonquestioninghislistenerstobringouttheirownideasheemphasizedthebalancingofthetheoreticalandpracticalaspectsofsubjectstaughtamongwhichheexplicitlymentionsreadingwritingmathematicsmusicphysicaleducationliteraturehistoryandawiderangeofsciencesaswellasplaywhichhealsoconsideredimportantduringthemedievalperiodtheideaofperennialismwasfirstformulatedbystthomasaquinashisinworkdemagistroperennialismholdsthatoneshouldteachthosethingsdeemedtobeofeverlastingimportancetoallpeopleeverywherenamelyprinciplesandreasoningnotjustfactswhichareapttochangeovertimeandthatoneshouldteachfirstaboutpeoplenotmachinesortechniquesitwasoriginallyreligiousinnatureanditwasonlymuchlaterthatatheoryofsecularperennialismdevelopedduringtherenaissancethefrenchskepticmicheldemontaignewasoneofthefirsttocriticallylookateducationunusuallyforhistimemontaignewaswillingtoquestiontheconventionalwisdomoftheperiodcallingintoquestionthewholeedificeoftheeducationalsystemandtheimplicitassumptionthatuniversityeducatedphilosopherswerenecessarilywiserthanuneducatedfarmworkersforexample";
    //were going with this rand text instead of just taking random characters one at a time so that people cant detect junk usinc Index of Coincidence.
    //this is still easily breakable.
    char plainrandtext[100];
    srand(time(0));

    plainrandtext[0] = '\0'; //gpt made these lines:
    int rand_start = rand() % (strlen(randtext) - 100); // ensure at least 100 chars available
    int rand_len = 49 + rand() % 50; 
    strncpy(plainrandtext, randtext + rand_start, rand_len);
    plainrandtext[rand_len] = '\0';

    char cipherrandtext[100];
    char cipherjunktext[500];
    int key[100];
    int len=makekey(plainrandtext,randtext,key);
    vigenerre_encrypt(plainrandtext,cipherrandtext,key,len);
    insertjunkintostream(cipherrandtext,cipherjunktext);

    int num=(int)ceil(strlen(cipherrandtext)/18.0);
    char packets[num][26];
    makepackets(cipherrandtext,packets_out); //TODO: change to packets when hill is done.
    //hill_encrypt(num,packets,hillkey,packets_out); //TODO: yet to test

    return(num);

}

int makejunkkeystream(int *keystream_out){ //same as before
    char randtext[]="philosophyofeducationisalabelappliedtothestudyofthepurposeprocessnatureandidealsofeducationitcanbeconsideredabranchofbothphilosophyandeducationeducationcanbedefinedastheteachingandlearningofspecificskillsandtheimpartingofknowledgejudgmentandwisdomandissomethingbroaderthanthesocietalinstitutionofeducationweoftenspeakofmanyeducationalistsconsideritaweakandwoollyfieldtoofarremovedfromthepracticalapplicationsoftherealworldtobeusefulbutphilosophersdatingbacktoplatoandtheancientgreekshavegiventheareamuchthoughtandemphasisandthereislittledoubtthattheirworkhashelpedshapethepracticeofeducationoverthemillenniaplatoistheearliestimportanteducationalthinkerandeducationisanessentialelementintherepublichismostimportantworkonphilosophyandpoliticaltheorywrittenaroundbcinitheadvocatessomeratherextrememethodsremovingchildrenfromtheirmotherscareandraisingthemaswardsofthestateanddifferentiatingchildrensuitabletothevariouscastesthehighestreceivingthemosteducationsothattheycouldactasguardiansofthecityandcareforthelessablehebelievedthateducationshouldbeholisticincludingfactsskillsphysicaldisciplinemusicandartplatobelievedthattalentandintelligenceisnotdistributedgeneticallyandthusisbefoundinchildrenborntoallclassesalthoughhisproposedsystemofselectivepubliceducationforaneducatedminorityofthepopulationdoesnotreallyfollowademocraticmodelaristotleconsideredhumannaturehabitandreasontobeequallyimportantforcestobecultivatedineducationtheultimateaimofwhichshouldbetoproducegoodandvirtuouscitizensheproposedthatteachersleadtheirstudentssystematicallyandthatrepetitionbeusedasakeytooltodevelopgoodhabitsunlikesocratesemphasisonquestioninghislistenerstobringouttheirownideasheemphasizedthebalancingofthetheoreticalandpracticalaspectsofsubjectstaughtamongwhichheexplicitlymentionsreadingwritingmathematicsmusicphysicaleducationliteraturehistoryandawiderangeofsciencesaswellasplaywhichhealsoconsideredimportantduringthemedievalperiodtheideaofperennialismwasfirstformulatedbystthomasaquinashisinworkdemagistroperennialismholdsthatoneshouldteachthosethingsdeemedtobeofeverlastingimportancetoallpeopleeverywherenamelyprinciplesandreasoningnotjustfactswhichareapttochangeovertimeandthatoneshouldteachfirstaboutpeoplenotmachinesortechniquesitwasoriginallyreligiousinnatureanditwasonlymuchlaterthatatheoryofsecularperennialismdevelopedduringtherenaissancethefrenchskepticmicheldemontaignewasoneofthefirsttocriticallylookateducationunusuallyforhistimemontaignewaswillingtoquestiontheconventionalwisdomoftheperiodcallingintoquestionthewholeedificeoftheeducationalsystemandtheimplicitassumptionthatuniversityeducatedphilosopherswerenecessarilywiserthanuneducatedfarmworkersforexample";
    char randusn[100];
    char randpwd[100];
    srand(time(0));
    randusn[0] = '\0'; 
    randpwd[0] = '\0';
    int rand_start = rand() % (strlen(randtext) - 100); // ensure at least 100 chars available
    int rand_len = 49 + rand() % 50; // copy 90 to 99 characters
    strncpy(randusn, randtext + rand_start, rand_len);
    randusn[rand_len] = '\0';
    rand_start = rand() % (strlen(randtext) - 100);
    rand_len = 49 + rand() % 50;
    strncpy(randpwd, randtext + rand_start, rand_len);
    randpwd[rand_len]='\0';

    for(int i=0;i<((strlen(randpwd)<strlen(randusn))?strlen(randpwd):strlen(randusn));i++){
        keystream_out[i]=((int)randusn[i]*(int)randpwd[i])%26;
    }
    return (strlen(randpwd)<strlen(randusn))?strlen(randpwd):strlen(randusn);
}

void namepackets(char packetnames_out[][100],int numpackets,int seed){ //same as before
    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";

    for(int i=0;i<numpackets;i++){
        //we need to name the packets in a recreatable yet seemingly random way so that we can fetch those packets again but the guy cant.
        //name should be dependent only on keystream (which comes from usn and password) and not on packet contents as we should be able to recreate to fetch the right packets.
        srand(seed + i);
        for(int j=0;j<100;j++){
            packetnames_out[i][j]=letters[rand()%strlen(letters)];
        }
        packetnames_out[i][99]='\0';

        char temp[113];
        sprintf(temp,"storage/%s.txt",packetnames_out[i]);
        FILE *pF=fopen(temp,"r");
        if (pF){ //this means file already exists. so we throw error and stop
            printf("FILE ALREADY EXISTS ERROR!: %s",temp);
        }

        for(int k=0;k<i;k++){if(!strcmp(packetnames_out[k],packetnames_out[i])){printf("FILENAME already EXISTS ERROR!");}}

    }
}

//TODO: IMP change this to make it check for any empty directories and fill those first. maybe evne invcrease nubmer of junk somehow?
void namejunk(char packetpaths_out[][513],int numpackets,int *keystream, int len_of_keystream){ //same as before
    //this is the same as the above function but if the file exists, we just generate another name, doesnt matter.
    //cause it doesnt need to be reproducible.
    char packetnames[numpackets][100];
    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";
    unsigned long long int num_unique_to_user=1;
    for(int i=0;i<len_of_keystream;i++){num_unique_to_user*=(keystream[i]+1);} //avoids 0 multiplication.
    num_unique_to_user+=len_of_keystream;

    char subdirs[100][256];
    int num_subdirs = get_subdirectories("storage", subdirs, 100); //gets (up to) 100 subdirs from storage.

    //TODO: check and make sure reproducibility.
    //PROBLEM THIS IS NOT REPRODUCIBLE. MAKE SURE IT DOESNT HAPPEN!!
    if (num_subdirs == 0) { //if no subdirectories found. failsafe. consider changing it to make dirs based on key instead of just using default.
        printf("ERROR: No subdirectories found. Creating 'default'...\n");
        mkdir("storage/default");
        strcpy(subdirs[0], "default");
        num_subdirs = 1;
    }

    for(int i=0;i<numpackets;i++){
        //we need to name the packets in a recreatable yet seemingly random way so that we can fetch those packets again but the guy cant.
        //name should be dependent only on keystream (which comes from usn and password) and not on packet contents as we should be able to recreate to fetch the right packets.
        srand(num_unique_to_user/(keystream[i%len_of_keystream]+1)*(i+1)); //(avoids division/multiplication by 0)
        int unique_name=0; //keeps writing the path till a unique one found. 
        //yes i know this is inefficient, just roll with it for now.
        while(unique_name==0){
            for(int j=0;j<100;j++){
                packetnames[i][j]=letters[rand()%strlen(letters)];
            }
            packetnames[i][99]='\0';

            char temp[513];
            //srand(time(0));
            int subdir_index = rand() % num_subdirs;  //chooses a number and takes subdir[that num]
            sprintf(temp,"storage/%s/%s.txt", subdirs[subdir_index], packetnames[i]); //prints directory name into filepath   

            FILE *pF=fopen(temp,"r");
            if (!pF){ //meaning unique name created.
                unique_name=1;
                strcpy(packetpaths_out[i],temp);
            }
        }
    }
}

void writepacketsintofiles(char packetpaths[][513],int numpacks,char packets[][26],char junkpaths[][513],int numjunk,char junk[][26], int *keystream, int len_of_keystream){ //same as before
    //yes, I know this is V inneficient, just roll with it for this version. consider using a boolean array
    int writtenpackets[numpacks];
    for(int k=0;k<numpacks;k++){writtenpackets[k]=0;}
    int writtenjunk[numjunk];
    for(int k=0;k<numjunk;k++){writtenjunk[k]=0;}
    srand(keystream[((int)junk[2][19])%len_of_keystream]); //gives a random seed
    int countwrittenpacks=0, countwrittenjunk=0;
    while( countwrittenpacks!=numpacks || countwrittenjunk!=numjunk){ 
        //we need to write all packets and junkpackets at random so that attacker cant know if the packet is junk or not, or the order they go in by seeing when it was created.
        //checking_if_all_packets_written
            // countwrittenjunk=0;
            // countwrittenpacks=0;
            // for(int k=0;k<numpacks;k++){if(writtenpackets[k]==1){countwrittenpacks++;}}
            // for(int k=0;k<numjunk;k++){if(writtenjunk[k]==1){countwrittenjunk++;}}
            // if(countwrittenjunk==numjunk && countwrittenpacks==numpacks){break;}
        if(rand()%2==1){ //50/50 chnance of writing junk or packet
            if(countwrittenpacks==numpacks){continue;}
            //packet
            int temp= rand()%numpacks;
            if (writtenpackets[temp]==0){ //this means packet is not empty, so we write packet and make it empty
                writetofile(packetpaths[temp], packets[temp]); //writing
                writtenpackets[temp]=1;
                countwrittenpacks++;
                }
            else{continue;}//packet is empty already, run loop again
        }
        else{
            if(countwrittenjunk==numjunk){continue;}
            //junk
            int temp= rand()%numjunk;
            if (writtenjunk[temp]==0){ //this means packet is not empty, so we write packet and make it empty
                writetofile(junkpaths[temp], junk[temp]); //writing
                writtenjunk[temp]=1;
                countwrittenjunk++;
                }
            else{continue;}//packet is empty already, run loop again
        }
    }

}

void write_metadata_packet(int numpacks, int *key_given, int len_of_key_given, int seed){ //writes one packet containing numpacks.

    char onlyletters[]="abcdefghijklmnopqrstuvwxyz";
    char plaintext[3]="a";
    plaintext[1]=onlyletters[numpacks];
    plaintext[2]='\0';

    char ciphertext[10]={'\0'};
    vigenerre_encrypt(plaintext,ciphertext,key_given,len_of_key_given);
    char cipherjunktext[26];
    char temp[26];
    insertjunkintostream(ciphertext,cipherjunktext);

    //now we write ciphertext into a filename got from seed+1_len_of_key_given;
    char packetname[100];
    char packetpath[513];
    
    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";
    srand(9999*(seed+1+len_of_key_given)); //cause why not
    for(int j=0;j<100;j++){
        packetname[j]=letters[rand()%strlen(letters)];
    }
    packetname[99]='\0';

    char subdirs[100][256];
    int num_subdirs = get_subdirectories("storage", subdirs, 100); //gets (up to) 100 subdirs from storage.

    int subdir_index = (rand()+2) % num_subdirs;  //chooses a number and takes subdir[that num]
    sprintf(packetpath,"storage/%s/%s.txt", subdirs[subdir_index], packetname); //prints directory name into filepath   

    //first two letters are the number of junk characters added at the end for the packing to suit hill cipher usage.
    int counter=7;
    sprintf(temp, "aa%c%caa%c", letters[(int)ceil(strlen(cipherjunktext)/18.0)], cipherjunktext[0], letters[1]); //metadata that tells: num_of_packets, identifying attribute, packet_num
    for(int i=0;i<strlen(cipherjunktext);i++){
        temp[counter++]=cipherjunktext[i];
        if(counter==25){
            printf("ERROR: metadata packet too long!");exit(1);
        }
    }

    srand(9999*(seed+1+len_of_key_given)); //cause why not
    //we want to pad it with unnoticable junk.
    //later we just see the first two chars of the metadata and remove the junk based on that.
    int no_of_junk=25-counter;
    temp[0]=onlyletters[no_of_junk/10]; //so 3 means d, 0 means a etc.
    temp[1]=onlyletters[no_of_junk%10]; //so 3 means d, 0 means a etc.
    for(int k=strlen(temp);k<25;k++){ //now for the junk.
        temp[k]=onlyletters[rand()%26];
    }
    temp[25]='\0';
    writetofile(packetpath,temp); //writing
}

int read_metadata_packet(int *key_given, int len_of_key_given, int seed){
    char cipherjunktext[30];

    char packetname[100];
    char packetpath[513];
    
    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";
    srand(9999*(seed+1+len_of_key_given)); //cause why not
    for(int j=0;j<100;j++){
        packetname[j]=letters[rand()%strlen(letters)];
    }
    packetname[99]='\0';

    char subdirs[100][256];
    int num_subdirs = get_subdirectories("storage", subdirs, 100); //gets (up to) 100 subdirs from storage.

    int subdir_index = (rand()+2) % num_subdirs;  //chooses a number and takes subdir[that num]
    sprintf(packetpath,"storage/%s/%s.txt", subdirs[subdir_index], packetname); //prints directory name into filepath   

    readcontents(packetpath,cipherjunktext);
    //remove your junk
    
    int no_of_junk=( ((int)cipherjunktext[0]-97)*10 + ((int)cipherjunktext[1]-97) );
    for(int i=strlen(cipherjunktext)-no_of_junk;i<strlen(cipherjunktext);i++){cipherjunktext[i]='\0';} //removing junk at end.
    char ciphertext[10]={'\0'};
    char temp[20]={'\0'};
    for(int i=7;i<25;i++){temp[i-7]=cipherjunktext[i];}

    removejunkfromstream(temp,ciphertext);

    char plaintext[10];
    vigenerre_decrypt(ciphertext,plaintext,key_given,len_of_key_given);
    int numpacks=((int)plaintext[1]-97);
    return(numpacks);
    printf("%s",plaintext);

}

void handle_encryption_tasks(char *plaintext, int *key_given, int len_of_key_given, int seed_passed){
    //TODO: CRITICAL: METADATA IS 001A002 AND HILL ENCRYPT IT MEANT TO HANDLE ONLY A-Z. ALSO CONTENTS OF PACKETS ARENT RIGHT.(so instead of 001, have it be aab or smthn like that k?)
    //      SOMEHOW, PACKET CONTENTS ARE ENDING IN .TXT AND ARE TOO LONG. (gemini said it might be improper null termination so fprintf isnt right.)
    char ciphertext[200];
    srand(9999*(seed_passed+1+len_of_key_given)); //cause why not

    vigenerre_encrypt(plaintext,ciphertext,key_given,len_of_key_given);
    //now ciphertext has vigenerre encrypted plaintext

    char cipherjunktext[500];
    insertjunkintostream(ciphertext,cipherjunktext);
    //now ciphertext is filled with junk making it longer and slightly disturbing frequency analysis.

    // now distributing contents based on seed.
    int numpacks=(int)ceil(strlen(cipherjunktext)/18.0);
    if(numpacks>25){printf("ERROR: contents too long.");exit(1);}
    char packets[numpacks][26];
    char packetnames[numpacks][100];
    char packetpaths[numpacks][513]; //maybe make this 313
    char junkpaths[26][513];
    char junk [26][26]; 
    int junkkeystream[100];
    for(int i=0;i<26;i++){for(int j=0;j<26;j++){junk[i][j]='\0';}}

    //TODO: in future, consider putting this also in with writepackets intofiles so that attacker doesnt know which is metadata.
    write_metadata_packet(numpacks,key_given,len_of_key_given,seed_passed); //writes in number of files as one packet. so we can retrieve and read.

    makepackets(cipherjunktext,packets);
    //now packets have the ciphertext with the junk with the metadata(unencrypted)

    char newpackets[numpacks][26];

    int hillkey[26][26];
    //makehillkey(seed_passed,hillkey);
    //hill_encrypt(numpacks,packets,hillkey,newpackets); //TODO: yet to test
    //now newpackets have the final encrypted packets (even metadata is encrypted.)

    int numjunk=makejunk(junk,hillkey);

    int junkkeylen=makejunkkeystream(junkkeystream);

    namepackets(packetnames,numpacks,seed_passed);//needs to be reproducible, based on all seed
    namejunk(junkpaths,numjunk,junkkeystream,junkkeylen); //doenst need to be reproducible, but shouldnt overwrite actual packets.

    getpaths(packetpaths,packetnames,numpacks,seed_passed);

    writepacketsintofiles(packetpaths,numpacks,packets,junkpaths,numjunk,junk,key_given,len_of_key_given);

    printf("Encrypted and Saved.",seed_passed);
}

/*******************************************************************************
 * HIGH-LEVEL USER OPERATIONS
 ******************************************************************************/

/**
 * Handles user signup and initial content encryption
 * @param keystream - Main keystream derived from username/password
 * @param len_of_key - Length of the main keystream
 * @param seed - Unique seed for this user
 * 
 * Process:
 *   1. Prompts for two security questions (mother's name, friend's name)
 *   2. Generates first_key from keystream + answer1
 *   3. Generates second_key from keystream + first_key + answer2
 *   4. Encrypts content using all three keys combined
 *   5. Stores keys hierarchically (each encrypted with previous level)
 * 
 * Storage hierarchy:
 *   - key1: encrypted with keystream, located using seed
 *   - key2: encrypted with keystream+key1, located using seed*seed1
 *   - content: encrypted with keystream+key1+key2, located using seed*seed1*seed2
 */

// Add these functions before the signup() function

/**
 * Reads packet contents from files into memory
 * @param numpacks - Number of packets to read
 * @param packetnames - Array of file paths to read from
 * @param packets_out - Output array to store packet contents
 */
void getpackets(int numpacks, char packetnames[100][513], char packets_out[numpacks][26]){
    for(int i=0;i<numpacks;i++){
        readcontents(packetnames[i],packets_out[i]);
    }
}

/**
 * Extracts ciphertext from packets by removing metadata
 * @param ciphertext_out - Output buffer for extracted ciphertext
 * @param packets - Input packets containing metadata and ciphertext
 * @param numpacks - Number of packets to process
 * Note: Skips first 7 characters (metadata) of each packet
 */
void openpackets(char *ciphertext_out, char packets[][26], int numpacks){ //same as before
    int count=0;
    
    ciphertext_out[0]='\0';
    for(int i=0;i<numpacks;i++){
        
    //remove your junk
    int no_of_junk=( ((int)packets[i][0]-97)*10 + ((int)packets[i][1]-97) );
    //for(int k=strlen(packets[i])-no_of_junk;k<strlen(packets[i]);k++){packets[i][k]='\0';} //removing junk at end.

        for(int j=7;j<strlen(packets[i])-no_of_junk;j++){
            if (packets[i][j]!='\0' && packets[i][j]>=97 && packets[i][j]<=122){
                ciphertext_out[count++]=packets[i][j];
            }
        }
    }

    ciphertext_out[count]='\0';
}

/**
 * Generates packet filenames for retrieval
 * @param packetnames_out - Output array for generated filenames
 * @param seed - Seed for reproducible name generation
 * @param key_given - Encryption key (used for metadata packet)
 * @param len_of_key - Length of the encryption key
 * @return Number of packets to retrieve
 * Note: Reads metadata packet first to determine packet count
 */
int getpacketnames(char packetnames_out[][100],int seed, int *key_given, int len_of_key){
    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";

    int numpackets=read_metadata_packet(key_given,len_of_key,seed);

    for(int i=0;i<numpackets;i++){
        srand(seed+i);
        for(int j=0;j<100;j++){
            packetnames_out[i][j]=letters[rand()%strlen(letters)];
        }
        packetnames_out[i][99]='\0';
    }
    return numpackets;
}

/**
 * Retrieves and decrypts full plaintext from distributed packets
 * @param keystream - Decryption key
 * @param len_of_key - Length of the keystream
 * @param seed - Seed for locating packets
 * @param plaintext_out - Output buffer for decrypted plaintext
 * 
 * Process:
 *   1. Gets packet names using metadata
 *   2. Retrieves packets from storage
 *   3. Optionally decrypts with Hill cipher (currently commented out)
 *   4. Removes junk data
 *   5. Decrypts with Vigenère cipher
 */
void getfullplaintext(int *keystream, int len_of_key, int seed, char *plaintext_out){
    char plaintext[100];
    char cipherjunktext[500];
    char packetnames[100][100];
    char packetpaths[100][513];
    int numpackets=getpacketnames(packetnames,seed,keystream,len_of_key);

    getpaths(packetpaths,packetnames,numpackets,seed);

    char encrypted_packets[numpackets][26];
    
    getpackets(numpackets,packetpaths,encrypted_packets); 

    char packets[numpackets][26];
    int hillkey[26][26];
    //makehillkey(seed,hillkey);
    //hill_decrypt(numpackets,encrypted_packets,hillkey,packets);

    openpackets(cipherjunktext,encrypted_packets,numpackets); //TODO: change this when you implement hill.
    char ciphertext[200]; 
    removejunkfromstream(cipherjunktext,ciphertext);
    vigenerre_decrypt(ciphertext, plaintext, keystream, len_of_key);
    strcpy(plaintext_out,plaintext);
}

/**
 * Handles deletion of encrypted data packets
 * @param keystream - Key used to locate packets
 * @param len_of_key_given - Length of the keystream
 * @param seed - Seed for packet location
 * Note: Deletes both data packets and metadata packet
 */
void handledeletion(int *keystream, int len_of_key_given, int seed){
    char plaintext[100];
    char cipherjunktext[500];
    char packetnames[100][100];
    char packetpaths[100][513];
    int numpackets=getpacketnames(packetnames,seed,keystream,len_of_key_given);

    getpaths(packetpaths,packetnames,numpackets,seed);

    char encrypted_packets[numpackets][26];
    
    deletepackets(packetpaths, numpackets);

    char packetname[100];
    char packetpath[513];

    char letters[]="abcdefghijklmnopqrstuvwxyz0123456789";
    srand(9999*(seed+1+len_of_key_given));
    for(int j=0;j<100;j++){
        packetname[j]=letters[rand()%strlen(letters)];
    }
    packetname[99]='\0';

    char subdirs[100][256];
    int num_subdirs = get_subdirectories("storage", subdirs, 100);

    int subdir_index = (rand()+2) % num_subdirs;
    sprintf(packetpath,"storage/%s/%s.txt", subdirs[subdir_index], packetname);

    if (remove(packetpath) == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error: Unable to delete the file: %s\n",packetpath);
    }
}

void signup(int *keystream, int len_of_key, int seed){
    //note that overflow problems may still occur cause of how everything is [100], so make sure you enter small stuff for this version.
    char first_answer[100];
    int first_key[100];
    char second_answer[100];
    int second_key[100];
    int tempkey[100];
    int tempkeylen;

    char plaintext[100];
    
    inputstring("your mothers full name", first_answer);
    inputstring("the name of any good friend", second_answer);
    //since two people can have the same answer, we use a combination of the keystream (which is from usn and pwd) and the answers to make new keystreams
    // once created and stored, while reading, only needs to be found and used. never made again. 
    // hence this can be anything. in the future, make it more secure.
    int key1len=makefirstanswerkey(keystream, len_of_key, first_answer, first_key, seed); //made using keystream and ans1
    int seed1=getuniquetouserseed(first_key, key1len);
    int key2len=makesecondanswerkey(keystream, first_key, key1len, second_answer, second_key, seed, seed1); //made using keystream and ans1key and ans2
    int seed2=getuniquetouserseed(second_key,key2len);
    for(int i=0;i<100;i++){first_answer[i]='\0';second_answer[i]='\0';}
    //   now the keystream is from usn and password. key1 is from Q1, keystream. key2 is from Q2, keystream, key1.
    //   this kinda doesnt matter though as they are just created once and then stored.

    //locations of key1 is from seed and encryption is from keystream.
    //locations of key2 is from seed1 and seed and encryption is from keystream and key1.
    //locations of contents is from seed2, seed1 and seed and encryption is from keystream, key1 and key2.

    //encrypting contents from keystream, key1 and key2
    inputstring("Contents",plaintext);
    tempkeylen=key2len;
    for(int i=0;i<tempkeylen;i++){
        tempkey[i]=(keystream[i]+first_key[i]+second_key[i])%26;
    }
    handle_encryption_tasks(plaintext, tempkey, tempkeylen, (seed*seed1*seed2));
    for(int i=0;i<100;i++){plaintext[i]='\0'; tempkey[i]=0;} //for safety.

    //encrypting key2 from keystream and key1
    tempkeylen=key1len;
    for(int i=0;i<key2len;i++){plaintext[i]=(char)(second_key[i]+97);}
    plaintext[99]='\0';//praying the length of key is under 100
    for(int i=0;i<tempkeylen;i++){
        tempkey[i]=(keystream[i]+first_key[i])%26;
    }
    handle_encryption_tasks(plaintext, tempkey, tempkeylen, (seed*seed1));
    for(int i=0;i<100;i++){second_key[i]=0;tempkey[i]=0;plaintext[i]='\0';} //for safety.

    //encrypting key1 from keystream 
    for(int i=0;i<key1len;i++){plaintext[i]=(char)(first_key[i]+97);}
    plaintext[99]='\0'; //praying the length of key is under 100
    handle_encryption_tasks(plaintext, keystream, len_of_key, seed);
    for(int i=0;i<100;i++){first_key[i]=0;tempkey[i]=0;plaintext[i]=0;} //for safety.
}

/**
 * Reads encrypted content for a user
 * @param keystream - Main keystream derived from username/password
 * @param len_of_key - Length of the main keystream
 * @param seed - Unique seed for this user
 * 
 * Process:
 *   1. Decrypts key1 using keystream
 *   2. Decrypts key2 using keystream+key1
 *   3. Decrypts and displays content using keystream+key1+key2
 */
void my_read(int *keystream, int len_of_key, int seed){
    char plaintext[100];
    int first_key[100];
    int key1len;
    int second_key[100];
    int key2len;
    int tempkey[100];
    int tempkeylen;
    int seed1;
    int seed2;

    //locations of key1 is from seed and encryption is from keystream.
    //locations of key2 is from seed1 and seed and encryption is from keystream and key1.
    //locations of contents is from seed2, seed1 and seed and encryption is from keystream, key1 and key2.

    //getting decrypted key1 using seed and keystream
    getfullplaintext(keystream, len_of_key, seed, plaintext);
    key1len=strlen(plaintext);
    for(int i=0;i<key1len;i++){first_key[i]=(int)(plaintext[i]-97);}
    seed1=getuniquetouserseed(first_key, key1len);
    for(int i=0;i<100;i++){plaintext[i]='\0';}

    //getting decrypted key2 using seed1 and seed and keystream and key1
    tempkeylen=key1len;
    for(int i=0;i<tempkeylen;i++){tempkey[i]=(keystream[i]+first_key[i])%26;}
    getfullplaintext(tempkey, tempkeylen, (seed*seed1), plaintext);
    key2len=strlen(plaintext);
    for(int i=0;i<key2len;i++){second_key[i]=(int)(plaintext[i]-97);}
    seed2=getuniquetouserseed(second_key,key2len);
    for(int i=0;i<100;i++){plaintext[i]='\0';tempkey[i]=0;}

    //getting decrypted contents using seed2, seed1 and seed and keystream and key1 and key2
    tempkeylen=key2len;
    for(int i=0;i<tempkeylen;i++){tempkey[i]=(keystream[i]+first_key[i]+second_key[i])%26;}
    getfullplaintext(tempkey, tempkeylen, (seed*seed1*seed2), plaintext);
    for(int i=0;i<100;i++){tempkey[i]=0;first_key[i]=0;second_key[i]=0;}
    printf("Contents: %s",plaintext);
}

/**
 * Deletes all encrypted data for a user
 * @param keystream - Main keystream derived from username/password
 * @param len_of_key - Length of the main keystream
 * @param seed - Unique seed for this user
 * 
 * Process:
 *   1. Retrieves all keys (reverse of signup)
 *   2. Deletes content packets
 *   3. Deletes key2 packets
 *   4. Deletes key1 packets
 * Note: Currently has known issues - needs testing
 */
void delete(int *keystream, int len_of_key, int seed){ //TODO: CRITICAL: DOESNT WORK. FIX THIS. ASAP IMP
    char plaintext[100];
    int first_key[100];
    int key1len;
    int second_key[100];
    int key2len;
    int tempkey[100];
    int tempkeylen;
    int seed1;
    int seed2;

    //locations of key1 is from seed and encryption is from keystream.
    //locations of key2 is from seed1 and seed and encryption is from keystream and key1.
    //locations of contents is from seed2, seed1 and seed and encryption is from keystream, key1 and key2.

    //getting decrypted key1 using seed and keystream
    getfullplaintext(keystream, len_of_key, seed, plaintext);
    key1len=strlen(plaintext);
    for(int i=0;i<key1len;i++){first_key[i]=(int)(plaintext[i]-97);}
    seed1=getuniquetouserseed(first_key, key1len);
    for(int i=0;i<100;i++){plaintext[i]='\0';}

    //getting decrypted key2 using seed1 and seed and keystream and key1
    tempkeylen=key1len;
    for(int i=0;i<tempkeylen;i++){tempkey[i]=(keystream[i]+first_key[i])%26;}
    getfullplaintext(tempkey, tempkeylen, (seed*seed1), plaintext);
    key2len=strlen(plaintext);
    for(int i=0;i<key2len;i++){second_key[i]=(int)(plaintext[i]-97);}
    seed2=getuniquetouserseed(second_key,key2len);
    for(int i=0;i<100;i++){plaintext[i]='\0';tempkey[i]=0;}

    //getting decrypted contents using seed2, seed1 and seed and keystream and key1 and key2
    tempkeylen=key2len;
    for(int i=0;i<tempkeylen;i++){tempkey[i]=(keystream[i]+first_key[i]+second_key[i])%26;}
    handledeletion(tempkey, tempkeylen, (seed*seed1*seed2)); //contents deletion

    tempkeylen=key1len;
    for(int i=0;i<tempkeylen;i++){tempkey[i]=(keystream[i]+first_key[i])%26;}
    handledeletion(tempkey, tempkeylen, (seed*seed1)); //key2 deletion

    handledeletion(keystream, len_of_key, seed); //key1 deletion

}

/**
 * Edits existing content by reading, deleting, and re-creating
 * @param keystream - Main keystream derived from username/password
 * @param len_of_key - Length of the main keystream
 * @param seed - Unique seed for this user
 */
void edit(int *keystream, int len_of_key, int seed){
    my_read(keystream, len_of_key, seed);
    printf("\n");
    delete(keystream, len_of_key, seed);
    printf("\n");
    signup(keystream, len_of_key, seed);
    printf("Saved.");
}

/*******************************************************************************
 * UTILITY FUNCTIONS - UI
 ******************************************************************************/

/**
 * Displays the ShatterLock ASCII art title
 */
void printtitle(){
    printf("  _________.__            __    __                 .____                  __     \n");
    printf(" /   _____/|  |__ _____ _/  |__/  |_  ___________  |    |    ____   ____ |  | __ \n");
    printf(" \\_____  \\ |  |  \\\\__  \\\\   __\\   __\\/ __ \\_  __ \\ |    |   /  _ \\_/ ___\\|  |/ / \n");
    printf(" /        \\|   Y  \\/ __ \\|  |  |  | \\  ___/|  | \\/ |    |__(  <_> )  \\___|    <  \n");
    printf("/_______  /|___|  (____  /__|  |__|  \\___  >__|    |_______ \\____/ \\___  >__|_ \\ \n");
    printf("        \\/      \\/     \\/                \\/                \\/          \\/     \\/ \n");
    printf("\n\n");
}

/*******************************************************************************
 * MAIN FUNCTION
 ******************************************************************************/

/**
 * Main entry point
 * Handles user authentication and operation selection (Signup/Read/Edit/Delete)
 */
int main(){
    printtitle();
    char username[100];
    char password[100];
    int keystream[100];
    inputstring("Username",username);
    inputstring("Password",password);

    int len_of_key=makekey(username,password,keystream);
    for(int i=0;i<100;i++){username[i]='\0';password[i]='\0';}
    int seed=getuniquetouserseed(keystream, len_of_key);

    char useless[10];
    fgets(useless,10,stdin); //clears
    
    char choice;
    printf("Enter choice (S for signup, R for read, E for edit, D for delete):\n");
    scanf("%c",&choice);
    switch (choice)
    {
    case 'S':
        signup(keystream,len_of_key, seed);
        break;
    case 'R':
        my_read(keystream,len_of_key, seed);
        break;
    case 'E':
        edit(keystream,len_of_key, seed);
        break;
    case 'D':
        delete(keystream, len_of_key, seed);
        break;
    default:
        printf("Invalid input.");
        break;
    }

    return 0;
}
