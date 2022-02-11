/* 
 * CS:APP Data Lab 
 * 
 * NAME:ghy
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return (~(~x & ~y)) & (~(x & y));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return (1 << 31);
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
  return !((~(x + 1) ^ x) | !(x + 1));
  // return !(x ^ 0x7FFFFFFF);
  // return  !(~(x + 1) ^ x);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  int y = (0xAA << 24 | 0xAA << 16 | 0xAA << 8 | 0xAA);
  return !((x & y) ^ y);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return (~x + 1);
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  // int flag_high = (x & 0xF0) ^ 0x30;
  int flag_high = (x >> 4) ^ 3;
  int flag_low = ((x >> 3) & 1) & (((x >> 2) & 1) | ((x >> 1) & 1));
  return !(flag_high | flag_low); 
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
    int flag = !!x + (~0);

    return ((~flag & y) ^ (flag & z));
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    int symbolX = (x >> 31) & 1;
    int symbolY = (y >> 31) & 1;
    int overflow = symbolX ^ symbolY;

    int subtractionRes = x + (~y + 1);
    int subtractionResEqualZero = !subtractionRes;
    int subtractionResLessThanZero = (subtractionRes >> 31) & 1; 
    // return (((symbolX ^ symbolY) & symbolX) | is_zero | ((res >> 31) & 1)) & 1;
    return ((!overflow) & (subtractionResEqualZero | subtractionResLessThanZero)) | (overflow & symbolX);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  return (((x | (~x + 1)) >> 31) & 1) ^ 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    // x >= 0: symbol = 0000000...
    // x < 0:  symbol = 1111111...
    // 把寻找正数最高位1和负数最高位0的操作统一为寻找最高为1
    int symbol = x >> 31;
    int bit_16, bit_8, bit_4, bit_2, bit_1, bit_0;

    x = (symbol & (~x)) | (~symbol & x);
    
    // 寻找最高位的1
    bit_16 = !!(x >> 16) << 4;
    x >>= bit_16;
    
    bit_8 = !!(x >> 8) << 3;
    x >>= bit_8;

    bit_4 = !!(x >> 4) << 2;
    x >>= bit_4;

    bit_2 = !!(x >> 2) << 1;
    x >>= bit_2;

    bit_1 = !!(x >> 1);
    x >>= bit_1;
    
    bit_0 = x; // 剩余的最高位，为1则加上它本身的位置

    return bit_16 + bit_8 + bit_4 + bit_2 + bit_1 + bit_0 + 1; // +1是符号位
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  unsigned exp = uf & 0x7f800000;
  unsigned frac = uf & 0x7fffff;

  if (exp == 0x7f800000) return uf; // exp全1：无穷 || Nan 直接返回即可
  else if (!exp) frac <<= 1; // exp全0：非规格化数，将frac*2
  else {
    exp += 0x800000;
    if (exp == 0x7f800000) frac = 0; // 处理非规格化数运算后结果为无穷的情况
  }
  return (uf & 0x80000000) | exp | frac;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */

unsigned float_i2f(int x) {
  unsigned unx, tmp_unx;
  int e, exp, frac;
  int cnt; // 统计位数
  int overflow; // 计算得到frac需要移位的数量
  int flag; // 标记舍入是否需要进位
  int i, m;
  
  unx = x;
  if (x < 0) unx = -unx;
  tmp_unx = unx;
  
  cnt = 0;
  while (unx) {
      ++cnt;
      unx >>= 1;
  }

  unx = tmp_unx;
  //if (x < 0) unx = -unx;

  if (!x) {
    exp = 0;
  } else {
      e = cnt - 1;
      exp = (e + 127) << 23;
  }
  //exp &= 0x7f800000; // 这里不会在exp的区域产生多余的数字
  
  flag = 0;
  overflow = 24 - cnt; // 23 - (cnt - 1)
  if (overflow > 0) {
    //unx &= 0x7fffff; // 即使这里左移后23bit以外有其他值，下面的frac&=还会进行处理
    frac = unx << overflow;
  } else {
      overflow = -overflow;
      frac = unx >> overflow;
      if ((unx >> (overflow - 1)) & 1) {
          i = m = 0;
          while (i < overflow - 1) {
              m |= unx >> i;
              ++i;
          }
          if (m & 1) flag = 1; // 超过一半，向上舍入
          else flag = frac & 1; // 恰好一半，向偶数舍入
      }
  }
  frac &= 0x7fffff; // unx在移位后23bit以外可能有无用的1，需要除去 

  return ((x & 0x80000000) | exp | frac) + flag;
}


/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
    int symbol = uf & 0x80000000;
    int exp = uf & 0x7f800000, e;
    int frac = uf & 0x7fffff;
    
    e = (exp >> 23) - 127;
    if (e < 0) return 0;
    if (e >= 31) return 0x80000000u;
    if (e < 23) {
        frac >>= 23 - e;
        frac |= 1 << e;
    } else {
        frac |= 1 << 23;
    }
    
    if (symbol == 0x80000000) frac = -frac; // 如果最后为负数，需要注意存储方式为补码，负数需要取反加一
    return symbol | frac;
}
