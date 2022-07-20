#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// global variables for the program

#define LINES_PER_BANK 16


int registers[16] = {0x0};     /* Our 16 general registers, intially set to 0 */
int verbose = 0;               /* Verbose method which determines output */
int word_count;                /* indicates how many memory words to display at end */
unsigned int ram[0x1000];               /* Our ram to store input data */
int currentInst = 0x0;         /* This is our current instruction address */
int halt = 0;
int operand1 = 0;            /* Our current operand 1 */
int operand2 = 0;            /* Our current operand 2 */
int currentX2 = 0;
int currentB2 = 0;
int firstDisp = 0;
int lastDisp = 0;
int conditionCode = 0;
int registerSum = 0x0;
int registerDiff = 0x0;
int storeVar[4] = {0x0};          /* Specific byte addresses used in our store function */
int wordFormer[0x1000];
int mask = 0x0;
int compareContents = 0x0;
int maskResult = 0x0;
int instructionFetches = 0;
int LRcount = 0;
int CRcount = 0;
int ARcount = 0;
int SRcount = 0;
int LAcount = 0;
int BCTcount = 0;
int BCcount = 0;
int STcount = 0;
int Lcount = 0;
int Ccount = 0;
int memoryDataReads = 0;
int memoryDataWrites = 0;
int BCTtaken = 0;
int BCtaken = 0;
int displacement = 0;

//cache stuff
unsigned int hits;
unsigned int misses;
unsigned int valid[2][LINES_PER_BANK];
unsigned int tag[2][LINES_PER_BANK];
int plru_state[LINES_PER_BANK];
int plru_bank[4] = { 0, 0, 1, 1 };
int lru_bank[LINES_PER_BANK];




/*    MISC FUNCTIONS    */


// This function prints our cache stats
void cache_stats(void){
  printf( "  cache hits          = %d\n", hits );
  printf( "  cache misses        = %d\n", misses );
}


// initialize cache

void cache_init(void){
  int i;
  for(i = 0; i < LINES_PER_BANK; i++){
    lru_bank[i] = 0;
    valid[0][i] = tag[0][i] = 0;
    valid[1][i] = tag[1][i] = 0;
  }
  hits = misses = 0;
}

/* address is incoming word address, will be converted to byte address */
/* type is read (=0) or write (=1) */

void cache_access(unsigned int address){

  unsigned int
    addr_tag,    /* tag bits of address     */
    addr_index,  /* index bits of address   */
    bank;        /* bank that hit, or bank chosen for replacement */

  addr_index = (address >> 3) & 0xf;
  addr_tag = address >> 7;

  /* check bank 0 hit */

  if(valid[0][addr_index] && (addr_tag==tag[0][addr_index])){
    hits++;
    bank = 0;

  /* check bank 1 hit */

  }else if(valid[1][addr_index] && (addr_tag==tag[1][addr_index])){
    hits++;
    bank = 1;

  /* miss - choose replacement bank */
  }else{
    misses++;

         if(!valid[0][addr_index]){ 
            bank = 0;
            lru_bank[addr_index] = 1;
         }
         else if(!valid[1][addr_index]) {
            bank = 1;
            lru_bank[addr_index] = 0;
         }
         else bank = lru_bank[addr_index];

    valid[bank][addr_index] = 1;
    tag[bank][addr_index] = addr_tag;     
  }

        if(bank == 0) lru_bank[addr_index] = 1;
   else if(bank == 1) lru_bank[addr_index] = 0;

}




// This function will grab our full instruction from ram 
// and seperate its components so we can later combine
// to form full address
void dissectInstruction(){

  // Get operand 1
  operand1 = ram[currentInst+1] >> 4;
  currentX2 = ram[currentInst+1] & 0x0f;
  currentB2 = ram[currentInst+2] >> 4;

  // firstDisp and lastDisp make up the full 12 bit displacement
  firstDisp = ram[currentInst+2] & 0x0f;
  lastDisp = ram[currentInst+3];
  displacement = (firstDisp << 8) | lastDisp;
  


}

// This function will make sure we are not currently at an
// address that is out of bounds
void addressCheck(){

  if (currentInst >= 0x1000){
    printf("\nout of range instruction address %x\n", currentInst);
    exit(0);
  }
}


/* initialization routine to read in memory contents */


void load_ram(){
  int i = 0;

  if( verbose ){
    printf( "initial contents of memory arranged by bytes\n" );
    printf( "addr value\n" );
  }
  while( scanf( "%x", &ram[i] ) != EOF ){
    if( i >= 4095 ){
      printf( "program file overflows available memory\n" );
      exit( -1 );
    }
    ram[i] = ram[i] & 0xfff; /* clamp to 12-bit word size */
    if( verbose ) printf( " %03x:  %02x\n", i, ram[i] );
    i++;
  }
  word_count = i;
  for( ; i < 4095; i++ ){
    ram[i] = 0;
  }
  if( verbose ) printf( "\n" );
}




/* RX OPCODE FUNCTIONS */


/* 
41 Load Address - load the 24-bit effective address into R1 and 
set the high 8 bits to zeros (there is no alignment check
nor range check on the address; it is treated as an
immediate value that will be loaded into the register) 
*/
void loadAddress(){
  
  // Get our instruction components 
  dissectInstruction();

  // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;
  operand2 = displacement;

    if (currentX2 > 0) { operand2 += registers[currentX2]; }
    if (currentB2 > 0) { operand2 += registers[currentB2]; }


  // Update our register
  registers[operand1] = operand2;

  // Update our current instruction
  currentInst+=0x04;

}

/*
// 46 Branch on Count - decrement the contents of R1 and branch to
// the effective address if the result is not equal to zero
// (the effective address is calculated before the decrement)
*/
void branchOnCount(){

  // Get our instruction components 
  dissectInstruction();

  // Check for data access alignment 
  if (ram[currentInst+3] % 2 != 0){
    printf("\ninstruction fetch alignment error at %x to %x\n", currentX2, ram[currentInst+3]);
    exit(0);
  }

  // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;
  operand2 = displacement;

    if (currentX2 > 0) { operand2 += registers[currentX2]; }
    if (currentB2 > 0) { operand2 += registers[currentB2]; }

  // Decrement our register
  registers[operand1] = registers[operand1] - 1;


  if (registers[operand1] == 0x0){
    currentInst+=4;
    return;
  }

  BCTtaken++;
  currentInst = operand2;
  


}

/*
47 Branch on Condition - treat the R1 field as a branch mask
and branch to the effective address if the current
condition code value is one of the values selected by
the mask bit(s)
*/
void branchOnCondition(){

  // Get our instruction components 
  dissectInstruction();

  // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;
  operand2 = displacement;

    if (currentX2 > 0) { operand2 += registers[currentX2]; }
    if (currentB2 > 0) { operand2 += registers[currentB2]; }


    maskResult = operand1 & conditionCode;
    //printf("\n\nmask is: %x, and conditionCode is: %x, and maskResult is %x\n\n", mask, conditionCode, maskResult);

    if ((maskResult > 0) |(operand1 == 0xf)){
      BCtaken++;
      currentInst = operand2;
      return;
    }

  currentInst+=4;
}


/*
50 Store - store the contents of R1 into the memory word at the
effective address (align check and range check the address)
*/
void store(){

  // Get our instruction components 
  dissectInstruction();

  // Check for data access alignment 
    if (ram[currentInst+3] % 4 != 0){
      printf("\ndata access alignment error at %x to %x\n", currentX2, ram[currentInst+3]);
      exit(0);
    }

  // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;

  operand2 = (firstDisp << 8) | lastDisp;

  if ((currentX2 > 0) && (currentB2 == 0)){
    operand2 += registers[currentX2];
  }
  else if ((currentX2 == 0) && (currentB2 > 0)){
    operand2 += registers[currentB2];
  }
  else if ((currentX2 > 0) && (currentB2 > 0)){
    operand2 += registers[currentX2] + registers[currentB2];
  }
 
 


  // Setting our bytes into store variables
  storeVar[0] = (registers[operand1] >> 24) & 0xff;
  storeVar[1] = (registers[operand1] >> 16) & 0xff;
  storeVar[2] = (registers[operand1] >> 8) & 0xff;
  storeVar[3] = (registers[operand1]) & 0xff;

  cache_access(operand2);


  // Putting these bytes into our ram
  ram[operand2] = storeVar[0];
  ram[operand2+1] = storeVar[1];
  ram[operand2+2] = storeVar[2];
  ram[operand2+3] = storeVar[3];
  memoryDataWrites++;


  currentInst+=4;

}

/**/
/*
58 Load - load the contents of the memory word at the effective
address into R1 (align check and range check the address)
*/
void load(){

  // Get our instruction components 
  dissectInstruction();
  
  
  // Check for data access alignment 
  if (ram[currentInst+3] % 4 != 0){
    printf("\ndata access alignment error at %x to %x\n", currentX2, ram[currentInst+3]);
    exit(0);
  }
  
  
  // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;
  operand2 = displacement;

    if (currentX2 > 0) { operand2 += registers[currentX2]; }
    if (currentB2 > 0) { operand2 += registers[currentB2]; };

  
  cache_access(operand2);
  

  registers[operand1] = (ram[operand2] << 24) | (ram[operand2+1] << 16) | (ram[operand2+2] << 8) | ram[operand2+3];
  
  memoryDataReads++;
  
  
  
  currentInst+=4;
}


/*
59 Compare – compare the contents of R1 with the contents of
the memory word at the effective address and set the
condition code (align check and range check the address)
*/
void compare(){

  // Get our instruction components 
  dissectInstruction();

  // Check for data access alignment 
    if (ram[currentInst+3] % 4 != 0){
      printf("\ndata access alignment error at %x to %x\n", currentX2, ram[currentInst+3]);
      exit(0);
    }
 
    // Form our operand 2 from instruction components
  //operand2 = (currentX2 << 24) | (currentB2 << 16) | (firstDisp << 8) | lastDisp;
  operand2 = displacement;

    if (currentX2 > 0) { operand2 += registers[currentX2]; }
    if (currentB2 > 0) { operand2 += registers[currentB2]; }

  cache_access(operand2);

  compareContents = (ram[operand2] << 24) | (ram[operand2+1] << 16) | (ram[operand2+2] << 8) | ram[operand2+3];
  memoryDataReads++;

  //printf("\n\n Compare Contents is: %x\n\n", compareContents);
  //printf("\n\n Registers[%d] is: %x\n\n", operand1, registers[operand1]);

    if (registers[operand1] == compareContents){
      conditionCode = 0;
    }
    else if (registers[operand1] > compareContents){
      conditionCode = 2;
    }
    else if (registers[operand1] < compareContents){
      conditionCode = 1;
    }

    currentInst+=4;

}





/* RR OPCODE FUNCTIONS */



/*
18 Load Register - load the contents of R2 into R1
*/
void loadRegister(){

  // Getting our register numbers from the instruction
  operand1 = ram[currentInst+1] >> 4;
  operand2 = ram[currentInst+1] & 0x0f;  

  // Loading R2 into R1
  registers[operand1] = registers[operand2];

  currentInst+=2;


}


/*
19 Compare Register – compare the contents of R1 with the
contents of R2 and set the condition code
*/
void compareRegister(){

  // Getting our register numbers from the instruction
  operand1 = ram[currentInst+1] >> 4;
  operand2 = ram[currentInst+1] & 0x0f;  

    if (registers[operand1] == registers[operand2]){
      conditionCode = 0;
    }
    else if (registers[operand1] > registers[operand2]){
      conditionCode = 2;
    }
    else if (registers[operand1] < registers[operand2]){
      conditionCode = 1;
    }

  currentInst+=2;


}



/*
1A Add Register - add the contents of R2 to the contents of R1,
place the result into R1, and set the condition code
(i.e., R1 = R1 + R2)
*/
void addRegister(){

  // Getting our register numbers from the instruction
  operand1 = ram[currentInst+1] >> 4;
  operand2 = ram[currentInst+1] & 0x0f;  

  // Getting our sum of the two registers
  registerSum = registers[operand1] + registers[operand2];

  // Updating our register with the sum
  registers[operand1] = registerSum;

  currentInst+=2;


  // Update condition code
  if (registerSum == 0x0){
    conditionCode = 0;
  }
  else if (registerSum < 0x0){
    conditionCode = 1;
  }
  else {
    conditionCode = 2;
  }

}


/*
1B Subtract Register - subtract the contents of R2 from the
contents of R1, place the result into R1, and set the
condition code (i.e., R1 = R1 – R2)
*/
void subtractRegister(){

  // Getting our register numbers from the instruction
  operand1 = ram[currentInst+1] >> 4;
  operand2 = ram[currentInst+1] & 0x0f;  

  // Getting our sum of the two registers
  registerDiff = registers[operand1] - registers[operand2];

  // Updating our register with the sum
  registers[operand1] = registerDiff;

  currentInst+=2;


  // Update condition code
  if (registerDiff == 0x0){
    conditionCode = 0;
  }
  else if (registerDiff < 0x0){
    conditionCode = 1;
  }
  else {
    conditionCode = 2;
  }


}








int main( int argc, char **argv ){


  /* VERBOSE CHECK*/
    if( argc > 1 ){
    if( ( argv[1][0] == '-' ) &&
        (( argv[1][1] == 'v' ) || ( argv[1][1] == 'V' )) ){

      verbose = 1;

    }else{
      printf( "usage: either %s or %s -v with input taken from stdin\n",
        argv[0], argv[0] );
      exit( -1 );
    }
  }

  printf("\nbehavioral simulation of S/360 subset\n");

  if( verbose ){
    printf( "\n(memory is limited to 4096 bytes in this simulation)\n" );
    printf( "(addresses, register values, and memory values are shown in hexadecimal)\n\n" );
  }

    load_ram();


  if( verbose ){
    printf( "initial pc, condition code, and register values are all zero\n\n" );
    printf( "updated pc, condition code, and register values are shown after\n each instruction has been executed\n" );
  }  




  while (halt != 1) {
    switch(ram[currentInst]) {

      case 0x41 :
      
        loadAddress();
        LAcount++;
        instructionFetches++;
          if(verbose){
            printf("\nLA instruction, operand 1 is R%x, operand 2 at address %06x\n", operand1, operand2);
            printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
          }
        break;

      case 0x58 :

        load();
        Lcount++;
        instructionFetches++;
        if(verbose){
          printf("\nL instruction, operand 1 is R%x, operand 2 at address %06x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;


      case 0x1A :

        addRegister();
        ARcount++;
        instructionFetches++;
        if(verbose){
          printf("\nAR instruction, operand 1 is R%x, operand 2 is R%x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x1B :

        subtractRegister();
        SRcount++;
        instructionFetches++;
        if(verbose){
          printf("\nSR instruction, operand 1 is R%x, operand 2 is R%x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x50 :
        store();
        STcount++;
        instructionFetches++;
        if(verbose){
          printf("\nST instruction, operand 1 is R%x, operand 2 at address %06x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x46 :

        branchOnCount();
        BCTcount++;
        instructionFetches++;
        if(verbose){
          printf("\nBCT instruction, operand 1 is R%x, branch target is address %06x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x18 :

        loadRegister();
        LRcount++;
        instructionFetches++;
        if(verbose){
          printf("\nLR instruction, operand 1 is R%x, operand 2 is R%x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x19 :

        compareRegister();
        CRcount++;
        instructionFetches++;
        if(verbose){
          printf("\nCR instruction, operand 1 is R%x, operand 2 is R%x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x59 :

        compare();
        Ccount++;
        instructionFetches++;
        if(verbose){
          printf("\nC instruction, operand 1 is R%x, operand 2 at address %06x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      case 0x47 :

        branchOnCondition();
        BCcount++;
        instructionFetches++;
        if(verbose){
          printf("\nBC instruction, mask is %x, branch target is address %06x\n", operand1, operand2);
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;

      
      default:

        halt = 1;
        currentInst+=2;
        if(verbose){
          printf("\nHalt encountered\n");
          printf("instruction address = %06x, condition code = %x\n", currentInst, conditionCode);
        }
        break;
      
    }

    if(verbose){
      printf ("R0 = %08x, R4 = %08x, R8 = %08x, RC = %08x\n", registers[0], registers[4], registers[8], registers[12]);
      printf ("R1 = %08x, R5 = %08x, R9 = %08x, RD = %08x\n", registers[1], registers[5], registers[9], registers[13]);
      printf ("R2 = %08x, R6 = %08x, RA = %08x, RE = %08x\n", registers[2], registers[6], registers[10], registers[14]);
      printf ("R3 = %08x, R7 = %08x, RB = %08x, RF = %08x\n", registers[3], registers[7], registers[11], registers[15]);
    }
    
    addressCheck();
   
  }  

 
  if (verbose){
    printf("\nfinal contents of memory arranged by words\n");
    printf("addr value\n");

    int i = 0x0;

      while (i < word_count){
        printf("%03x: %02x%02x%02x%02x\n", i, ram[i],ram[i+1],ram[i+2],ram[i+3]);
        i = i + 4;
      }
  }

  printf("\nexecution statistics\n");
  printf("  instruction fetches = %d\n", instructionFetches);
  printf("    LR  instructions  = %d\n", LRcount);
  printf("    CR  instructions  = %d\n", CRcount);
  printf("    AR  instructions  = %d\n", ARcount);
  printf("    SR  instructions  = %d\n", SRcount);
  printf("    LA  instructions  = %d\n", LAcount);
  printf("    BCT instructions  = %d", BCTcount );
    if( BCTcount > 0 ){
      printf( ", taken = %d (%.1f%%)\n", BCTtaken,
        100.0*((float)BCTtaken)/((float)BCTcount) );
    }else{
      printf("\n");
    }
  printf("    BC instructions   = %d", BCcount );
    if( BCcount > 0 ){
      printf( ", taken = %d (%.1f%%)\n", BCtaken,
        100.0*((float)BCtaken)/((float)BCcount) );
    }else{
      printf("\n");
    }
  printf("    ST  instructions  = %d\n", STcount);
  printf("    L   instructions  = %d\n", Lcount);
  printf("    C   instructions  = %d\n", Ccount);

  printf("  memory data reads   = %d\n", memoryDataReads);
  printf("  memory data writes  = %d\n", memoryDataWrites);
  cache_stats();

  


  return 0;  
}


