#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define uint unsigned int
#define uchar unsigned char
#define bool unsigned char
#define true 1
#define false 0
#define allocate(type,name,size) type name = (type)malloc(size); if (NULL == name) { printf("\nFailed to allocate %llu bytes for %s\n", size, #name); exit(1);}

uint* primeseek(uint ptop, uint * primecnt) { //returns a pointer to a prime array, and its size
    // first prime: array[0], last prime array[primecnt-1])

#define STAMPLENGTH 6 // optimal value. Higher would decrease CPU operations but its cache wouldn't fit into L2 cache
    const uint FIRSTPRIMES[STAMPLENGTH + 1] = { 2,3,5,7,11,13,17 };
#define PRIMECNTMAX 5
    const uint STAMPSIZE = 2 * 3 * 5 * 7 * 11 * 13; // Product of first STAMPLENGTH primes
    const uint M = 1000000;
    const uint PRIMELIMITS[PRIMECNTMAX] = { 100000, M, 10 * M, 100 * M, 1000 * M };
    const uint PRIMERATIO[PRIMECNTMAX] = { 10, 12,15, 17, 19 }; // prime counting function says that if ptop > PRIMELIMITS[n], then the number of primes is less than ptop/PRIMERATIO[n]
    const uint SIEVEMAXSIZE = 32 * 1024 * 8;
    const uchar bitsetmask[8] = { 0b11111110,0b11111101,0b11111011,0b11110111,0b11101111,0b11011111,0b10111111,0b01111111 };
    const uchar bitgetmask[16] = { 1,1,2,2,4,4,8,8,16,16,32,32,64,64,128,128 };

    allocate(bool*, stamp, STAMPSIZE * sizeof(bool))
    uint stampjumpsize = 1;
    for (uint i = 1; i <= STAMPSIZE; i++) {
        bool newp = 1;
        for (uint j = 0; j < STAMPLENGTH; j++) if (i % FIRSTPRIMES[j] == 0) {
            newp = 0;
            break;
        }
        stamp[i - 1] = newp;
        if (newp) stampjumpsize++;
    }
    allocate(uint*, stampjump, stampjumpsize * sizeof(int) + sizeof(int))
    stampjump[0] = 1;
    uint stampposition = 0;
#pragma warning(push)
#pragma warning(disable:6386)
    for (uint i = 0; i < STAMPSIZE; i++) if (stamp[i]) stampjump[++stampposition] = 1; else ++(stampjump[stampposition]);
#pragma warning(pop)
    stampjump[stampjumpsize] = 0;
    free(stamp);

    allocate(uint*, stampjumpsleft, stampjumpsize * sizeof(int))
    stampjumpsleft[stampjumpsize - 1] = 0;
    for (uint i = stampjumpsize - 1; i > 0; i--) stampjumpsleft[i - 1] = stampjumpsleft[i] + stampjump[i];

    uint primememsize;
    for (uint i = 0; i < PRIMECNTMAX; i++) if (ptop >= PRIMELIMITS[i]) primememsize = ptop / PRIMERATIO[i];
    if (primememsize < stampjumpsize) primememsize = stampjumpsize;
    if (primememsize < PRIMELIMITS[0] / 2) primememsize = PRIMELIMITS[0] / 2;
    allocate(uint*, primearray, primememsize * sizeof(int));
    for (uint i = 0; i < STAMPLENGTH; i++) primearray[i] = FIRSTPRIMES[i];

    stampposition = 1;
    uint pmax = STAMPLENGTH - 1;
    uint possibleprime = FIRSTPRIMES[STAMPLENGTH];
    const uint lastknownprimesq = primearray[STAMPLENGTH - 1] * primearray[STAMPLENGTH - 1];
    while (possibleprime < lastknownprimesq) {
        primearray[++pmax] = possibleprime;
        possibleprime += stampjump[++stampposition];
    }

    allocate(uchar*, sieve, sizeof(uchar) * SIEVEMAXSIZE / 8);    
    while (possibleprime <= ptop) {
        const uint sievebottom = possibleprime;
        uint sievesize = (primearray[pmax] < sqrt(ptop)) ? primearray[pmax] * primearray[pmax] - sievebottom : ptop - sievebottom;
        if (sievesize > SIEVEMAXSIZE * 2 - 1) sievesize = SIEVEMAXSIZE * 2 - 1;
        uint sievetop = sievesize + sievebottom;
        const uint sievetopsqrt = (uint)sqrt(sievetop++) + 1;
        const uint sievesizehalf = sievesize / 2 + 1;
        primearray[pmax + 1] = sievetopsqrt;
        memset(sieve, 255, SIEVEMAXSIZE / 8);

        for (uint j = STAMPLENGTH; (primearray[j] < sievetopsqrt); j++) {
            const uint actualprime = primearray[j];
            uint i = sievebottom % actualprime;
            if (i > 0) {
                i = actualprime - i;
                if (1 == (i & 1)) i += actualprime;
                i = i >> 1;
            }
            while (i < sievesizehalf) {
                sieve[i >> 3] &= bitsetmask[i & 7];
                i += actualprime;
            }
        }

#define readaprime {primearray[++pmax] = possibleprime; \
                uint i = (possibleprime - sievebottom);\
                if (!(sieve[i >> 4] & bitgetmask[i & 14])) --pmax;\
                possibleprime += stampjump[++stampposition];}
        
        while (((sievetop - possibleprime) > stampjumpsleft[stampposition]) && (sievetop > possibleprime)){
            while (stampposition < stampjumpsize) readaprime 
            stampposition = 1;
            possibleprime += stampjump[stampposition];
        }
        while (possibleprime < sievetop) readaprime
    }
    
    free(stampjump);
    free(sieve);
    free(stampjumpsleft);
    *primecnt = pmax + 1;
    return(primearray);
}


uint timedrun(const uint ptop, const uint verify) { //times primeseek, checks if the result is correct, returns time}

    clock_t timingstart = clock();
    uint primesfound;
    uint* primes = primeseek(ptop, &primesfound);
    if (0 == primes) exit(1);
    if ((verify > 0) && (primesfound != verify)) {
        printf("\nWRONG RESULT, %lu found, %lu expected up to %lu\n", primesfound, verify, primes[primesfound - 1]);
        exit(1);
    }
    if (verify == 0) printf("\nUnable to verify, %lu found, up to %lu\n", primesfound, primes[primesfound - 1]);
    clock_t timingend = clock();
    free(primes);
    return((uint)(((double)(timingend - timingstart) * 1000)) / CLOCKS_PER_SEC);
    return(0);
}

void printprefixed(const uint num, const uchar width) {
    // rounds an int to the nearest prefixed number where the rounding error is less than 1%, 
    // adds leading spaces tp print it in width
    const uchar codes[4] = { 'B','M','K',' ' };
    uchar str[256]=" ";
    const uint limits[5] = { 1000000000,1000000,1000,1,1 }; // last 2 must be 1
    uint i;
    for (i = 0; i < 4; i++) if ((limits[i] <= num) && ((num % limits[i]) < (limits[i + 1] * 10))) break;
    const int strlength=sprintf_s(str,255,"%lu%c", num / limits[i], codes[i]);
    if (-1 == strlength) {
        printf("Error in printprefixed processing number %lu\n", num);
        exit(1);
    }
    for (i = strlength; i < (uint)width; i++) putchar(' ');
    printf("%s", str);
}
         
int main(void) {
    const int selecttop = -1; //0-10: selects top value from primetop. -1: for scaling test over all of them
    #define MAXSELECTTOP 11
    const uint M = 1000000;
    const uint PRIMETOP[MAXSELECTTOP] = { 100000, 300000, M, 3 * M, 10 * M,30 * M,100 * M,300 * M,1000 * M, 2000*M, 4294*M };
    const uint VERIFYRES[MAXSELECTTOP] = { 9592,25997,78498, 216816, 664579, 1857859, 5761455, 16252325, 50847534,98222287,203236859};
    const uint MAXRUNS[MAXSELECTTOP] = {31, 21, 17, 15, 13, 11, 9, 7, 5, 3, 3};

#ifdef _DEBUG
    printf("DEBUG MODE");
    uint realselecttop = ((selecttop >= 0) && (selecttop < MAXSELECTTOP)) ? realselecttop = selecttop : 3;
    printf(", seeking primes up to ");
    printprefixed(PRIMETOP[realselecttop], 0);
    printf("\nDebugged run time: %lu miliseconds.\n ", timedrun(PRIMETOP[realselecttop], VERIFYRES[realselecttop]));

#else
    uint scaletimes[MAXSELECTTOP];
    printf("Seeking primes\n");
    if (selecttop > MAXSELECTTOP) {
        printf("Wrong selecttop: %d", selecttop);
        return(1);
    }
    printf("For accurate timing, we must wait for processor to finish load from compiling. Press ENTER continue");
    uchar c = getc(stdin);
    
    uint maxscalerun = 1;
    if (selecttop < 0) {
        maxscalerun = MAXSELECTTOP;
        printf(" Scaling test");
    }
    for (uint scalerun = 0; scalerun < maxscalerun; scalerun++) {
        uint times[31];
        const uint realselecttop = (selecttop < 0) ? scalerun : selecttop;
         for (uint runs = 0; runs < MAXRUNS[realselecttop]; runs++) {
            times[runs] = timedrun(PRIMETOP[realselecttop], VERIFYRES[realselecttop]);
            printf(".");
        }
        bool unsorted;
        do {
            unsorted = false;
            for (uint j = 0; j < MAXRUNS[realselecttop] - 1; j++) if (times[j] < times[j + 1]) {
                int ii = times[j];
                times[j] = times[j + 1];
                times[j + 1] = ii;
                unsorted = true;
            }
        } while (unsorted);
        printf("\nSeeking up to:");
        printprefixed(PRIMETOP[realselecttop],8);
        printf(" Minimum time: %lu miliseconds. Median time: %lu miliseconds.", times[MAXRUNS[realselecttop] - 1], times[MAXRUNS[realselecttop] / 2]);   
        scaletimes[scalerun] = times[MAXRUNS[realselecttop] - 1];
    }
    printf("\n");
   // for (uint scalerun = 0; scalerun < maxscalerun; scalerun++) printf("%lu\n", scaletimes[scalerun]);
#endif
    return(0);
}