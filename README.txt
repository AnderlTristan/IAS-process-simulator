USAGE
    INPUT
        - g++ main.cpp -o run
        - enter program file name
    OUTPUT
        - PRINT operations
        - IORegister content
        - cache hit/miss count
        - memory overwritten on 'results.txt'

TECHNICAL DETAILS
    - main()
        - creates filestream objects for initial memory, program file, and output file
        - initializes a CPU object with a cache size of 10 (will be 10 by default is paramter is ommitted)
        - CPU.initMemory(ifstream) => creates main memory from memory file
        - CPU.fetch(ifstream) => starts instruction cycle from program file
        - CPU.cacheAnalysis => prints cache hits/misses
        - CPU.printMemory(ostream) => prints overwritten memory to file

    - struct MemoryUnit
        - object used to store content of memory addresses and their tag used for caching

    - global function - bitStringToFloat
        - used to convert 32-bit memory address content to IEEE754 float format

    - class CPU
        - private members
            - AC = accumulator register
            - PC = program counter register
            - MQ = multiplier quotient register
            - floatAC/floatMQ = IEEE754 conversion
            - IOtemp - storing registers during interrupts
            - IORegister - single address IO content
            - mainMemory[] - main array of memory addresses
            - cache[], cacheHits, cacheMisses, cacheSize - cache details

        - methods
            - initMemory(ifstream) - stores content from memory file to memory array (array of MemoryUnit structs)
            - fetch(ifstream) - fetches each instruction line from program file and decodes
            - decode(int, int) - checks for cache hits, sends opcode/operand to execution step
            - execute(int, int, bool) - executes instruction based on opcode/operand and gets/stores data in either memory or cache
                - uses switch cases to perform correct operations
            - cacheInsert(MemoryUnit, int) - stores memory content in cache based on hash
            - printMemory(ofstream) - prints overwritten memory
            - cacheAnalysis - prints cache hits/misses
            - handleInterrupt(int, int) - performs IO operation and stores/restores CPU registers
    
*** WHAT DOES NOT WORK ***
    - unable to get error correction working with interrupts (no hamming codes)
    - unable to get float operations to work correctly
        - able to get float from instruction integer, unable to go from IEEE754 format back to integer
    - generateHammingCodeFromValue(int, bool[]) - not used.