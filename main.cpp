#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

using namespace std;

// opcodes
#define LOAD 1
#define LOAD_MQ 9
#define STORE 33
#define MQ_TO_AC 10
#define ADD 5
#define ADD_FLOAT 7
#define SUBTRACT 6 
#define SUBTRACT_FLOAT 8
#define MULTIPLY 11
#define MULTIPLY_FLOAT 13
#define DIVIDE 12
#define DIVIDE_FLOAT 14
#define LOAD_IO 2
#define STORE_IO 3
#define PRINT 4

// object used to store each memory location
struct MemoryUnit {
    int content;
    int tag;
};

/*
* function_identifier: convert int memory unit to float value (IEEE 754)
* parameters: memory content
* return value: converted float
*/
float bitStringToFloat(int mem) {
    // Convert decimal to binary array
    int temp = mem;
    int binaryNum[32]; 
    int i = 0; 
    while (i < 32) { 
        binaryNum[i] = temp % 2; 
        temp = temp / 2; 
        i++; 
    }
    // Store exponent part
    int exp = 0;
    int z1 = 8;
    int z2 = 8;
    for (int j = 23; j < i - 1; j++) {
        exp = exp + binaryNum[j] * pow(2, (z1 - z2));
        z2--;
    }
    // Fix biased exponent
    exp = exp - 127;
    // Calculate mantissa from from bits 1-23
    float sum = 0.0;
    float expSum = 0.0;
    for(int i = 0; i < 23; i++) {
        sum += (mem % 2);
        sum /= 2.0;
        mem /= 2;
    }
    sum += 1.0;
    // Calculate final float
    sum = sum * pow(2, exp);
    return sum;
}

class CPU {
    public:
        CPU() {
            // Allocating default cache
            cacheSize = 10;   
            cacheHits = 0;
            cacheMisses = 0;
            cache = new MemoryUnit [cacheSize];    
            // Setting tags to -1 to check if cache location is hit/miss 
            for(int i = 0; i < cacheSize; i++) cache[i].tag = -1;
        }
        CPU(int size) {
            // Allocating cache based on parameters
            cacheSize = size;
            cacheHits = 0;
            cacheMisses = 0;
            cache = new MemoryUnit [cacheSize]; 
            // Setting tags to -1 to check if cache location is hit/miss 
            for(int i = 0; i < cacheSize; i++) cache[i].tag = -1;
        }
        /*
        * function_identifier: writes to memory array from memory file
        * parameters: generated memory file object
        * return value: none
        */
        void initMemory(ifstream& memory) {
            for(int i = 0; i < 524288; i++) {
                memory >> mainMemory[i].content; // store input into memory
            }
        }
        /*
        * function_identifier: gets IAS instructions from program  file
        * parameters: IAS program file object
        * return value: none
        */
        void fetch(ifstream& program) {
            PC = 0;
            bool isInterrupt = false; // for IO operations
            int instruction; // EX : 16777225
            while(program >> instruction) {
                int opcode = instruction >> 24; // opcode = first 24 bits
                int operand = instruction & 0x00FFFFFF; // operand = everything after opcode

                decode(opcode, operand);

                if(opcode == LOAD_IO || opcode == STORE_IO) {
                    handleInterrupt(opcode, operand);
                }

                // increment program counter
                PC++;
            }
        }
        /*
        * function_identifier: getting hamming code from a value
        * parameters: value to be checked
        * return value: none
        */
        void handleInterrupt(int opcode, int operand) {
            // store AC/MQ
            switch(opcode) {
                case STORE_IO:
                    IORegister = AC;
                    AC = IOtemp; // restore register
                    break;
                case LOAD_IO:
                    IOtemp = AC; // temporarily store AC
                    AC = IORegister;
                    break;
            }
            cout << "IORegister = " << IORegister << endl;
        }
        void generateHammingCodeFromValue(int value, bool hammingCode[]) {
            int hammingIndex = 2; 

            for(int i = 0; i < 8; i++) {
                while(hammingIndex) {
                    if((hammingIndex - 1) == 0)
                        hammingIndex++;
                }

                hammingCode[hammingIndex] = value % 2;
                value /= 2;
            }

            // Calculate check bits
            bool tempCheckBit;
            for(int i = 1; i < 16; i *= 2) {
                // generate powers of 2..
                // traverse through hamming code array
                tempCheckBit = false;
                for(int j = i; j < 12; j++) {
                    // Data bit to start counting
                    for(int k = j; k < i; k++) {
                        // How many data bits we've counted
                        if(k == i + 1) {
                            // If this is the check bit's block, shift 1
                            k++;
                        }

                        tempCheckBit ^= hammingCode[k - 1];
                    }

                    hammingCode[i - 1] = tempCheckBit;
                }
            }
        }
        /*
        * function_identifier: decodes instruction and checks if operand hits/misses cache
        * parameters: fetched opcode/operand integers
        * return value: none
        */
        void decode(int opcode, int operand) {
            // bool to store whether or not cache was hit 
            bool cacheHit;

            // Check if cache is hit
            if(cache[operand % cacheSize].tag == -1) {
                // miss - insert into cache
                cacheMisses++;
                cacheHit = false;

                // create new MemoryUnit to store in cache if there is a miss
                MemoryUnit cacheItem;
                cacheItem.content = mainMemory[operand].content; // store from main memory into cache index
                cacheItem.tag = operand; // set tag for future tag hits at this index

                // store in cache
                cacheInsert(cacheItem, operand);
            } else if(operand != cache[operand % cacheSize].tag) {
                // miss - insert into cache
                cacheMisses++;
                cacheHit = false;

                // overwrite cache at this location
                MemoryUnit cacheItem;
                cacheItem.content = mainMemory[operand].content; // store from main memory into cache index
                cacheItem.tag = operand; // set tag for future tag hits at this index
                cacheInsert(cacheItem, operand);
            } else {
                // cache hit
                cacheHits++;
                cacheHit = true;
            }
            
            // Execute instruction
            execute(opcode, operand, cacheHit);
        }
        /*
        * function_identifier: uses IAS opcode/operand from instruction to execute operation
        * parameters: fetched opcode/operand integers
        * return value: none
        */
        void execute(int opcode, int operand, bool cacheHit) {
            // find instruction type
            // process operands based on instruction
            // ** if cache hit, get memory from cache. else, get from main memory
            if(!cacheHit) {
                // cache missed, get data from main memory
                switch(opcode) {
                    case MQ_TO_AC: // LOAD MQ
                        AC = MQ;
                        break;
                    case LOAD_MQ: // LOAD MQ,M(X)
                        MQ = mainMemory[operand].content;
                        MQ = cache[operand % cacheSize].content;
                        break;
                    case STORE: // STOR M(X)
                        mainMemory[operand].content = AC; // store AC to [x]
                        cache[operand % cacheSize].content = AC;
                        break;
                    case LOAD: // LOAD M(X) 
                        AC = mainMemory[operand].content; // transfer M(X) to AC
                        AC = cache[operand % cacheSize].content;
                        MQ = mainMemory[operand].content;
                        MQ = cache[operand % cacheSize].content;
                        break;
                    case ADD: // ADD M(X)
                        AC += mainMemory[operand].content; // AC = M(X) + AC
                        break;
                    case SUBTRACT: // SUB M(X)
                        AC = AC - mainMemory[operand].content; // AC = AC - M(X)
                        break;
                    case MULTIPLY: // MUL M(X)
                        MQ = MQ * mainMemory[operand].content;
                        AC = MQ & 0x000FFF; // AC = most significant bits of MQ
                        break;
                    case DIVIDE: // DIV M(X)
                        MQ = AC / mainMemory[operand].content;
                        AC = MQ & 0x000FFF; // AC = most significant bits of MQ
                        break;
                    case ADD_FLOAT:
                        floatAC = bitStringToFloat(AC);
                        floatAC += bitStringToFloat(mainMemory[operand].content);
                        cout << floatAC << endl;
                        break;
                    case SUBTRACT_FLOAT:
                        floatAC = bitStringToFloat(AC);
                        floatAC -= bitStringToFloat(mainMemory[operand].content);
                        cout << floatAC << endl;
                        break;
                    case MULTIPLY_FLOAT:
                        floatMQ = bitStringToFloat(MQ);
                        floatAC = bitStringToFloat(AC);
                        floatMQ *= bitStringToFloat(mainMemory[operand].content);
                        floatAC = floatMQ; // AC = most significant bits of MQ
                        cout << floatAC << endl;
                        break;
                    case DIVIDE_FLOAT:
                        floatMQ = bitStringToFloat(MQ);
                        floatAC = bitStringToFloat(AC);
                        floatMQ = floatAC / bitStringToFloat(mainMemory[operand].content);
                        floatAC = floatMQ; // AC = most significant bits of MQ
                        cout << floatAC << endl;
                        break;
                    case PRINT:
                        cout << mainMemory[operand].content << endl;
                        break;
                }
            } else {
                // cache hit .. get data from cache
                // use hash to find correct row 
                int hash = operand % cacheSize;
                switch(opcode) {
                    case MQ_TO_AC: // LOAD MQ
                        AC = MQ;
                        break;
                    case LOAD_MQ: // LOAD MQ,M(X)
                        MQ = cache[hash].content;
                        break;
                    case STORE: // STOR M(X)
                        cache[hash].content = AC; // store AC to [x]
                        break;
                    case LOAD: // LOAD M(X)
                        AC = cache[hash].content; // transfer M(X) to AC
                        MQ = cache[hash].content;
                        break;
                    case ADD: // ADD M(X)
                        AC += cache[hash].content; // AC = M(X) + AC
                        break;
                    case SUBTRACT: // SUB M(X)
                        AC = AC - cache[hash].content; // AC = AC - M(X)
                        break;
                    case MULTIPLY: // MUL M(X)
                        MQ = MQ * cache[hash].content;
                        AC = MQ & 0x000FFF; // AC = most significant bits of MQ
                        break;
                    case DIVIDE: // DIV M(X)
                        MQ = AC / cache[hash].content;
                        AC = MQ & 0x000FFF; // AC = most significant bits of MQ
                        break;
                    case ADD_FLOAT:
                        floatAC = bitStringToFloat(AC);
                        floatAC = floatAC + bitStringToFloat(cache[hash].content);
                        cout << floatAC << endl;
                        break;
                    case SUBTRACT_FLOAT:
                        floatAC = bitStringToFloat(AC);
                        floatAC = floatAC - bitStringToFloat(cache[hash].content);
                        cout << floatAC << endl;
                        break;
                    case MULTIPLY_FLOAT:
                        floatMQ = bitStringToFloat(MQ);
                        floatAC = bitStringToFloat(AC);
                        floatMQ *= bitStringToFloat(mainMemory[operand].content);
                        floatAC = floatMQ; // AC = most significant bits of MQ
                        cout << floatAC << endl;
                        break;
                    case DIVIDE_FLOAT:
                        floatMQ = bitStringToFloat(MQ);
                        floatAC = bitStringToFloat(AC);
                        floatMQ = floatAC / bitStringToFloat(mainMemory[operand].content);
                        floatAC = floatMQ; // AC = most significant bits of MQ
                        cout << floatAC << endl;
                        break;
                    case PRINT:
                        cout << cache[hash].content << endl;
                        break;
                }
            }
        }
        /*
        * function_identifier: inserts new item into cache
        * parameters: item to be added to cache
        * return value: none
        */
       void cacheInsert(MemoryUnit item, int operand) {
           cache[operand % cacheSize] = item;
       }
        /*
        * function_identifier: writes overwritten memory to output file
        * parameters: output file object
        * return value: none
        */
        void printMemory(ofstream& outfile) {
            for(int i = 1; i < 524288; i++) {
                outfile << mainMemory[i].content << endl;
            }
        }
        /*
        * function_identifier: output cache performance
        * parameters: none
        * return value: none
        */
        void cacheAnalysis() {
            cout << "Cache hits: " << cacheHits << endl;
            cout << "Cache misses: " << cacheMisses << endl;
        }
    private:
        int AC; // accumulator
        int PC; // program counter
        int MQ;
        float floatAC;
        float floatMQ;
        int IOtemp;
        int IORegister;
        MemoryUnit mainMemory[524288]; // 2^24 bits
        // CACHE MEMBERS
        int cacheHits;
        int cacheMisses;
        int cacheSize; // number of cache rows
        MemoryUnit * cache; // array of vectors used for cache storage
};

int main() {
    ifstream memory;
    memory.open("memoryinit_ex.txt");
    ifstream program;
    string filename;
    cout << "Enter filename: " << endl;
    cin >> filename;
    program.open(filename);
    ofstream outfile;
    outfile.open("result.txt");

    CPU IASTest(10);
    IASTest.initMemory(memory);
    IASTest.fetch(program);
    IASTest.cacheAnalysis();
    IASTest.printMemory(outfile);

    memory.close();
    outfile.close();

    return 0;
}