/*
This file is part of Alpertron Calculators.

Copyright 2016 Dario Alejandro Alpern

Alpertron Calculators is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Alpertron Calculators is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Alpertron Calculators.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <math.h>
#include "bignbr.h"
#include "expression.h"
#include "factor.h"

// These defines are valid for factoring up to 10^110.
#define MAX_NBR_FACTORS 13
#define MAX_PRIMES 150000
#define MAX_LIMBS_SIQS 25
#define LENGTH_OFFSET 0
typedef struct
{
  int value;
  int modsqrt;
  int Bainv2[MAX_NBR_FACTORS];
  int Bainv2_0;
  int soln1;
  int difsoln;
} PrimeSieveData;

typedef struct
{
  int value;
  int exp2;
  int exp3;
  int exp4;
  int exp5;
  int exp6;
} PrimeTrialDivisionData;

unsigned char SIQSInfoText[300];
int numberThreads;
extern int NumberLength;
int matrixBLength;
long trialDivisions;
long smoothsFound;
long totalPartials;
long partialsFound;
long ValuesSieved;
int nbrPrimes;
int congruencesFound;
long polynomialsSieved;
int nbrPartials;
int multiplier;
int nbrFactorsA;
long afact[MAX_NBR_FACTORS];
long startTime;
int TestNbr2[MAX_LIMBS_SIQS];
int biQuadrCoeff[MAX_LIMBS_SIQS];
int biLinearDelta[MAX_LIMBS_SIQS][MAX_LIMBS_SIQS];
static long largePrimeUpperBound;
static unsigned char logar2;
static int aindex[MAX_NBR_FACTORS];
//  static Thread threadArray[];
static PrimeSieveData primeSieveData[MAX_PRIMES+3];
static PrimeTrialDivisionData primeTrialDivisionData[MAX_PRIMES];
static int span;
static int indexMinFactorA;
static int threadNumber;
static int nbrThreadFinishedPolySet;
static long oldSeed;
static long newSeed = 0;
static int NbrPolynomials;
static int SieveLimit;
static int matrixPartial[MAX_PRIMES * 8][MAX_LIMBS_SIQS/2+4];
static int matrixPartialLength;
static int vectLeftHandSide[MAX_PRIMES+50][MAX_LIMBS_SIQS];
static int matrixPartialHashIndex[2048];
static int matrixB[MAX_PRIMES + 50][MAX_LIMBS_SIQS];
static int amodq[MAX_NBR_FACTORS];
static int tmodqq[MAX_NBR_FACTORS];
static unsigned char threshold;
static int smallPrimeUpperLimit;
static int firstLimit;
static int secondLimit;
static int thirdLimit;
static int vectExpParity[MAX_PRIMES + 50];
static int matrixAV[MAX_PRIMES];
static int matrixV[MAX_PRIMES];
static int matrixV1[MAX_PRIMES];
static int matrixV2[MAX_PRIMES];
static int matrixXmY[MAX_PRIMES];
static int newColumns[MAX_PRIMES];
// Matrix that holds temporary data
static int matrixCalc3[MAX_PRIMES];
static int matrixTemp2[MAX_PRIMES];
static int nbrPrimes2;
static BigInteger factorSiqs;
static unsigned char onlyFactoring;
PrimeSieveData *firstPrimeSieveData;
static unsigned char InsertNewRelation(
  int *rowMatrixB,
  int *biT, int *biU, int *biR,
  int NumberLength);
static void BlockLanczos(void);
void ShowSIQSStatus(int matrixBLength, long startTime);

static void ChSignBigNbr(int *nbr, int length)
{
  int carry = 0;
  int ctr;
  for (ctr = 0; ctr < length; ctr++)
  {
    carry -= *nbr;
    *nbr &= -LIMB_RANGE;
    carry >>= BITS_PER_GROUP;
  }
}

static void PerformSiqsSieveStage(PrimeSieveData primeSieveData[],
  short SieveArray[],
  int nbrPrimes, int nbrPrimes2,
  int firstLimit, int secondLimit, int thirdLimit,
  int smallPrimeUpperLimit, unsigned char threshold,
  int multiplier, int amodq[],
  int PolynomialIndex,
  int nbrFactorsA, limb *biLinearCoeff,
  int NumberLength)
{
  short logPrimeEvenPoly, logPrimeOddPoly;
  int currentPrime, F1, F2, F3, F4, X1, X2;
  int index, index2, indexFactorA;
  int mask;
  unsigned char polyadd;
  int S1, G0, G1, G2, G3;
  int H0, H1, H2, H3, I0, I1, I2, I3;
  PrimeSieveData *rowPrimeSieveData;

  F1 = PolynomialIndex;
  indexFactorA = 0;
  while ((F1 & 1) == 0)
  {
    F1 >>= 1;
    indexFactorA++;
  }
  if (polyadd = ((F1 & 2) != 0))   // Adjust value of B as appropriate
  {                                // according to the Gray code.
    AddBigNbr(biLinearCoeff, biLinearDelta[indexFactorA], biLinearCoeff,
      NumberLength);
    AddBigNbr(biLinearCoeff, biLinearDelta[indexFactorA], biLinearCoeff,
      NumberLength);
  }
  else
  {
    SubtractBigNbr(biLinearCoeff, biLinearDelta[indexFactorA],
      biLinearCoeff, NumberLength);
    SubtractBigNbr(biLinearCoeff, biLinearDelta[indexFactorA],
      biLinearCoeff, NumberLength);
  }
  indexFactorA--;
  X1 = SieveLimit << 1;
  rowPrimeSieveData = &primeSieveData[1];
  F1 = polyadd ? -rowPrimeSieveData -> Bainv2[indexFactorA] :
    rowPrimeSieveData->Bainv2[indexFactorA];
  if (((rowPrimeSieveData->soln1 += F1) & 1) == 0)
  {
    SieveArray[0] = (short)(logar2 - threshold);
    SieveArray[1] = (short)(-threshold);
  }
  else
  {
    SieveArray[0] = (short)(-threshold);
    SieveArray[1] = (short)(logar2 - threshold);
  }
  if (((rowPrimeSieveData->soln1 + rowPrimeSieveData->Bainv2_0) & 1) == 0)
  {
    SieveArray[0] += (short)((logar2 - threshold) << 8);
    SieveArray[1] += (short)((-threshold) << 8);
  }
  else
  {
    SieveArray[0] += (short)((-threshold) << 8);
    SieveArray[1] += (short)((logar2 - threshold) << 8);
  }
  F2 = 2;
  index = 2;
  for (;;)
  {
    rowPrimeSieveData = &primeSieveData[index];
    currentPrime = rowPrimeSieveData->value;
    F3 = F2 * currentPrime;
    if (X1 + 1 < F3)
    {
      F3 = X1 + 1;
    }
    F4 = F2;
    while (F4 * 2 <= F3)
    {
      memcpy(&SieveArray[F4], &SieveArray[0], F4*sizeof(SieveArray[0]));
      F4 *= 2;
    }
    memcpy(&SieveArray[F4], &SieveArray[0], (F3-F4) * sizeof(SieveArray[0]));
    if (F3 == X1 + 1)
    {
      break;
    }
    F1 = currentPrime;
    logPrimeEvenPoly = 1;
    while (F1 >= 5)
    {
      F1 /= 3;
      logPrimeEvenPoly++;
    }
    logPrimeOddPoly = (short)(logPrimeEvenPoly << 8);
    F1 = polyadd ? -rowPrimeSieveData->Bainv2[indexFactorA] :
      rowPrimeSieveData->Bainv2[indexFactorA];
    index2 = (rowPrimeSieveData->soln1 + F1) % currentPrime;
    rowPrimeSieveData->soln1 = index2 += currentPrime & (index2 >> 31);
    for (; index2 < F3; index2 += currentPrime)
    {
      SieveArray[index2] += logPrimeEvenPoly;
    }
    for (index2 = (rowPrimeSieveData->soln1 + currentPrime -
      rowPrimeSieveData->Bainv2_0) % currentPrime;
      index2 < F3;
      index2 += currentPrime)
    {
      SieveArray[index2] += logPrimeOddPoly;
    }
    if (currentPrime != multiplier)
    {
      for (F1 = index2 = (rowPrimeSieveData->soln1 + currentPrime -
        rowPrimeSieveData->difsoln) % currentPrime;
        index2 < F3;
        index2 += currentPrime)
      {
        SieveArray[index2] += logPrimeEvenPoly;
      }
      for (index2 = (F1 + currentPrime -
        rowPrimeSieveData->Bainv2_0) % currentPrime;
        index2 < F3;
        index2 += currentPrime)
      {
        SieveArray[index2] += logPrimeOddPoly;
      }
    }
    index++;
    F2 *= currentPrime;
  }

  F1 = primeSieveData[smallPrimeUpperLimit].value;
  logPrimeEvenPoly = 1;
  logPrimeOddPoly = 0x100;
  mask = 5;
  while (F1 >= 5)
  {
    F1 /= 3;
    logPrimeEvenPoly++;
    logPrimeOddPoly += 0x100;
    mask *= 3;
  }
  if (polyadd)
  {
    for (; index < smallPrimeUpperLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if ((S1 = rowPrimeSieveData->soln1 -
        rowPrimeSieveData->Bainv2[indexFactorA]) < 0)
      {
        S1 += currentPrime;
      }
      rowPrimeSieveData->soln1 = S1;
    }
    for (index = smallPrimeUpperLimit; index < firstLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = currentPrime + currentPrime;
      F3 = F2 + currentPrime;
      F4 = F3 + currentPrime;
      S1 = rowPrimeSieveData->soln1 -
        rowPrimeSieveData->Bainv2[indexFactorA];
      rowPrimeSieveData->soln1 = S1 += (S1 >> 31) & currentPrime;
      index2 = X1 / F4 * F4 + S1;
      G0 = -rowPrimeSieveData->difsoln;
      if (S1 + G0 < 0)
      {
        G0 += currentPrime;
      }
      G1 = G0 + currentPrime;
      G2 = G1 + currentPrime;
      G3 = G2 + currentPrime;
      H0 = -rowPrimeSieveData->Bainv2_0;
      if (S1 + H0 < 0)
      {
        H0 += currentPrime;
      }
      H1 = H0 + currentPrime;
      H2 = H1 + currentPrime;
      H3 = H2 + currentPrime;
      I0 = H0 - rowPrimeSieveData->difsoln;
      if (S1 + I0 < 0)
      {
        I0 += currentPrime;
      }
      I1 = I0 + currentPrime;
      I2 = I1 + currentPrime;
      I3 = I2 + currentPrime;
      do
      {
        SieveArray[index2] += logPrimeEvenPoly;
        SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
        SieveArray[index2 + F2] += logPrimeEvenPoly;
        SieveArray[index2 + F3] += logPrimeEvenPoly;
        SieveArray[index2 + G0] += logPrimeEvenPoly;
        SieveArray[index2 + G1] += logPrimeEvenPoly;
        SieveArray[index2 + G2] += logPrimeEvenPoly;
        SieveArray[index2 + G3] += logPrimeEvenPoly;
        SieveArray[index2 + H0] += logPrimeOddPoly;
        SieveArray[index2 + H1] += logPrimeOddPoly;
        SieveArray[index2 + H2] += logPrimeOddPoly;
        SieveArray[index2 + H3] += logPrimeOddPoly;
        SieveArray[index2 + I0] += logPrimeOddPoly;
        SieveArray[index2 + I1] += logPrimeOddPoly;
        SieveArray[index2 + I2] += logPrimeOddPoly;
        SieveArray[index2 + I3] += logPrimeOddPoly;
      } while ((index2 -= F4) >= 0);
    }
    for (; index < secondLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      F2 = currentPrime + currentPrime;
      F3 = F2 + currentPrime;
      F4 = F2 + F2;
      X2 = X1 - F4;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      if (rowPrimeSieveData->difsoln >= 0)
      {
        F1 = rowPrimeSieveData->soln1 -
          rowPrimeSieveData->Bainv2[indexFactorA];
        F1 += (F1 >> 31) & currentPrime;
        index2 = (rowPrimeSieveData->soln1 = F1);
        do
        {
          SieveArray[index2] += logPrimeEvenPoly;
          SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
          SieveArray[index2 + F2] += logPrimeEvenPoly;
          SieveArray[index2 + F3] += logPrimeEvenPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeEvenPoly;
        }
        index2 = F1 - rowPrimeSieveData->Bainv2_0;
        index2 += (index2 >> 31) & currentPrime;
        do
        {
          SieveArray[index2] += logPrimeOddPoly;
          SieveArray[index2 + currentPrime] += logPrimeOddPoly;
          SieveArray[index2 + F2] += logPrimeOddPoly;
          SieveArray[index2 + F3] += logPrimeOddPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeOddPoly;
        }
        F1 -= rowPrimeSieveData->difsoln;
        F1 += (F1 >> 31) & currentPrime;
        index2 = F1;
        do
        {
          SieveArray[index2] += logPrimeEvenPoly;
          SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
          SieveArray[index2 + F2] += logPrimeEvenPoly;
          SieveArray[index2 + F3] += logPrimeEvenPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeEvenPoly;
        }
        index2 = F1 - rowPrimeSieveData->Bainv2_0;
        index2 += (index2 >> 31) & currentPrime;
        do
        {
          SieveArray[index2] += logPrimeOddPoly;
          SieveArray[index2 + currentPrime] += logPrimeOddPoly;
          SieveArray[index2 + F2] += logPrimeOddPoly;
          SieveArray[index2 + F3] += logPrimeOddPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeOddPoly;
        }
      }
    }
    for (; index < thirdLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      F2 += currentPrime & (F2 >> 31);
      index2 = (rowPrimeSieveData->soln1 = F2);
      do
      {
        SieveArray[index2] += logPrimeEvenPoly;
      } while ((index2 += currentPrime) <= X1);
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      F1 += currentPrime & (F1 >> 31);
      do
      {
        SieveArray[F1] += logPrimeOddPoly;
      } while ((F1 += currentPrime) <= X1);
      F2 -= rowPrimeSieveData->difsoln;
      index2 = F2 += currentPrime & (F2 >> 31);
      do
      {
        SieveArray[index2] += logPrimeEvenPoly;
      } while ((index2 += currentPrime) <= X1);
      F2 += (currentPrime & ((F2 - F3) >> 31)) - F3;
      do
      {
        SieveArray[F2] += logPrimeOddPoly;
      } while ((F2 += currentPrime) <= X1);
    }
    for (; index < nbrPrimes2; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      if ((F2 += (currentPrime & ((F2 - F3) >> 31)) - F3) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      if ((F2 += (currentPrime & ((F2 - F3) >> 31)) - F3) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
    }
    for (; index < nbrPrimes; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 - rowPrimeSieveData->Bainv2[indexFactorA];
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
    }
  }
  else
  {
    for (; index < smallPrimeUpperLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      S1 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      S1 += currentPrime & (S1 >> 31);
      rowPrimeSieveData->soln1 = S1;
    }
    for (index = smallPrimeUpperLimit; index < firstLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = currentPrime + currentPrime;
      F3 = F2 + currentPrime;
      F4 = F3 + currentPrime;
      S1 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      rowPrimeSieveData->soln1 = S1 += (S1 >> 31) & currentPrime;
      index2 = X1 / F4 * F4 + S1;
      G0 = -rowPrimeSieveData->difsoln;
      if (S1 + G0 < 0)
      {
        G0 += currentPrime;
      }
      G1 = G0 + currentPrime;
      G2 = G1 + currentPrime;
      G3 = G2 + currentPrime;
      H0 = -rowPrimeSieveData->Bainv2_0;
      if (S1 + H0 < 0)
      {
        H0 += currentPrime;
      }
      H1 = H0 + currentPrime;
      H2 = H1 + currentPrime;
      H3 = H2 + currentPrime;
      I0 = H0 - rowPrimeSieveData->difsoln;
      if (S1 + I0 < 0)
      {
        I0 += currentPrime;
      }
      I1 = I0 + currentPrime;
      I2 = I1 + currentPrime;
      I3 = I2 + currentPrime;
      do
      {
        SieveArray[index2] += logPrimeEvenPoly;
        SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
        SieveArray[index2 + F2] += logPrimeEvenPoly;
        SieveArray[index2 + F3] += logPrimeEvenPoly;
        SieveArray[index2 + G0] += logPrimeEvenPoly;
        SieveArray[index2 + G1] += logPrimeEvenPoly;
        SieveArray[index2 + G2] += logPrimeEvenPoly;
        SieveArray[index2 + G3] += logPrimeEvenPoly;
        SieveArray[index2 + H0] += logPrimeOddPoly;
        SieveArray[index2 + H1] += logPrimeOddPoly;
        SieveArray[index2 + H2] += logPrimeOddPoly;
        SieveArray[index2 + H3] += logPrimeOddPoly;
        SieveArray[index2 + I0] += logPrimeOddPoly;
        SieveArray[index2 + I1] += logPrimeOddPoly;
        SieveArray[index2 + I2] += logPrimeOddPoly;
        SieveArray[index2 + I3] += logPrimeOddPoly;
      } while ((index2 -= F4) >= 0);
    }
    for (; index < secondLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      F2 = currentPrime + currentPrime;
      F3 = F2 + currentPrime;
      F4 = F2 + F2;
      X2 = X1 - F4;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      if (rowPrimeSieveData->difsoln >= 0)
      {
        F1 = rowPrimeSieveData->soln1 +
          rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
        F1 += currentPrime & (F1 >> 31);
        index2 = (rowPrimeSieveData->soln1 = F1);
        do
        {
          SieveArray[index2] += logPrimeEvenPoly;
          SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
          SieveArray[index2 + F2] += logPrimeEvenPoly;
          SieveArray[index2 + F3] += logPrimeEvenPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeEvenPoly;
        }
        index2 = F1 - rowPrimeSieveData->Bainv2_0;
        index2 += (index2 >> 31) & currentPrime;
        do
        {
          SieveArray[index2] += logPrimeOddPoly;
          SieveArray[index2 + currentPrime] += logPrimeOddPoly;
          SieveArray[index2 + F2] += logPrimeOddPoly;
          SieveArray[index2 + F3] += logPrimeOddPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeOddPoly;
        }
        F1 -= rowPrimeSieveData->difsoln;
        F1 += (F1 >> 31) & currentPrime;
        index2 = F1;
        do
        {
          SieveArray[index2] += logPrimeEvenPoly;
          SieveArray[index2 + currentPrime] += logPrimeEvenPoly;
          SieveArray[index2 + F2] += logPrimeEvenPoly;
          SieveArray[index2 + F3] += logPrimeEvenPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeEvenPoly;
        }
        index2 = F1 - rowPrimeSieveData->Bainv2_0;
        index2 += (index2 >> 31) & currentPrime;
        do
        {
          SieveArray[index2] += logPrimeOddPoly;
          SieveArray[index2 + currentPrime] += logPrimeOddPoly;
          SieveArray[index2 + F2] += logPrimeOddPoly;
          SieveArray[index2 + F3] += logPrimeOddPoly;
        } while ((index2 += F4) <= X2);
        for (; index2 <= X1; index2 += currentPrime)
        {
          SieveArray[index2] += logPrimeOddPoly;
        }
      }
    }
    for (; index < thirdLimit; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      index2 = (rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31)));
      do
      {
        SieveArray[index2] += logPrimeEvenPoly;
      } while ((index2 += currentPrime) <= X1);
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      F1 += currentPrime & (F1 >> 31);
      do
      {
        SieveArray[F1] += logPrimeOddPoly;
      } while ((F1 += currentPrime) <= X1);
      F2 -= rowPrimeSieveData->difsoln;
      F1 = F2 += currentPrime & (F2 >> 31);
      do
      {
        SieveArray[F2] += logPrimeEvenPoly;
      } while ((F2 += currentPrime) <= X1);
      F1 -= F3;
      F1 += currentPrime & (F1 >> 31);
      do
      {
        SieveArray[F1] += logPrimeOddPoly;
      } while ((F1 += currentPrime) <= X1);
    }
    for (; index < nbrPrimes2; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
      rowPrimeSieveData = &primeSieveData[++index];
      currentPrime = rowPrimeSieveData->value;
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
    }
    for (; index < nbrPrimes; index++)
    {
      rowPrimeSieveData = &primeSieveData[index];
      currentPrime = rowPrimeSieveData->value;
      if (currentPrime >= mask)
      {
        mask *= 3;
        logPrimeEvenPoly++;
        logPrimeOddPoly += 0x100;
      }
      F2 = rowPrimeSieveData->soln1 +
        rowPrimeSieveData->Bainv2[indexFactorA] - currentPrime;
      if ((rowPrimeSieveData->soln1 = (F2 += currentPrime & (F2 >> 31))) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F1 = F2 - (F3 = rowPrimeSieveData->Bainv2_0);
      if ((F1 += currentPrime & (F1 >> 31)) < X1)
      {
        SieveArray[F1] += logPrimeOddPoly;
      }
      F2 -= rowPrimeSieveData->difsoln;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeEvenPoly;
      }
      F2 -= F3;
      if ((F2 += currentPrime & (F2 >> 31)) < X1)
      {
        SieveArray[F2] += logPrimeOddPoly;
      }
    }
  }
}

static long PerformTrialDivision(PrimeSieveData primeSieveData[],
  PrimeTrialDivisionData primeTrialDivisionData[],
  int nbrPrimes, int rowMatrixBbeforeMerge[],
  int nbrFactorsA, int index2,
  int biDividend[], int rowSquares[],
  int NumberLength, int biT[],
  unsigned char oddPolynomial)
{
  long biR0 = 0, biR1 = 0, biR2 = 0, biR3 = 0, biR4 = 0, biR5 = 0;
  long biR6 = 0;
  unsigned char cond = FALSE;
  unsigned char testFactorA;
  long Divid, Divisor;
  int nbrSquares = rowSquares[0];
  int divis, iRem;
  int index;
  int newFactorAIndex;
  long Rem;
  int expParity;
  int left, right, median, nbr;
  int indexFactorA = 0;
  int nbrColumns = rowMatrixBbeforeMerge[0];
  unsigned char fullRemainder;
  PrimeSieveData *rowPrimeSieveData;
  PrimeTrialDivisionData *rowPrimeTrialDivisionData;
  switch (NumberLength)
  {
  case 7:
    biR6 = biDividend[6];
  case 6:
    biR5 = biDividend[5];
  case 5:
    biR4 = biDividend[4];
  case 4:
    biR3 = biDividend[3];
  case 3:
    biR2 = biDividend[2];
  case 1:
  case 2:
    biR1 = biDividend[1];
    biR0 = biDividend[0];
  }
  expParity = 0;
  Divid = (biR1 << 31) + biR0;
  if (NumberLength <= 2)
  {
    for (index = 1; index < nbrPrimes; index++)
    {
      Divisor = primeTrialDivisionData[index].value;
      while (Divid % Divisor == 0)
      {
        Divid /= Divisor;
        expParity = 1 - expParity;
        if (expParity == 0)
        {
          rowSquares[nbrSquares++] = (int)Divisor;
        }
      }
      if (expParity != 0)
      {
        rowMatrixBbeforeMerge[nbrColumns++] = index;
        expParity = 0;
      }
    }
  }
  else
  {
    testFactorA = TRUE;
    newFactorAIndex = aindex[0];
    for (index = 1; testFactorA; index++)
    {
      fullRemainder = FALSE;
      if (index < 3)
      {
        fullRemainder = TRUE;
      }
      else if (index == newFactorAIndex)
      {
        fullRemainder = TRUE;
        if (++indexFactorA == nbrFactorsA)
        {
          testFactorA = FALSE;   // All factors of A were tested.
        }
        else
        {
          newFactorAIndex = aindex[indexFactorA];
        }
      }
      for (;;)
      {
        if (fullRemainder == FALSE)
        {
          rowPrimeSieveData = &primeSieveData[index];
          Divisor = rowPrimeSieveData->value;
          divis = (int)Divisor;
          if (oddPolynomial)
          {
            iRem = index2 - rowPrimeSieveData->soln1 +
              rowPrimeSieveData->Bainv2_0;
          }
          else
          {
            iRem = index2 - rowPrimeSieveData->soln1;
          }
          if (iRem >= divis)
          {
            if ((iRem -= divis) >= divis)
            {
              if ((iRem -= divis) >= divis)
              {
                iRem %= divis;
              }
            }
          }
          else
          {
            iRem += (iRem >> 31) & divis;
          }
          if (iRem != 0 && iRem != divis - rowPrimeSieveData->difsoln)
          {
            if (expParity != 0)
            {
              rowMatrixBbeforeMerge[nbrColumns++] = index;
              expParity = 0;
            }
            break;              // Process next prime.
          }
          fullRemainder = TRUE;
        }
        else
        {
          rowPrimeTrialDivisionData = &primeTrialDivisionData[index];
          Divisor = rowPrimeTrialDivisionData->value;
          divis = (int)Divisor;
          switch (NumberLength)
          {
          case 7:
            Rem = biR6*rowPrimeTrialDivisionData->exp6 +
              biR5*rowPrimeTrialDivisionData->exp5 +
              biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 6:
            Rem = biR5*rowPrimeTrialDivisionData->exp5 +
              biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 5:
            Rem = biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 4:
            Rem = biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          default:
            Rem = biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          }
          if (Rem%divis != 0)
          {                     // Number is not a multiple of prime.
            if (expParity != 0)
            {
              rowMatrixBbeforeMerge[nbrColumns++] = index;
              expParity = 0;
            }
            break;              // Process next prime.
          }
        }
        expParity = 1 - expParity;
        if (expParity == 0)
        {
          rowSquares[nbrSquares++] = (int)Divisor;
        }
        Rem = 0;
        switch (NumberLength)
        {
        case 7:
          Divid = biR6 + (Rem << 31);
          Rem = Divid - (biR6 = Divid / Divisor) * Divisor;
        case 6:
          Divid = biR5 + (Rem << 31);
          Rem = Divid - (biR5 = Divid / Divisor) * Divisor;
        case 5:
          Divid = biR4 + (Rem << 31);
          Rem = Divid - (biR4 = Divid / Divisor) * Divisor;
        case 4:
          Divid = biR3 + (Rem << 31);
          Rem = Divid - (biR3 = Divid / Divisor) * Divisor;
        case 3:
          Divid = biR2 + (Rem << 31);
          Rem = Divid - (biR2 = Divid / Divisor) * Divisor;
          Divid = biR1 + (Rem << 31);
          biR1 = Divid / Divisor;
          biR0 = (biR0 + ((Divid - Divisor*biR1) << 31)) / Divisor;
        }
        switch (NumberLength)
        {
        case 7:
          cond = (biR6 == 0 && biR5 < 0x40000000);
          break;
        case 6:
          cond = (biR5 == 0 && biR4 < 0x40000000);
          break;
        case 5:
          cond = (biR4 == 0 && biR3 < 0x40000000);
          break;
        case 4:
          cond = (biR3 == 0 && biR2 < 0x40000000);
          break;
        case 3:
          cond = (biR2 == 0 && biR1 < 0x40000000);
          break;
        }
        Divid = (biR1 << 31) + biR0;
        if (cond)
        {
          NumberLength--;
          if (NumberLength == 2)
          {
            int sqrtDivid = (int)sqrt(Divid) + 1;
            fullRemainder = TRUE;
            for (; index < nbrPrimes; index++)
            {
              rowPrimeSieveData = &primeSieveData[index];
              Divisor = rowPrimeSieveData->value;
              if (testFactorA && index == newFactorAIndex)
              {
                fullRemainder = TRUE;
                if (++indexFactorA == nbrFactorsA)
                {
                  testFactorA = FALSE;   // All factors of A were tested.
                }
                else
                {
                  newFactorAIndex = aindex[indexFactorA];
                }
              }
              for (;;)
              {
                if (fullRemainder == FALSE)
                {
                  divis = (int)Divisor;
                  if (oddPolynomial)
                  {
                    iRem = index2 - rowPrimeSieveData->soln1 +
                      rowPrimeSieveData->Bainv2_0;
                  }
                  else
                  {
                    iRem = index2 - rowPrimeSieveData->soln1;
                  }
                  if (iRem >= divis)
                  {
                    if ((iRem -= divis) >= divis)
                    {
                      if ((iRem -= divis) >= divis)
                      {
                        iRem %= divis;
                      }
                    }
                  }
                  else
                  {
                    iRem += (iRem >> 31) & divis;
                  }
                  if (iRem != 0 && iRem != divis - rowPrimeSieveData->difsoln)
                  {
                    break;
                  }
                  fullRemainder = TRUE;
                }
                else if (Divid % Divisor != 0)
                {
                  break;
                }
                Divid /= Divisor;
                sqrtDivid = (int)sqrt(Divid) + 1;
                expParity = 1 - expParity;
                if (expParity == 0)
                {
                  rowSquares[nbrSquares++] = (int)Divisor;
                }
              }
              if (expParity != 0)
              {
                rowMatrixBbeforeMerge[nbrColumns++] = index;
                expParity = 0;
              }
              if (Divisor > sqrtDivid)
              {                     // End of trial division.
                rowSquares[0] = nbrSquares;
                index = nbrPrimes - 1;
                if (Divid <= primeTrialDivisionData[index].value &&
                  Divid > 1)
                {          // Perform binary search to find the index.
                  left = -1;
                  median = right = nbrPrimes;
                  while (left != right)
                  {
                    median = ((right - left) >> 1) + left;
                    nbr = primeTrialDivisionData[median].value;
                    if (nbr < Divid)
                    {
                      if (median == left &&
                        congruencesFound >= matrixBLength)
                      {
                        return 0;
                      }
                      left = median;
                    }
                    else if (nbr > Divid)
                    {
                      right = median;
                    }
                    else
                    {
                      break;
                    }
                  }
                  rowMatrixBbeforeMerge[nbrColumns++] = median;
                  rowMatrixBbeforeMerge[0] = nbrColumns;
                  return 1;
                }
                rowMatrixBbeforeMerge[0] = nbrColumns;
                return Divid;
              }
              fullRemainder = FALSE;
            }
            break;
          }
        }
      }             /* end while */
    }               /* end for */
    for (; index < nbrPrimes; index++)
    {
      fullRemainder = FALSE;
      for (;;)
      {
        if (fullRemainder == FALSE)
        {
          rowPrimeSieveData = &primeSieveData[index];
          Divisor = rowPrimeSieveData->value;
          divis = (int)Divisor;
          if (oddPolynomial)
          {
            iRem = index2 - rowPrimeSieveData->soln1 +
              rowPrimeSieveData->Bainv2_0;
          }
          else
          {
            iRem = index2 - rowPrimeSieveData->soln1;
          }
          if (iRem >= divis)
          {
            if ((iRem -= divis) >= divis)
            {
              if ((iRem -= divis) >= divis)
              {
                iRem %= divis;
              }
            }
          }
          else
          {
            iRem += (iRem >> 31) & divis;
          }
          if (iRem != 0 && iRem != divis - rowPrimeSieveData->difsoln)
          {
            if (expParity != 0)
            {
              rowMatrixBbeforeMerge[nbrColumns++] = index;
              expParity = 0;
            }
            break;              // Process next prime.
          }
          fullRemainder = TRUE;
        }
        else
        {
          rowPrimeTrialDivisionData = &primeTrialDivisionData[index];
          Divisor = rowPrimeTrialDivisionData->value;
          divis = (int)Divisor;
          switch (NumberLength)
          {
          case 7:
            Rem = biR6*rowPrimeTrialDivisionData->exp6 +
              biR5*rowPrimeTrialDivisionData->exp5 +
              biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 6:
            Rem = biR5*rowPrimeTrialDivisionData->exp5 +
              biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 5:
            Rem = biR4*rowPrimeTrialDivisionData->exp4 +
              biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          case 4:
            Rem = biR3*rowPrimeTrialDivisionData->exp3 +
              biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          default:
            Rem = biR2*rowPrimeTrialDivisionData->exp2 + Divid;
            break;
          }
          if (Rem%divis != 0)
          {                     // Number is not a multiple of prime.
            if (expParity != 0)
            {
              rowMatrixBbeforeMerge[nbrColumns++] = index;
              expParity = 0;
            }
            break;              // Process next prime.
          }
        }
        expParity = 1 - expParity;
        if (expParity == 0)
        {
          rowSquares[nbrSquares++] = (int)Divisor;
        }
        Rem = 0;
        switch (NumberLength)
        {
        case 7:
          Divid = biR6 + (Rem << 31);
          Rem = Divid - (biR6 = Divid / Divisor) * Divisor;
        case 6:
          Divid = biR5 + (Rem << 31);
          Rem = Divid - (biR5 = Divid / Divisor) * Divisor;
        case 5:
          Divid = biR4 + (Rem << 31);
          Rem = Divid - (biR4 = Divid / Divisor) * Divisor;
        case 4:
          Divid = biR3 + (Rem << 31);
          Rem = Divid - (biR3 = Divid / Divisor) * Divisor;
        case 3:
          Divid = biR2 + (Rem << 31);
          Rem = Divid - (biR2 = Divid / Divisor) * Divisor;
          Divid = biR1 + (Rem << 31);
          biR1 = Divid / Divisor;
          biR0 = (biR0 + ((Divid - Divisor*biR1) << 31)) / Divisor;
        }
        switch (NumberLength)
        {
        case 7:
          cond = (biR6 == 0 && biR5 < 0x40000000);
          break;
        case 6:
          cond = (biR5 == 0 && biR4 < 0x40000000);
          break;
        case 5:
          cond = (biR4 == 0 && biR3 < 0x40000000);
          break;
        case 4:
          cond = (biR3 == 0 && biR2 < 0x40000000);
          break;
        case 3:
          cond = (biR2 == 0 && biR1 < 0x40000000);
          break;
        }
        Divid = (biR1 << 31) + biR0;
        if (cond)
        {
          NumberLength--;
          if (NumberLength == 2)
          {
            int sqrtDivid = (int)sqrt(Divid) + 1;
            fullRemainder = TRUE;
            for (; index < nbrPrimes; index++)
            {
              rowPrimeSieveData = &primeSieveData[index];
              Divisor = rowPrimeSieveData->value;
              if (testFactorA && index == newFactorAIndex)
              {
                fullRemainder = TRUE;
                if (++indexFactorA == nbrFactorsA)
                {
                  testFactorA = FALSE;   // All factors of A were tested.
                }
                else
                {
                  newFactorAIndex = aindex[indexFactorA];
                }
              }
              for (;;)
              {
                if (fullRemainder == FALSE)
                {
                  divis = (int)Divisor;
                  if (oddPolynomial)
                  {
                    iRem = index2 - rowPrimeSieveData->soln1 +
                      rowPrimeSieveData->Bainv2_0;
                  }
                  else
                  {
                    iRem = index2 - rowPrimeSieveData->soln1;
                  }
                  if (iRem >= divis)
                  {
                    if ((iRem -= divis) >= divis)
                    {
                      if ((iRem -= divis) >= divis)
                      {
                        iRem %= divis;
                      }
                    }
                  }
                  else
                  {
                    iRem += (iRem >> 31) & divis;
                  }
                  if (iRem != 0 && iRem != divis - rowPrimeSieveData->difsoln)
                  {
                    break;
                  }
                  fullRemainder = TRUE;
                }
                else if (Divid % Divisor != 0)
                {
                  break;
                }
                Divid /= Divisor;
                sqrtDivid = (int)sqrt(Divid) + 1;
                expParity = 1 - expParity;
                if (expParity == 0)
                {
                  rowSquares[nbrSquares++] = (int)Divisor;
                }
              }
              if (expParity != 0)
              {
                rowMatrixBbeforeMerge[nbrColumns++] = index;
                expParity = 0;
              }
              if (Divisor > sqrtDivid)
              {                     // End of trial division.
                rowSquares[0] = nbrSquares;
                index = nbrPrimes - 1;
                if (Divid <= primeTrialDivisionData[index].value &&
                  Divid > 1)
                {          // Perform binary search to find the index.
                  left = -1;
                  median = right = nbrPrimes;
                  while (left != right)
                  {
                    median = ((right - left) >> 1) + left;
                    nbr = primeTrialDivisionData[median].value;
                    if (nbr < Divid)
                    {
                      if (median == left &&
                        congruencesFound >= matrixBLength)
                      {
                        return 0;
                      }
                      left = median;
                    }
                    else if (nbr > Divid)
                    {
                      right = median;
                    }
                    else
                    {
                      break;
                    }
                  }
                  rowMatrixBbeforeMerge[nbrColumns++] = median;
                  rowMatrixBbeforeMerge[0] = nbrColumns;
                  return 1;
                }
                rowMatrixBbeforeMerge[0] = nbrColumns;
                return Divid;
              }
              fullRemainder = FALSE;
            }
            break;
          }
        }
      }             /* end while */
    }               /* end for */
  }
  rowSquares[0] = nbrSquares;
  rowMatrixBbeforeMerge[0] = nbrColumns;
  if (NumberLength > 2)
  {
    return 0;           // Very large quotient.
  }
  return Divid;
}

static void mergeArrays(int nbrFactorsA,
  int rowMatrixB[], int rowMatrixBeforeMerge[],
  PrimeTrialDivisionData primeTrialDivisionData[],
  int rowSquares[])
{
  int indexAindex = 0;
  int indexRMBBM = 1;
  int indexRMB = 1;
  int nbrColumns = rowMatrixBeforeMerge[0];

  while (indexAindex < nbrFactorsA && indexRMBBM < nbrColumns)
  {
    if (aindex[indexAindex] < rowMatrixBeforeMerge[indexRMBBM])
    {
      rowMatrixB[indexRMB++] = aindex[indexAindex++];
    }
    else if (aindex[indexAindex] > rowMatrixBeforeMerge[indexRMBBM])
    {
      rowMatrixB[indexRMB++] = rowMatrixBeforeMerge[indexRMBBM++];
    }
    else
    {
      rowSquares[rowSquares[0]++] =
        primeTrialDivisionData[aindex[indexAindex++]].value;
      indexRMBBM++;
    }
  }
  while (indexAindex < nbrFactorsA)
  {
    rowMatrixB[indexRMB++] = aindex[indexAindex++];
  }
  while (indexRMBBM < nbrColumns)
  {
    rowMatrixB[indexRMB++] = rowMatrixBeforeMerge[indexRMBBM++];
  }
  rowMatrixB[0] = indexRMB;
}

static void SmoothRelationFound(
  unsigned char positive,
  int *rowMatrixB, int *rowMatrixBbeforeMerge,
  int *vectExpParity,
  int index2,
  PrimeTrialDivisionData *primeTrialDivisionData,
  long startTime, int nbrFactorsA,
  int *rowSquares, limb *biLinearCoeff,
  int NumberLength, int *biT, int *biU,
  int *biR, unsigned char oddPolynomial)
{
  int index;
  long D;
  int nbrSquares;
  if (congruencesFound == matrixBLength)
  {
    return;            // All congruences already found.
  }
  // Add all elements of aindex array to the rowMatrixB array discarding
  // duplicates.
  mergeArrays(nbrFactorsA, rowMatrixB, rowMatrixBbeforeMerge,
    primeTrialDivisionData, rowSquares);
  nbrSquares = rowSquares[0];
  LongToBigNbr(1, biR, NumberLength);
  LongToBigNbr(positive ? 1 : -1, biT, NumberLength);
  MultBigNbrByLong(biQuadrCoeff, index2 - SieveLimit, biU,
    NumberLength);                     // Ax
  AddBigNbr(biU, biLinearCoeff, biU, NumberLength);         // Ax+B
  if (oddPolynomial)
  {
    SubtractBigNbr(biU, biLinearDelta[0], biU, NumberLength);// Ax+B (odd)
    SubtractBigNbr(biU, biLinearDelta[0], biU, NumberLength);// Ax+B (odd)
  }
  if ((biU[NumberLength - 1] & 0x40000000) != 0)
  {                                        // If number is negative
    ChSignBigNbr(biU, NumberLength);   // make it positive.
  }
  for (index = 1; index < nbrSquares; index++)
  {
    D = rowSquares[index];
    if (D == multiplier)
    {
      AddBigNbr(biU, TestNbr, biU, NumberLength);
      DivBigNbrByLong(biU, D, biU, NumberLength);
    }
    else
    {
      MultBigNbrByLong(biR, D, biR, NumberLength);
    }
  }
  if (InsertNewRelation(rowMatrixB, biT, biU, biR, NumberLength))
  {
    smoothsFound++;
    ShowSIQSStatus(matrixBLength, startTime);
  }
  return;
}

static void PartialRelationFound(
  unsigned char positive,
  int *rowMatrixB, int *rowMatrixBbeforeMerge,
  int *vectExpParity,
  int index2,
  PrimeTrialDivisionData *primeTrialDivisionData,
  long startTime, int nbrFactorsA,
  long Divid, int *rowPartials,
  int *rowSquares,
  limb *biLinearCoeff, int NumberLength, int *biT,
  int *biR, int *biU, int *biV,
  int *indexFactorsA, unsigned char oddPolynomial)
{
  int index;
  int expParity;
  long D, Rem, Divisor;
  int nbrFactorsPartial;
  int prev;
  long seed;
  int hashIndex;
  int *rowPartial;
  int newDivid = (int)Divid;    // This number is greater than zero.
  int indexFactorA = 0;
  int oldDivid;
  int nbrSquares;
  int NumberLengthDivid;
  long DividLSDW;
  long biT2, biT3, biT4, biT5, biT6;
  int squareRootSize = NumberLength / 2 + 1;
  int nbrColumns;
  PrimeTrialDivisionData *rowPrimeTrialDivisionData;

  if (congruencesFound == matrixBLength)
  {
    return;
  }
  // Partial relation found.
  totalPartials++;
  // Check if there is already another relation with the same
  // factor outside the prime base.
  // Calculate hash index
  hashIndex = matrixPartialHashIndex[(int)(Divid & 0xFFE) >> 1];
  prev = -1;
  while (hashIndex >= 0)
  {
    rowPartial = matrixPartial[hashIndex];
    oldDivid = rowPartial[0];
    if (newDivid == oldDivid || newDivid == -oldDivid)
    {   // Match of partials.
      for (index = 0; index < squareRootSize; index++)
      {
        biV[index] = rowPartial[index + 2];
      }                           // biV = Old positive square root (Ax+B).
      for (; index < NumberLength; index++)
      {
        biV[index] = 0;
      }
      seed = rowPartial[squareRootSize + 2];
      getFactorsOfA(seed, nbrFactorsA, indexFactorsA);
      LongToBigNbr(newDivid, biR, NumberLength);
      nbrFactorsPartial = 0;
      // biT = old (Ax+B)^2.
      MultBigNbr(biV, biV, biT, NumberLength);
      // biT = old (Ax+B)^2 - N.
      SubtractBigNbr(biT, TestNbr, biT, NumberLength);
      if (oldDivid < 0)
      {
        rowPartials[nbrFactorsPartial++] = 0; // Insert -1 as a factor.
      }
      if (biT[NumberLength - 1] >= 0x40000000)
      {
        ChSignBigNbr(biT, NumberLength);   // Make it positive.
      }
      NumberLengthDivid = NumberLength;
      // The number is multiple of the big prime, so divide by it.
      DivBigNbrByLong(biT, newDivid, biT, NumberLengthDivid);
      if (biT[NumberLengthDivid - 1] == 0 &&
        biT[NumberLengthDivid - 2] < 0x40000000)
      {
        NumberLengthDivid--;
      }
      for (index = 0; index < nbrFactorsA; index++)
      {
        DivBigNbrByLong(biT,
          primeTrialDivisionData[indexFactorsA[index]].value, biT,
          NumberLengthDivid);
        if (biT[NumberLengthDivid - 1] == 0 &&
          biT[NumberLengthDivid - 2] < 0x40000000)
        {
          NumberLengthDivid--;
        }
      }
      DividLSDW = ((long)biT[1] << 31) + biT[0];
      biT2 = biT[2];
      biT3 = biT[3];
      biT4 = biT[4];
      biT5 = biT[5];
      biT6 = biT[6];
      for (index = 1; index < nbrPrimes; index++)
      {
        expParity = 0;
        if (index >= indexMinFactorA && indexFactorA < nbrFactorsA)
        {
          if (index == indexFactorsA[indexFactorA])
          {
            expParity = 1;
            indexFactorA++;
          }
        }
        rowPrimeTrialDivisionData = &primeTrialDivisionData[index];
        Divisor = rowPrimeTrialDivisionData->value;
        for (;;)
        {
          switch (NumberLengthDivid)
          {
          case 7:
            Rem = biT6*rowPrimeTrialDivisionData->exp6 +
              biT5*rowPrimeTrialDivisionData->exp5 +
              biT4*rowPrimeTrialDivisionData->exp4 +
              biT3*rowPrimeTrialDivisionData->exp3 +
              biT2*rowPrimeTrialDivisionData->exp2 + DividLSDW;
            break;
          case 6:
            Rem = biT5*rowPrimeTrialDivisionData->exp5 +
              biT4*rowPrimeTrialDivisionData->exp4 +
              biT3*rowPrimeTrialDivisionData->exp3 +
              biT2*rowPrimeTrialDivisionData->exp2 + DividLSDW;
            break;
          case 5:
            Rem = biT4*rowPrimeTrialDivisionData->exp4 +
              biT3*rowPrimeTrialDivisionData->exp3 +
              biT2*rowPrimeTrialDivisionData->exp2 + DividLSDW;
            break;
          case 4:
            Rem = biT3*rowPrimeTrialDivisionData->exp3 +
              biT2*rowPrimeTrialDivisionData->exp2 + DividLSDW;
            break;
          case 3:
            Rem = biT2*rowPrimeTrialDivisionData->exp2 + DividLSDW;
            break;
          default:
            Rem = DividLSDW;
            break;
          }
          if (Rem%Divisor != 0)
          {
            break;
          }
          expParity = 1 - expParity;
          DivBigNbrByLong(biT, Divisor, biT, NumberLengthDivid);
          DividLSDW = ((long)biT[1] << 31) + biT[0];
          biT2 = biT[2];
          biT3 = biT[3];
          biT4 = biT[4];
          biT5 = biT[5];
          biT6 = biT[6];
          if (expParity == 0)
          {
            rowSquares[rowSquares[0]++] = (int)Divisor;
          }
          if (NumberLengthDivid <= 2)
          {
            if (DividLSDW == 1)
            {               // Division has ended.
              break;
            }
          }
          else if (biT[NumberLengthDivid - 1] == 0 &&
            biT[NumberLengthDivid - 2] < 0x40000000)
          {
            NumberLengthDivid--;
          }
        }
        if (expParity != 0)
        {
          rowPartials[nbrFactorsPartial++] = index;
        }
      }
      MultBigNbrByLong(biQuadrCoeff, index2 - SieveLimit, biT,
        NumberLength);
      AddBigNbr(biT, biLinearCoeff, biT, NumberLength); // biT = Ax+B
      if (oddPolynomial)
      {                                                     // Ax+B (odd)
        SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
        SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
      }
      if ((biT[NumberLength - 1] & 0x40000000) != 0)
      {                                        // If number is negative
        ChSignBigNbr(biT, NumberLength);   // make it positive.
      }
      // biU = Product of old Ax+B times new Ax+B
      MultBigNbrModN(biV, biT, biU, TestNbr, NumberLength);
      // Add all elements of aindex array to the rowMatrixB array discarding
      // duplicates.
      mergeArrays(nbrFactorsA, rowMatrixB, rowMatrixBbeforeMerge,
        primeTrialDivisionData, rowSquares);
      rowMatrixBbeforeMerge[0] = nbrColumns = rowMatrixB[0];
      memcpy(&rowMatrixBbeforeMerge[1], &rowMatrixB[1], nbrColumns*sizeof(int));
      mergeArrays(rowPartials, nbrFactorsPartial,
        rowMatrixB, rowMatrixBbeforeMerge, primeTrialDivisionData,
        rowSquares);
      nbrSquares = rowSquares[0];
      for (index = 1; index < nbrSquares; index++)
      {
        D = rowSquares[index];
        if (D != multiplier)
        {
          MultBigNbrByLong(biR, D, biR, NumberLength);
        }
        else
        {
          AddBigNbr(biU, TestNbr, biU, NumberLength);
          DivBigNbrByLong(biU, multiplier, biU, NumberLength);
        }
      }
      if (rowMatrixB[0] > 1 &&
        InsertNewRelation(rowMatrixB, biT, biU, biR, NumberLength))
      {
        partialsFound++;
        ShowSIQSStatus(matrixBLength, startTime);
      }
      return;
    }
    else
    {
      prev = hashIndex;
      hashIndex = rowPartial[1]; // Get next index for same hash.
    }
  } /* end while */
//  synchronized(firstPrimeSieveData)
  {
    if (hashIndex == -1 && nbrPartials < matrixPartialLength)
    { // No match and partials table is not full.
      // Add partial to table of partials.
      if (prev >= 0)
      {
        matrixPartial[prev][1] = nbrPartials;
      }
      else
      {
        matrixPartialHashIndex[(newDivid & 0xFFE) >> 1] = nbrPartials;
      }
      rowPartial = matrixPartial[nbrPartials];
      // Add all elements of aindex array to the rowMatrixB array discarding
      // duplicates.
      mergeArrays(nbrFactorsA, rowMatrixB, rowMatrixBbeforeMerge,
        primeTrialDivisionData, rowSquares);
      LongToBigNbr(Divid, biR, NumberLength);
      nbrSquares = rowSquares[0];
      for (index = 1; index < nbrSquares; index++)
      {
        D = rowSquares[index];
        MultBigNbrByLongModN(biR, D, biR, TestNbr, NumberLength);
        if (D == multiplier)
        {
          DivBigNbrByLong(biU, D, biU, NumberLength);
        }
      }
      rowPartial[0] = (positive ? newDivid : -newDivid);
      // Indicate last index with this hash.
      rowPartial[1] = -1;
      MultBigNbrByLong(biQuadrCoeff, index2 - SieveLimit, biT,
        NumberLength);
      AddBigNbr(biT, biLinearCoeff, biT, NumberLength); // biT = Ax+B
      if (oddPolynomial)
      {                                                     // Ax+B (odd)
        SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
        SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
      }
      if ((biT[NumberLength - 1] & 0x40000000) != 0)
      {                      // If square root is negative convert to positive.
        ChSignBigNbr(biT, NumberLength);
      }
      for (index = 0; index < squareRootSize; index++)
      {
        rowPartial[index + 2] = (int)biT[index];
      }
      rowPartial[squareRootSize + 2] = (int)oldSeed;
      nbrPartials++;
    }
  }               // End synchronized block.
  return;
}

static void SieveLocationHit(int rowMatrixB[], int rowMatrixBbeforeMerge[],
  int vectExpParity[],
  int index2,
  PrimeSieveData primeSieveData[],
  PrimeTrialDivisionData primeTrialDivisionData[],
  long startTime, int nbrFactorsA, int rowPartials[],
  int multiplier, int rowSquares[], int biDividend[],
  int NumberLength, int biT[], limb *biLinearCoeff,
  int biR[], int biU[], int biV[],
  int indexFactorsA[], unsigned char oddPolynomial)
{
  unsigned char positive;
  int NumberLengthDivid;
  int index;
  long Divid;
  int nbrColumns;

  trialDivisions++;
  MultBigNbrByLong(biQuadrCoeff, index2 - SieveLimit, biT,
    NumberLength);                       // Ax
  AddBigNbr(biT, biLinearCoeff, biT, NumberLength);     // Ax+B
  if (oddPolynomial)
  {                                                         // Ax+B (odd)
    SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
    SubtractBigNbr(biT, biLinearDelta[0], biT, NumberLength);
  }
  MultBigNbr(biT, biT, biDividend, NumberLength);       // (Ax+B)^2
                                                            // To factor: (Ax+B)^2-N
  SubtractBigNbr(biDividend, TestNbr, biDividend, NumberLength);
  /* factor biDividend */

  NumberLengthDivid = NumberLength; /* Number length for dividend */
  positive = TRUE;
  if (biDividend[NumberLengthDivid - 1] >= 0x40000000)
  { /* Negative */
    positive = FALSE;
    ChSignBigNbr(biDividend, NumberLengthDivid); // Convert to positive
  }
  rowSquares[0] = 1;
  for (index = 0; index < nbrFactorsA; index++)
  {
    DivBigNbrByLong(biDividend, afact[index], biDividend,
      NumberLengthDivid);
    if ((biDividend[NumberLengthDivid - 1] == 0
      && biDividend[NumberLengthDivid - 2] < 0x40000000))
    {
      NumberLengthDivid--;
    }
  }
  nbrColumns = 1;
  if (!positive)
  {                                  // Insert -1 as a factor.
    rowMatrixBbeforeMerge[nbrColumns++] = 0;
  }
  rowMatrixBbeforeMerge[0] = nbrColumns;
  Divid = PerformTrialDivision(primeSieveData, primeTrialDivisionData,
    nbrPrimes, rowMatrixBbeforeMerge,
    nbrFactorsA, index2, biDividend,
    rowSquares, NumberLengthDivid, biT,
    oddPolynomial);
  if (Divid == 1)
  { // Smooth relation found.
    SmoothRelationFound(positive, rowMatrixB,
      rowMatrixBbeforeMerge,
      vectExpParity,
      index2, primeTrialDivisionData, startTime,
      nbrFactorsA, rowSquares,
      biLinearCoeff, NumberLength, biT, biU, biR,
      oddPolynomial);
  }
  else
  {
    if (Divid > 0 && Divid < largePrimeUpperBound)
    {
      PartialRelationFound(positive, rowMatrixB,
        rowMatrixBbeforeMerge,
        vectExpParity,
        index2, primeTrialDivisionData, startTime,
        nbrFactorsA, Divid, rowPartials,
        rowSquares, biLinearCoeff,
        NumberLength, biT, biR, biU, biV,
        indexFactorsA, oddPolynomial);
    }
  }
  return;
}

static long getFactorsOfA(long seed, int nbrFactorsA)
{
  int index, index2, i, tmp;
  for (index = 0; index < nbrFactorsA; index++)
  {
    do
    {
      seed = (1141592621 * seed + 321435) & 0xFFFFFFFFL;
      i = (int)(((seed * span) >> 32) + indexMinFactorA);
      for (index2 = 0; index2 < index; index2++)
      {
        if (aindex[index2] == i || aindex[index2] == i + 1)
        {
          break;
        }
      }
    } while (index2 < index);
    aindex[index] = i;
  }
  for (index = 0; index<nbrFactorsA; index++)    // Sort factors of A.
  {
    for (index2 = index + 1; index2<nbrFactorsA; index2++)
    {
      if (aindex[index] > aindex[index2])
      {
        tmp = aindex[index];
        aindex[index] = aindex[index2];
        aindex[index2] = tmp;
      }
    }
  }
  return seed;
}

/************************************************************************/
/* Multithread procedure:                                               */
/*                                                                      */
/* 1) Main thread generates factor base and other parameters.           */
/* 2) Start M threads where the number M is specified by the user in a  */
/*    box beneath the applet.                                           */
/* 3) For each polynomial:                                              */
/*    3a) Main thread generates the data for the set of 2^n polynomials.*/
/*    3b) Each child thread computes a range of polynomials             */
/*        (u*2^n/M to (u+1)*2^n/M exclusive).                           */
/* Partial and full relation routines must be synchronized.             */
/************************************************************************/
BigInteger FactoringSIQS(BigInteger NbrToFactor)
{
  long FactorBase;
  int currentPrime;
  int NbrMod;
  PrimeSieveData *rowPrimeSieveData;
  PrimeTrialDivisionData *rowPrimeTrialDivisionData;
  long Power2, SqrRootMod, fact;
  int index;
  long D, E, Q, V, W, X, Y, Z, T1, V1, W1, Y1;
  double Temp, Prod;
  double bestadjust;
  int i, j, multiplier;
  int arrmult[] = { 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
    47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };
  double adjustment[sizeof(arrmult)/sizeof(arrmult[0])];
  int halfCurrentPrime;
  double dNumberToFactor, dlogNumberToFactor;
  ValuesSieved = 0;

  nbrThreadFinishedPolySet = 0;
//  threadArray = new Thread[numberThreads];
  nbrPartials = 0;
  congruencesFound = 0;
  Temp = logBigNbr(&NbrToFactor);
  nbrPrimes = (int)exp(sqrt(Temp * log(Temp)) * 0.318);
  SieveLimit = (int)exp(8.5 + 0.015 * Temp) & 0xFFFFFFF8;
  nbrFactorsA = (int)(Temp*0.025 + 1);
  NbrPolynomials = (1 << (nbrFactorsA - 1)) - 1;
  NumberLength = BigNbrToBigInt(NbrToFactor, TestNbr);
  TestNbr[NumberLength++].x = 0;
  memcpy(TestNbr2, TestNbr, NumberLength*sizeof(limb));
  for (i = sizeof(matrixPartialHashIndex)/sizeof(matrixPartialHashIndex[0]) - 1; i >= 0; i--)
  {
    matrixPartialHashIndex[i] = -1;
  }
//  SIQSInfoText = InitSIQSStrings(NbrToFactor, SieveLimit);

  /************************/
  /* Compute startup data */
  /************************/

  /* search for best Knuth-Schroeppel multiplier */
  bestadjust = -10.0e0;
  primeSieveData[0].value = 1;
  primeTrialDivisionData[0].value = 1;
  rowPrimeSieveData = &primeSieveData[1];
  rowPrimeTrialDivisionData = &primeTrialDivisionData[1];
  rowPrimeSieveData->value = 2;
  rowPrimeTrialDivisionData->value = 2;
  // (2^31)^(j+1) mod 2
  rowPrimeTrialDivisionData->exp2 =
    rowPrimeTrialDivisionData->exp3 = rowPrimeTrialDivisionData->exp4 =
    rowPrimeTrialDivisionData->exp5 = rowPrimeTrialDivisionData->exp6 = 0;

  NbrMod = NbrToFactor.limbs[0].x & 7;
  for (j = 0; j<sizeof(arrmult)/sizeof(arrmult[0]); j++)
  {
    int mod = (NbrMod * arrmult[j]) & 7;
    adjustment[j] = 0.34657359; /*  (ln 2)/2  */
    if (mod == 1)
      adjustment[j] *= (4.0e0);
    if (mod == 5)
      adjustment[j] *= (2.0e0);
    adjustment[j] -= log((double)arrmult[j]) / (2.0e0);
  }
  currentPrime = 3;
  while (currentPrime < 10000)
  {
    NbrMod = (int)RemDivBigNbrByLong(TestNbr, currentPrime,
      NumberLength);
    halfCurrentPrime = (currentPrime - 1) / 2;
    int jacobi = (int)intModPow(NbrMod, halfCurrentPrime, currentPrime);
    double dp = (double)currentPrime;
    double logp = log(dp) / dp;
    for (j = 0; j<sizeof(arrmult)/sizeof(arrmult[0]); j++)
    {
      if (arrmult[j] == currentPrime)
      {
        adjustment[j] += logp;
      }
      else if (jacobi * (int)intModPow(arrmult[j], halfCurrentPrime,
        currentPrime) % currentPrime == 1)
      {
        adjustment[j] += 2 * logp;
      }
    }
    do
    {
      currentPrime += 2;
      for (Q = 3; Q * Q <= currentPrime; Q += 2)
      { /* Check if currentPrime is prime */
        if (currentPrime % Q == 0)
        {
          break;  /* Composite */
        }
      }
    } while (Q * Q <= currentPrime);
  }  /* end while */
  multiplier = 1;
  for (j = 0; j<sizeof(arrmult)/sizeof(arrmult[0]); j++)
  {
    if (adjustment[j] > bestadjust)
    { /* find biggest adjustment */
      bestadjust = adjustment[j];
      multiplier = arrmult[j];
    }
  } /* end while */
  MultBigNbrByLong(TestNbr2, multiplier, TestNbr, NumberLength);
  FactorBase = currentPrime;
  matrixBLength = nbrPrimes + 50;
  rowPrimeSieveData->modsqrt = (NbrToFactor.limbs[0].x & 1) ? 1 : 0;
  switch ((int)TestNbr[0].x & 0x07)
  {
  case 1:
    logar2 = (unsigned char)3;
    break;
  case 5:
    logar2 = (unsigned char)1;
    break;
  default:
    logar2 = (unsigned char)1;
    break;
  }
  if (multiplier != 1 && multiplier != 2)
  {
    rowPrimeSieveData = &primeSieveData[2];
    rowPrimeTrialDivisionData = &primeTrialDivisionData[2];
    rowPrimeSieveData->value = multiplier;
    rowPrimeTrialDivisionData->value = multiplier;
    rowPrimeSieveData->modsqrt = 0;
    D = (1L << (2*BITS_PER_GROUP)) % multiplier;
    rowPrimeTrialDivisionData->exp2 = (int)D;  // (2^31)^2 mod multiplier
    D = (D << BITS_PER_GROUP) % multiplier;
    rowPrimeTrialDivisionData->exp3 = (int)D;  // (2^31)^3 mod multiplier
    D = (D << BITS_PER_GROUP) % multiplier;
    rowPrimeTrialDivisionData->exp4 = (int)D;  // (2^31)^4 mod multiplier
    D = (D << BITS_PER_GROUP) % multiplier;
    rowPrimeTrialDivisionData->exp5 = (int)D;  // (2^31)^5 mod multiplier
    D = (D << BITS_PER_GROUP) % multiplier;
    rowPrimeTrialDivisionData->exp6 = (int)D;  // (2^31)^6 mod multiplier
    j = 3;
  }
  else
  {
    j = 2;
  }
  currentPrime = 3;
  while (j < nbrPrimes)
  { /* select small primes */
    NbrMod = (int)RemDivBigNbrByLong(TestNbr, currentPrime,
      NumberLength);
    if (currentPrime != multiplier &&
      intModPow(NbrMod, (currentPrime - 1) / 2, currentPrime) == 1)
    {
      /* use only if Jacobi symbol = 0 or 1 */
      rowPrimeSieveData = &primeSieveData[j];
      rowPrimeTrialDivisionData = &primeTrialDivisionData[j];
      rowPrimeSieveData->value = (int)currentPrime;
      rowPrimeTrialDivisionData->value = (int)currentPrime;
      D = (1L << (2*BITS_PER_GROUP)) % currentPrime;
      rowPrimeTrialDivisionData->exp2 = (int)D; // (2^31)^2 mod currentPrime
      D = (D << BITS_PER_GROUP) % currentPrime;
      rowPrimeTrialDivisionData->exp3 = (int)D; // (2^31)^3 mod currentPrime
      D = (D << BITS_PER_GROUP) % currentPrime;
      rowPrimeTrialDivisionData->exp4 = (int)D; // (2^31)^4 mod currentPrime
      D = (D << BITS_PER_GROUP) % currentPrime;
      rowPrimeTrialDivisionData->exp5 = (int)D; // (2^31)^5 mod currentPrime
      D = (D << BITS_PER_GROUP) % currentPrime;
      rowPrimeTrialDivisionData->exp6 = (int)D; // (2^31)^6 mod currentPrime
      if ((currentPrime & 3) == 3)
      {
        SqrRootMod = intModPow(NbrMod, (currentPrime + 1) / 4, currentPrime);
      }
      else
      {
        if ((currentPrime & 7) == 5)    // currentPrime = 5 (mod 8)
        {
          SqrRootMod =
            intModPow(NbrMod * 2, (currentPrime - 5) / 8, currentPrime);
          SqrRootMod =
            ((((2 * NbrMod * SqrRootMod % currentPrime) * SqrRootMod - 1)
              % currentPrime)
              * NbrMod
              % currentPrime)
            * SqrRootMod
            % currentPrime;
        }
        else
        { /* p = 1 (mod 8) */
          Q = currentPrime - 1;
          E = 0;
          Power2 = 1;
          do
          {
            E++;
            Q /= 2;
            Power2 *= 2;
          } while ((Q & 1) == 0); /* E >= 3 */
          Power2 /= 2;
          X = 1;
          do
          {
            X++;
            Z = intModPow(X, Q, currentPrime);
          } while (intModPow(Z, Power2, currentPrime) == 1);
          Y = Z;
          X = intModPow(NbrMod, (Q - 1) / 2, currentPrime);
          V = NbrMod * X % currentPrime;
          W = V * X % currentPrime;
          while (W != 1)
          {
            T1 = 0;
            D = W;
            while (D != 1)
            {
              D = D * D % currentPrime;
              T1++;
            }
            D = intModPow(Y, 1 << (E - T1 - 1), currentPrime);
            Y1 = D * D % currentPrime;
            E = T1;
            V1 = V * D % currentPrime;
            W1 = W * Y1 % currentPrime;
            Y = Y1;
            V = V1;
            W = W1;
          } /* end while */
          SqrRootMod = V;
        } /* end if */
      } /* end if */
      rowPrimeSieveData->modsqrt = (int)SqrRootMod;
      j++;
    } /* end while */
    do
    {
      currentPrime += 2;
      for (Q = 3; Q * Q <= currentPrime; Q += 2)
      { /* Check if currentPrime is prime */
        if (currentPrime % Q == 0)
        {
          break;  /* Composite */
        }
      }
    } while (Q * Q <= currentPrime);
  } /* End while */

  FactorBase = currentPrime;
  largePrimeUpperBound = 100 * FactorBase;
  dlogNumberToFactor = logBigNbr(&NbrToFactor);
  dNumberToFactor = exp(dlogNumberToFactor);
//  SIQSInfoText += getMultAndFactorBase(multiplier, FactorBase);
//  writeLowerPane(SIQSInfoText);
  firstLimit = 2;
  for (j = 2; j < nbrPrimes; j++)
  {
    firstLimit *= (int)(primeSieveData[j].value);
    if (firstLimit > 2 * SieveLimit)
    {
      break;
    }
  }
  dNumberToFactor *= multiplier;
  smallPrimeUpperLimit = j + 1;
  threshold =
    (unsigned char)(log(
      sqrt(dNumberToFactor) * SieveLimit /
      (FactorBase * 64) /
      primeSieveData[j + 1].value)
      / log(3) + 0x81);
  firstLimit = (int)(log(dNumberToFactor) / 3);
  for (secondLimit = firstLimit; secondLimit < nbrPrimes; secondLimit++)
  {
    if (primeSieveData[secondLimit].value * 2 > SieveLimit)
    {
      break;
    }
  }
  for (thirdLimit = secondLimit; thirdLimit < nbrPrimes; thirdLimit++)
  {
    if (primeSieveData[thirdLimit].value > 2 * SieveLimit)
    {
      break;
    }
  }
  nbrPrimes2 = nbrPrimes - 4;
//  startTime = System.currentTimeMillis();
  // Sieve start time in milliseconds.
  Prod = sqrt(2 * dNumberToFactor) / (double)SieveLimit;
  fact = (long)pow(Prod, 1 / (float)nbrFactorsA);
  for (i = 2;; i++)
  {
    if (primeSieveData[i].value > fact)
    {
      break;
    }
  }
  span = nbrPrimes / (2 * nbrFactorsA*nbrFactorsA);
  if (nbrPrimes < 500)
  {
    span *= 2;
  }
  indexMinFactorA = i - span / 2;
  //this.multiplier = multiplier;
  /*********************************************/
  /* Generate sieve threads                    */
  /*********************************************/
  for (threadNumber = 0; threadNumber<numberThreads; threadNumber++)
  {
    //new Thread(this).start();                // Start new thread.
    //synchronized(amodq)
    {
//      while (threadArray[threadNumber] == null &&
//        getTerminateThread() == FALSE)
      {
        //try
        //{
        //  amodq.wait();
        //}
        //catch (InterruptedException ie) {}
      }
    }
  }
#if 0
  //synchronized(matrixB)
  {
    while (factorSiqs == null && getTerminateThread() == FALSE)
    {
      try
      {
        matrixB.wait();
      }
      catch (InterruptedException ie) {}
    }
  }
#endif
  if (getTerminateThread() || (factorSiqs.nbrLimbs == 1 && factorSiqs.limbs[0].x == 0))
  {
    //throw new ArithmeticException();
  }
#if 0
  for (threadNumber = 0; threadNumber<numberThreads; threadNumber++)
  {                 // Wake up all sieve threads so they can terminate.
    if (threadArray[threadNumber].isAlive())
    {
      //try
      {
        threadArray[threadNumber].interrupt();
      }
      //catch (Exception e) {}
    }
  }
#endif
#if 0
  synchronized(this)
  {
    saveSIQSStatistics(polynomialsSieved, trialDivisions,
      smoothsFound, totalPartials,
      partialsFound, ValuesSieved);
  }
  return factorSiqs;
#endif
}

void ShowSIQSStatus(int matrixBLength, long startTime)
{
  long New, u;
//  long oldTime = getOldTimeElapsed();
#if 0
//  if (getTerminateThread())
//  {
//    throw new ArithmeticException();
//  }
  //    Thread.yield();
  New = System.currentTimeMillis();
  if (oldTime >= 0
    && oldTime / 1000 != (oldTime + New - getOld()) / 1000)
  {
    oldTime += New - getOld();
    setOldTimeElapsed(oldTime);
    setOld(New);
    ShowSIQSInfo(New - startTime, congruencesFound,
      matrixBLength, oldTime / 1000);
  }
#endif
}

static int EraseSingletons(
  int nbrPrimes,
  int *vectExpParity,
  PrimeTrialDivisionData *primeTrialDivisionData)
{
  int row, column, delta;
  int *rowMatrixB;
  int matrixBlength = matrixBLength;
  // Find singletons in matrixB storing in array vectExpParity the number
  // of primes in each column.
  do
  {   // The singleton removal phase must run until there are no more
      // singletons to erase.
    for (column = nbrPrimes - 1; column >= 0; column--)
    {                  // Initialize number of primes per column to zero.
      vectExpParity[column] = 0;
    }
    for (row = matrixBlength - 1; row >= 0; row--)
    {                  // Traverse all rows of the matrix.
      rowMatrixB = matrixB[row];
      for (column = rowMatrixB[LENGTH_OFFSET] - 1; column >= 0; column--)
      {                // A prime appeared in that column.
        vectExpParity[rowMatrixB[column]]++;
      }
    }
    row = 0;
    for (column = 0; column<nbrPrimes; column++)
    {
      if (vectExpParity[column] > 1)
      {                // Useful column found with at least 2 primes.
        newColumns[column] = row;
        primeTrialDivisionData[row++].value =
          primeTrialDivisionData[column].value;
      }
    }
    nbrPrimes = row;
    delta = 0;
    // Erase singletons from matrixB. The rows to be erased are those where the
    // the corresponding element of the array vectExpParity equals 1.
    for (row = 0; row < matrixBlength; row++)
    {                  // Traverse all rows of the matrix.
      rowMatrixB = matrixB[row];
      for (column = rowMatrixB[LENGTH_OFFSET] - 1; column >= 0; column--)
      {                // Traverse all columns.
        if (vectExpParity[rowMatrixB[column]] == 1)
        {              // Singleton found: erase this row.
          delta++;
          break;
        }
      }
      if (column < 0)
      {                // Singleton not found: move row upwards.
        memcpy(matrixB[row - delta], matrixB[row], sizeof(matrixB[0]));
        memcpy(vectLeftHandSide[row - delta], vectLeftHandSide[row], sizeof(vectLeftHandSide[0]));
      }
    }
    matrixBlength -= delta;      // Update number of rows of the matrix.
    for (row = 0; row < matrixBlength; row++)
    {                  // Traverse all rows of the matrix.
      rowMatrixB = matrixB[row];
      for (column = rowMatrixB[LENGTH_OFFSET] - 1; column >= 0; column--)
      {                // Change all column indexes in this row.
        rowMatrixB[column] = newColumns[rowMatrixB[column]];
      }
    }
  } while (delta > 0);           // End loop if number of rows did not
                                 // change.
  primeTrialDivisionData[0].exp2 = nbrPrimes;
  return matrixBlength;
}

/************************/
/* Linear algebra phase */
/************************/
static unsigned char LinearAlgebraPhase(
  int nbrPrimes,
  PrimeTrialDivisionData *primeTrialDivisionData,
  int *vectExpParity,
  int *biT, int *biR, int *biU,
  int NumberLength)
{
  int mask, row, j, index;
  int *rowMatrixB;
  int primeIndex;
  int NumberLengthBak;
  // Get new number of rows after erasing singletons.
  int matrixBlength = EraseSingletons(nbrPrimes,
    vectExpParity, primeTrialDivisionData);
  showMatrixSize(SIQSInfoText, matrixBlength,
    primeTrialDivisionData[0].exp2);
  primeTrialDivisionData[0].exp2 = 0;         // Restore correct value.
  BlockLanczos();
  // The rows of matrixV indicate which rows must be multiplied so no
  // primes are multiplied an odd number of times.
  for (mask = 1; mask != 0; mask *= 2)
  {
    LongToBigNbr(1, biT, NumberLength);
    LongToBigNbr(1, biR, NumberLength);
    for (row = matrixBlength - 1; row >= 0; row--)
    {
      vectExpParity[row] = 0;
    }
    NumberLengthBak = NumberLength;
    if (TestNbr[NumberLength - 1].x == 0 && TestNbr[NumberLength - 2].x < 0x40000000)
    {
      NumberLength--;
    }
    for (row = matrixBlength - 1; row >= 0; row--)
    {
      if ((matrixV[row] & mask) != 0)
      {
        MultBigNbrModN(vectLeftHandSide[row], biR, biU, TestNbr,
          NumberLength);
        for (j = 0; j <= NumberLength; j++)
        {
          biR[j] = biU[j];
        }
        rowMatrixB = matrixB[row];
        for (j = rowMatrixB[LENGTH_OFFSET] - 1; j >= 0; j--)
        {
          primeIndex = rowMatrixB[j];
          vectExpParity[primeIndex] ^= 1;
          if (vectExpParity[primeIndex] == 0)
          {
            if (primeIndex == 0)
            {
              SubtractBigNbr(TestNbr, biT, biT, NumberLength); // Multiply biT by -1.
            }
            else
            {
              MultBigNbrByLongModN(biT,
                primeTrialDivisionData[primeIndex].value, biT,
                TestNbr, NumberLength);
            }
          }
        }
      }
    }
    NumberLength = NumberLengthBak;
    SubtractBigNbrModN(biR, biT, biR, TestNbr, NumberLength);
    GcdBigNbr(biR, TestNbr2, biT, NumberLength);
    index = 0;
    if (biT[0] == 1)
    {
      for (index = 1; index < NumberLength; index++)
      {
        if (biT[index] != 0)
        {
          break;
        }
      }
    }
    if (index < NumberLength)
    { /* GCD is not 1 */
      for (index = 0; index < NumberLength; index++)
      {
        if (biT[index] != TestNbr2[index])
        {
          break;
        }
      }
      if (index < NumberLength)
      { /* GCD is not 1 */
        return TRUE;
      }
    }
  }
  return FALSE;
}

static long intModPow(long NbrMod, long Expon, long currentPrime)
{
  long Power = 1;
  long Square = NbrMod;
  while (Expon != 0)
  {
    if ((Expon & 1) == 1)
    {
      Power = (Power * Square) % currentPrime;
    }
    Square = (Square * Square) % currentPrime;
    Expon >>= 1;
  }
  return Power;
}
static unsigned char InsertNewRelation(
  int *rowMatrixB,
  int *biT, int *biU, int *biR,
  int NumberLength)
{
  int i, k;
  int nbrColumns = rowMatrixB[0] - 1;
  int *curRowMatrixB;
  // Insert it only if it is different from previous relations.
  if (congruencesFound >= matrixBLength)
  {                   // Discard excess congruences.
    return TRUE;
  }
  for (i = 0; i < congruencesFound; i++)
  {
    curRowMatrixB = matrixB[i];
    if (nbrColumns > curRowMatrixB[LENGTH_OFFSET])
    {
      continue;
    }
    if (nbrColumns == curRowMatrixB[LENGTH_OFFSET])
    {
      for (k = 1; k <= nbrColumns; k++)
      {
        if (rowMatrixB[k] != curRowMatrixB[k - 1])
        {
          break;
        }
      }
      if (k > nbrColumns)
      {
        return FALSE; // Do not insert same relation.
      }
      if (rowMatrixB[k] > curRowMatrixB[k - 1])
      {
        continue;
      }
    }
    for (k = congruencesFound - 1; k >= i; k--)
    {
      memcpy(matrixB[k + 1], matrixB[k], sizeof(matrixB[0]));
      memcpy(vectLeftHandSide[k + 1], vectLeftHandSide[k], sizeof(vectLeftHandSide[0]));
    }
    break;
  }
  /* Convert negative numbers to the range 0 <= n < TestNbr */
  if ((TestNbr[0].x & 1) == 0)
  {
    DivBigNbrByLong(TestNbr, 2, TestNbr2, NumberLength);
    // If biR >= TestNbr perform biR = biR - TestNbr.
    for (k = 0; k < NumberLength; k++)
    {
      biT[k] = 0;
    }
    AddBigNbrModN(biR, biT, biR, TestNbr2, NumberLength);
    ModInvBigNbr(biR, biT, TestNbr2, NumberLength);
  }
  else
  {
    ModInvBigNbr(biR, biT, TestNbr, NumberLength);
  }
  if ((biU[NumberLength - 1] & 0x40000000) != 0)
  {
    AddBigNbr(biU, TestNbr, biU, NumberLength);
  }

  // Compute biU / biR  (mod TestNbr)
  MultBigNbrModN(biU, biT, biR, TestNbr, NumberLength);

  // Add relation to matrix B.
  memcpy(matrixB[i], &rowMatrixB[1], nbrColumns * sizeof(int));
  memcpy(vectLeftHandSide[i], biR, NumberLength * sizeof(int));
  congruencesFound++;
  return TRUE;
}

static int modInv(int NbrMod, int currentPrime)
{
  int QQ, T1, T3;
  int V1 = 1;
  int V3 = NbrMod;
  int U1 = 0;
  int U3 = currentPrime;
  while (V3 != 0)
  {
    if (U3 < V3 + V3)
    {               // QQ = 1
      T1 = U1 - V1;
      T3 = U3 - V3;
    }
    else
    {
      QQ = U3 / V3;
      T1 = U1 - V1 * QQ;
      T3 = U3 - V3 * QQ;
    }
    U1 = V1;
    U3 = V3;
    V1 = T1;
    V3 = T3;
  }
  return U1 + (currentPrime & (U1 >> 31));
}

/* Multiply binary matrices of length m x 32 by 32 x 32 */
/* The product matrix has size m x 32. Then add it to a m x 32 matrix. */
static void MatrixMultAdd(int *LeftMatr, int *RightMatr, int *ProdMatr)
{
  int leftMatr;
  int matrLength = matrixBLength;
  int prodMatr;
  int row, col;
  for (row = 0; row < matrLength; row++)
  {
    prodMatr = ProdMatr[row];
    leftMatr = LeftMatr[row];
    col = 0;
    while (leftMatr != 0)
    {
      if (leftMatr < 0)
      {
        prodMatr ^= RightMatr[col];
      }
      leftMatr *= 2;
      col++;
    }
    ProdMatr[row] = prodMatr;
  }
}
/* Multiply binary matrices of length m x 32 by 32 x 32 */
/* The product matrix has size m x 32 */
static void MatrixMultiplication(int *LeftMatr, int *RightMatr, int *ProdMatr)
{
  int leftMatr;
  int matrLength = 32;
  int prodMatr;
  int row, col;
  for (row = 0; row < matrLength; row++)
  {
    prodMatr = 0;
    leftMatr = LeftMatr[row];
    col = 0;
    while (leftMatr != 0)
    {
      if (leftMatr < 0)
      {
        prodMatr ^= RightMatr[col];
      }
      leftMatr *= 2;
      col++;
    }
    ProdMatr[row] = prodMatr;
  }
}

/* Multiply the transpose of a binary matrix of length n x 32 by */
/* another binary matrix of length n x 32 */
/* The product matrix has size 32 x 32 */
static void MatrTranspMult(int matrLength, int *LeftMatr, int *RightMatr, int *ProdMatr)
{
  int prodMatr;
  int row, col;
  int iMask = 1;
  for (col = 31; col >= 0; col--)
  {
    prodMatr = 0;
    for (row = 0; row < matrLength; row++)
    {
      if ((LeftMatr[row] & iMask) != 0)
      {
        prodMatr ^= RightMatr[row];
      }
    }
    ProdMatr[col] = prodMatr;
    iMask *= 2;
  }
}

static void MatrixAddition(int *leftMatr, int *rightMatr, int *sumMatr)
{
  for (int row = 32 - 1; row >= 0; row--)
  {
    sumMatr[row] = leftMatr[row] ^ rightMatr[row];
  }
}

static void MatrMultBySSt(int length, int *Matr, int diagS, int *Prod)
{
  for (int row = length - 1; row >= 0; row--)
  {
    Prod[row] = diagS & Matr[row];
  }
}

/* Compute Bt * B * input matrix where B is the matrix that holds the */
/* factorization relations */
static void MultiplyAByMatrix(int *Matr, int *TempMatr, int *ProdMatr)
{
  int index;
  int prodMatr;
  int row;
  int *rowMatrixB;

  /* Compute TempMatr = B * Matr */
  for (row = matrixBLength - 1; row >= 0; row--)
  {
    TempMatr[row] = 0;
  }
  for (row = matrixBLength - 1; row >= 0; row--)
  {
    rowMatrixB = matrixB[row];
    for (index = rowMatrixB[LENGTH_OFFSET] - 1; index >= 0; index--)
    {
      TempMatr[rowMatrixB[index]] ^= Matr[row];
    }
  }

  /* Compute ProdMatr = Bt * TempMatr */
  for (row = matrixBLength - 1; row >= 0; row--)
  {
    prodMatr = 0;
    rowMatrixB = matrixB[row];
    for (index = rowMatrixB[LENGTH_OFFSET] - 1; index >= 0; index--)
    {
      prodMatr ^= TempMatr[rowMatrixB[index]];
    }
    ProdMatr[row] = prodMatr;
  }
}

static void colexchange(int *XmY, int *V, int *V1, int *V2,
  int col1, int col2)
{
  int row;
  int mask1, mask2;
  int *matr1, *matr2;

  if (col1 == col2)
  {          // Cannot exchange the same column.
    return;
  }          // Exchange columns col1 and col2 of V1:V2
  mask1 = 0x80000000 >> (col1 & 31);
  mask2 = 0x80000000 >> (col2 & 31);
  matr1 = (col1 >= 32 ? V1 : V2);
  matr2 = (col2 >= 32 ? V1 : V2);
  for (row = matrixBLength - 1; row >= 0; row--)
  {             // If both bits are different toggle them.
    if (((matr1[row] & mask1) == 0) != ((matr2[row] & mask2) == 0))
    {           // If both bits are different toggle them.
      matr1[row] ^= mask1;
      matr2[row] ^= mask2;
    }
  }
  // Exchange columns col1 and col2 of XmY:V
  matr1 = (col1 >= 32 ? XmY : V);
  matr2 = (col2 >= 32 ? XmY : V);
  for (row = matrixBLength - 1; row >= 0; row--)
  {             // If both bits are different toggle them.
    if (((matr1[row] & mask1) == 0) != ((matr2[row] & mask2) == 0))
    {
      matr1[row] ^= mask1;
      matr2[row] ^= mask2;
    }
  }
}

static void coladd(int *XmY, int *V, int *V1, int *V2,
  int col1, int col2)
{
  int row;
  int mask1, mask2;
  int *matr1, *matr2;

  if (col1 == col2)
  {
    return;
  }               // Add column col1 to column col2 of V1:V2
  mask1 = 0x80000000 >> (col1 & 31);
  mask2 = 0x80000000 >> (col2 & 31);
  matr1 = (col1 >= 32 ? V1 : V2);
  matr2 = (col2 >= 32 ? V1 : V2);
  for (row = matrixBLength - 1; row >= 0; row--)
  {              // If bit to add is '1'...
    if ((matr1[row] & mask1) != 0)
    {            // Toggle bit in destination.
      matr2[row] ^= mask2;
    }
  }
  // Add column col1 to column col2 of XmY:V
  matr1 = (col1 >= 32 ? XmY : V);
  matr2 = (col2 >= 32 ? XmY : V);
  for (row = matrixBLength - 1; row >= 0; row--)
  {              // If bit to add is '1'...
    if ((matr1[row] & mask1) != 0)
    {            // Toggle bit in destination.
      matr2[row] ^= mask2;
    }
  }
}

static void BlockLanczos(void)
{
  int i, j, k;
  int oldDiagonalSSt, newDiagonalSSt;
  int index, indexC, mask;
  int matrixD[32];
  int matrixE[32];
  int matrixF[32];
  int matrixWinv[32];
  int matrixWinv1[32];
  int matrixWinv2[32];
  int matrixVtV0[32];
  int matrixVt1V0[32];
  int matrixVt2V0[32];
  int matrixVtAV[32];
  int matrixVt1AV1[32];
  int matrixCalcParenD[32];
  int vectorIndex[64];
  int matrixTemp[32];
  int matrixCalc1[32]; // Matrix that holds temporary data
  int matrixCalc2[32]; // Matrix that holds temporary data
  int *matr;
  int rowMatrixV;
  int rowMatrixXmY;
  double seed;
  int Temp, Temp1;
  int stepNbr = 0;
  int currentOrder, currentMask;
  int row, col;
  int leftCol, rightCol;
  int minind, min, minanswer;
  int *rowMatrixB;

  newDiagonalSSt = oldDiagonalSSt = -1;

  /* Initialize matrix X-Y and matrix V_0 with random data */
  seed = (double)123456789;
  for (i = matrixBLength - 1; i >= 0; i--)
  {
    matrixXmY[i] = (int)seed;
    seed = (int)(seed * 62089911L / 0x7FFFFFFF + 54325442);
    matrixXmY[i] += (int)seed;
    seed = (int)(seed * 62089911L / 0x7FFFFFFF + 54325442);
    matrixV[i] = (int)seed;
    seed = (int)(seed * 62089911L / 0x7FFFFFFF + 54325442);
    matrixV[i] += (int)seed;
    seed = (int)(seed * 62089911L / 0x7FFFFFFF + 54325442);
  }
  // Compute matrix Vt(0) * V(0)
  MatrTranspMult(matrixBLength, matrixV, matrixV, matrixVtV0);
  for (;;)
  {
    //if (getTerminateThread())
    //{
    //  throw new ArithmeticException();
    // }
    oldDiagonalSSt = newDiagonalSSt;
    stepNbr++;
    // Compute matrix A * V(i)
    MultiplyAByMatrix(matrixV, matrixCalc3, matrixAV);
    // Compute matrix Vt(i) * A * V(i)
    MatrTranspMult(matrixBLength, matrixV, matrixAV, matrixVtAV);

    /* If Vt(i) * A * V(i) = 0, end of loop */
    for (i = sizeof(matrixVtAV)/sizeof(matrixVtAV[0]) - 1; i >= 0; i--)
    {
      if (matrixVtAV[i] != 0)
      {
        break;
      }
    }
    if (i < 0)
    {
      break;
    } /* End X-Y calculation loop */

      /* Selection of S(i) and W(i) */

    memcpy(matrixTemp, matrixWinv2, sizeof(matrixTemp));
    memcpy(matrixWinv2, matrixWinv1, sizeof(matrixTemp));
    memcpy(matrixWinv1, matrixWinv, sizeof(matrixTemp));
    memcpy(matrixWinv, matrixTemp, sizeof(matrixTemp));

    mask = 1;
    for (j = 31; j >= 0; j--)
    {
      matrixD[j] = matrixVtAV[j]; /*  D = VtAV    */
      matrixWinv[j] = mask; /*  Winv = I    */
      mask *= 2;
    }

    index = 31;
    indexC = 31;
    for (mask = 1; mask != 0; mask *= 2)
    {
      if ((oldDiagonalSSt & mask) != 0)
      {
        matrixE[index] = indexC;
        matrixF[index] = mask;
        index--;
      }
      indexC--;
    }
    indexC = 31;
    for (mask = 1; mask != 0; mask *= 2)
    {
      if ((oldDiagonalSSt & mask) == 0)
      {
        matrixE[index] = indexC;
        matrixF[index] = mask;
        index--;
      }
      indexC--;
    }
    newDiagonalSSt = 0;
    for (j = 0; j < 32; j++)
    {
      currentOrder = matrixE[j];
      currentMask = matrixF[j];
      for (k = j; k < 32; k++)
      {
        if ((matrixD[matrixE[k]] & currentMask) != 0)
        {
          break;
        }
      }
      if (k < 32)
      {
        i = matrixE[k];
        Temp = matrixWinv[i];
        matrixWinv[i] = matrixWinv[currentOrder];
        matrixWinv[currentOrder] = Temp;
        Temp1 = matrixD[i];
        matrixD[i] = matrixD[currentOrder];
        matrixD[currentOrder] = Temp1;
        newDiagonalSSt |= currentMask;
        for (k = 31; k >= 0; k--)
        {
          if (k != currentOrder && ((matrixD[k] & currentMask) != 0))
          {
            matrixWinv[k] ^= Temp;
            matrixD[k] ^= Temp1;
          }
        } /* end for k */
      }
      else
      {
        for (k = j; k < 32; k++)
        {
          if ((matrixWinv[matrixE[k]] & currentMask) != 0)
          {
            break;
          }
        }
        i = matrixE[k];
        Temp = matrixWinv[i];
        matrixWinv[i] = matrixWinv[currentOrder];
        matrixWinv[currentOrder] = Temp;
        Temp1 = matrixD[i];
        matrixD[i] = matrixD[currentOrder];
        matrixD[currentOrder] = Temp1;
        for (k = 31; k >= 0; k--)
        {
          if ((matrixWinv[k] & currentMask) != 0)
          {
            matrixWinv[k] ^= Temp;
            matrixD[k] ^= Temp1;
          }
        } /* end for k */
      } /* end if */
    } /* end for j */
      /* Compute D(i), E(i) and F(i) */
    if (stepNbr >= 3)
    {
      // F = -Winv(i-2) * (I - Vt(i-1)*A*V(i-1)*Winv(i-1)) * ParenD * S*St
      MatrixMultiplication(matrixVt1AV1, matrixWinv1, matrixCalc2);
      index = 31; /* Add identity matrix */
      for (mask = 1; mask != 0; mask *= 2)
      {
        matrixCalc2[index] ^= mask;
        index--;
      }
      MatrixMultiplication(matrixWinv2, matrixCalc2, matrixCalc1);
      MatrixMultiplication(matrixCalc1, matrixCalcParenD, matrixF);
      MatrMultBySSt(32, matrixF, newDiagonalSSt, matrixF);
    }
    // E = -Winv(i-1) * Vt(i)*A*V(i) * S*St
    if (stepNbr >= 2)
    {
      MatrixMultiplication(matrixWinv1, matrixVtAV, matrixE);
      MatrMultBySSt(32, matrixE, newDiagonalSSt, matrixE);
    }
    // ParenD = Vt(i)*A*A*V(i) * S*St + Vt(i)*A*V(i)
    // D = I - Winv(i) * ParenD
    MatrTranspMult(matrixBLength, matrixAV, matrixAV, matrixCalc1); // Vt(i)*A*A*V(i)
    MatrMultBySSt(32, matrixCalc1, newDiagonalSSt, matrixCalc1);
    MatrixAddition(matrixCalc1, matrixVtAV, matrixCalcParenD);
    MatrixMultiplication(matrixWinv, matrixCalcParenD, matrixD);
    index = 31; /* Add identity matrix */
    for (mask = 1; mask != 0; mask *= 2)
    {
      matrixD[index] ^= mask;
      index--;
    }

    /* Update value of X - Y */
    MatrixMultiplication(matrixWinv, matrixVtV0, matrixCalc1);
    MatrixMultAdd(matrixV, matrixCalc1, matrixXmY);

    /* Compute value of new matrix V(i) */
    // V(i+1) = A * V(i) * S * St + V(i) * D + V(i-1) * E + V(i-2) * F
    MatrMultBySSt(matrixBLength, matrixAV, newDiagonalSSt, matrixCalc3);
    MatrixMultAdd(matrixV, matrixD, matrixCalc3);
    if (stepNbr >= 2)
    {
      MatrixMultAdd(matrixV1, matrixE, matrixCalc3);
      if (stepNbr >= 3)
      {
        MatrixMultAdd(matrixV2, matrixF, matrixCalc3);
      }
    }
    /* Compute value of new matrix Vt(i)V0 */
    // Vt(i+1)V(0) = Dt * Vt(i)V(0) + Et * Vt(i-1)V(0) + Ft * Vt(i-2)V(0)
    MatrTranspMult(32, matrixD, matrixVtV0, matrixCalc2);
    if (stepNbr >= 2)
    {
      MatrTranspMult(32, matrixE, matrixVt1V0, matrixCalc1);
      MatrixAddition(matrixCalc1, matrixCalc2, matrixCalc2);
      if (stepNbr >= 3)
      {
        MatrTranspMult(32, matrixF, matrixVt2V0, matrixCalc1);
        MatrixAddition(matrixCalc1, matrixCalc2, matrixCalc2);
      }
    }
    memcpy(matrixTemp2, matrixV2, sizeof(matrixTemp2));
    memcpy(matrixV2, matrixV1, sizeof(matrixV2));
    memcpy(matrixV1, matrixV, sizeof(matrixV1));
    memcpy(matrixV, matrixCalc3, sizeof(matrixV));
    memcpy(matrixCalc3, matrixTemp2, sizeof(matrixCalc3));
    memcpy(matrixTemp, matrixVt2V0, sizeof(matrixTemp));
    memcpy(matrixVt2V0, matrixVt1V0, sizeof(matrixVt2V0));
    memcpy(matrixVt1V0, matrixVtV0, sizeof(matrixVt1V0));
    memcpy(matrixVtV0, matrixCalc2, sizeof(matrixVtV0));
    memcpy(matrixCalc2, matrixTemp, sizeof(matrixCalc2));
    memcpy(matrixTemp, matrixVt1AV1, sizeof(matrixTemp));
    memcpy(matrixVt1AV1, matrixVtAV, sizeof(matrixVt1AV1));
    memcpy(matrixVtAV, matrixTemp, sizeof(matrixVtAV));
  } /* end while */

    /* Find matrix V1:V2 = B * (X-Y:V) */
  for (row = matrixBLength - 1; row >= 0; row--)
  {
    matrixV1[row] = matrixV2[row] = 0;
  }
  for (row = matrixBLength - 1; row >= 0; row--)
  {
    rowMatrixB = matrixB[row];
    rowMatrixXmY = matrixXmY[row];
    rowMatrixV = matrixV[row];
    // The vector rowMatrixB includes the indexes of the columns set to '1'.
    for (index = rowMatrixB[LENGTH_OFFSET] - 1; index >= 0; index--)
    {
      col = rowMatrixB[index];
      matrixV1[col] ^= rowMatrixXmY;
      matrixV2[col] ^= rowMatrixV;
    }
  }
  rightCol = 64;
  leftCol = 0;
  while (leftCol < rightCol)
  {
    for (col = leftCol; col < rightCol; col++)
    {       // For each column find the first row which has a '1'.
            // Columns outside this range must have '0' in all rows.
      matr = (col >= 32 ? matrixV1 : matrixV2);
      mask = 0x80000000 >> (col & 31);
      vectorIndex[col] = -1;    // indicate all rows in zero in advance.
      for (row = 0; row < matrixBLength; row++)
      {
        if ((matr[row] & mask) != 0)
        {               // First row for this mask is found. Store it.
          vectorIndex[col] = row;
          break;
        }
      }
    }
    for (col = leftCol; col < rightCol; col++)
    {
      if (vectorIndex[col] < 0)
      {  // If all zeros in col 'col', exchange it with first column with
         // data different from zero (leftCol).
        colexchange(matrixXmY, matrixV, matrixV1, matrixV2, leftCol, col);
        vectorIndex[col] = vectorIndex[leftCol];
        vectorIndex[leftCol] = -1;  // This column now has zeros.
        leftCol++;                  // Update leftCol to exclude that column.
      }
    }
    if (leftCol == rightCol)
    {
      break;
    }
    // At this moment all columns from leftCol to rightCol are non-zero.
    // Get the first row that includes a '1'.
    min = vectorIndex[leftCol];
    minind = leftCol;
    for (col = leftCol + 1; col < rightCol; col++)
    {
      if (vectorIndex[col] < min)
      {
        min = vectorIndex[col];
        minind = col;
      }
    }
    minanswer = 0;
    for (col = leftCol; col < rightCol; col++)
    {
      if (vectorIndex[col] == min)
      {
        minanswer++;
      }
    }
    if (minanswer > 1)
    {            // Two columns with the same first row to '1'.
      for (col = minind + 1; col < rightCol; col++)
      {
        if (vectorIndex[col] == min)
        {        // Add first column which has '1' in the same row to
                 // the other columns so they have '0' in this row after
                 // this operation.
          coladd(matrixXmY, matrixV, matrixV1, matrixV2, minind, col);
        }
      }
    }
    else
    {
      rightCol--;
      colexchange(matrixXmY, matrixV, matrixV1, matrixV2, minind, rightCol);
    }
  }
  leftCol = 0; /* find linear independent solutions */
  while (leftCol < rightCol)
  {
    for (col = leftCol; col < rightCol; col++)
    {         // For each column find the first row which has a '1'.
      matr = (col >= 32 ? matrixXmY : matrixV);
      mask = 0x80000000 >> (col & 31);
      vectorIndex[col] = -1;    // indicate all rows in zero in advance.
      for (row = 0; row < matrixBLength; row++)
      {
        if ((matr[row] & mask) != 0)
        {         // First row for this mask is found. Store it.
          vectorIndex[col] = row;
          break;
        }
      }
    }
    for (col = leftCol; col < rightCol; col++)
    {  // If all zeros in col 'col', exchange it with last column with
       // data different from zero (rightCol).
      if (vectorIndex[col] < 0)
      {
        rightCol--;                 // Update rightCol to exclude that column.
        colexchange(matrixXmY, matrixV, matrixV1, matrixV2, rightCol, col);
        vectorIndex[col] = vectorIndex[rightCol];
        vectorIndex[rightCol] = -1; // This column now has zeros.
      }
    }
    if (leftCol == rightCol)
    {
      break;
    }
    // At this moment all columns from leftCol to rightCol are non-zero.
    // Get the first row that includes a '1'.
    min = vectorIndex[leftCol];
    minind = leftCol;
    for (col = leftCol + 1; col < rightCol; col++)
    {
      if (vectorIndex[col] < min)
      {
        min = vectorIndex[col];
        minind = col;
      }
    }
    minanswer = 0;
    for (col = leftCol; col < rightCol; col++)
    {
      if (vectorIndex[col] == min)
      {
        minanswer++;
      }
    }
    if (minanswer > 1)
    {            // At least two columns with the same first row to '1'.
      for (col = minind + 1; col < rightCol; col++)
      {
        if (vectorIndex[col] == min)
        {        // Add first column which has '1' in the same row to
                 // the other columns so they have '0' in this row after
                 // this operation.
          coladd(matrixXmY, matrixV, matrixV1, matrixV2, minind, col);
        }
      }
    }
    else
    {
      colexchange(matrixXmY, matrixV, matrixV1, matrixV2, minind, leftCol);
      leftCol++;
    }
  }
  return matrixV;
}

/****************/
/* Sieve thread */
/****************/
void run(void)
{
  int polySet;
  int biT[20];
  int biU[20];
  int biV[20];
  int biR[20];
//  int multiplier = this.multiplier;
  PrimeSieveData *rowPrimeSieveData;
  PrimeSieveData *rowPrimeSieveData0;
  PrimeTrialDivisionData *rowPrimeTrialDivisionData;
  short SieveArray[100000];
  int rowPartials[200];
  int biLinearCoeff[20];
//  int threadNumber = this.threadNumber;
  int biDividend[20];
  int biAbsLinearCoeff[20];
  int indexFactorsA[50];
  int rowSquares[200];
  int polynomialsPerThread = ((NbrPolynomials - 1) / numberThreads) & 0xFFFFFFFE;
  int firstPolynomial = threadNumber*polynomialsPerThread;
  int lastPolynomial = firstPolynomial + polynomialsPerThread;
  firstPolynomial |= 1;
  int grayCode = firstPolynomial ^ (firstPolynomial >> 1);
  firstPolynomial++;
  int i, PolynomialIndex, index, index2;
  int currentPrime;
  long Rem, RemB, remE, D, Q;
  int Dividend[25];
  int rowMatrixBbeforeMerge[200];
  int rowMatrixB[200];
  unsigned char positive;
  BigInteger result;
  int inverseA, twiceInverseA;
  int NumberLengthA, NumberLengthB;
  NumberLength++;

//  synchronized(amodq)
  {
    if (threadNumber == 0)
    {
//      primeSieveData = this.primeSieveData;
    }
    else
    {
      for (i = 0; i<nbrPrimes; i++)
      {
        rowPrimeSieveData = &primeSieveData[i];
        rowPrimeSieveData0 = &primeSieveData[i];
        rowPrimeSieveData->value = rowPrimeSieveData0->value;
        rowPrimeSieveData->modsqrt = rowPrimeSieveData0->modsqrt;
        memcpy(rowPrimeSieveData->Bainv2, rowPrimeSieveData0->Bainv2, sizeof(rowPrimeSieveData0->Bainv2));
      }
    }
//    threadArray[threadNumber] = Thread.currentThread();
//    amodq.notifyAll();
  }               // End synchronized block.
    for (polySet = 1;; polySet++)
    {                         // For each polynomial set...
      //if (getTerminateThread())
      //{
      //  throw new ArithmeticException();
      //}
      //synchronized(amodq)
      {
        nbrThreadFinishedPolySet++;
        if (congruencesFound >= matrixBLength/* || factorSiqs != null*/)
        {
          if (nbrThreadFinishedPolySet < polySet * numberThreads)
          {
            return;
          }
          if (1/*factorSiqs == null*/)
          {
            while (!LinearAlgebraPhase(nbrPrimes,
              primeTrialDivisionData,
              vectExpParity,
              biT, biR, biU, NumberLength));
            /*result = BigIntToBigNbr(biT, NumberLength);  // Factor found.*/
#if 0
            synchronized(matrixB)
            {
              factorSiqs = result;
              matrixB.notify();
            }
#endif
          }
          else
          {
#if 0
            synchronized(matrixB)
            {
              matrixB.notify();
            }
#endif
          }
          return;
        }
        if (nbrThreadFinishedPolySet == polySet * numberThreads)
        {
          /*********************************************/
          /* Initialization stage for first polynomial */
          /*********************************************/
          firstPrimeSieveData = primeSieveData;
          oldSeed = newSeed;
          newSeed = getFactorsOfA(oldSeed, nbrFactorsA);
          for (index = 0; index<nbrFactorsA; index++)
          {                        // Get the values of the factors of A.
            afact[index] = primeSieveData[aindex[index]].value;
          }
          // Compute the leading coefficient in biQuadrCoeff.

          LongToBigNbr(afact[0], biQuadrCoeff, NumberLength);
          for (index = 1; index < nbrFactorsA; index++)
          {
            MultBigNbrByLong(biQuadrCoeff, afact[index], biQuadrCoeff,
              NumberLength);
          }
          for (NumberLengthA = NumberLength; NumberLengthA >= 2; NumberLengthA--)
          {
            if (biQuadrCoeff[NumberLengthA - 1] != 0 ||
              biQuadrCoeff[NumberLengthA - 2] >= 0x40000000)
            {
              break;
            }
          }
          for (index = 0; index < nbrFactorsA; index++)
          {
            currentPrime = (int)afact[index];
            D = RemDivBigNbrByLong(biQuadrCoeff,
              currentPrime*currentPrime, NumberLengthA) / currentPrime;
            Q = (long)primeSieveData[aindex[index]].modsqrt *
              modInv((int)D, currentPrime) % currentPrime;
            amodq[index] = (int)D << 1;
            tmodqq[index] = (int)RemDivBigNbrByLong(TestNbr,
              currentPrime*currentPrime, NumberLength);
            if (Q + Q > currentPrime)
            {
              Q = currentPrime - Q;
            }
            DivBigNbrByLong(biQuadrCoeff, currentPrime, biDividend,
              NumberLengthA);
            MultBigNbrByLong(biDividend, Q, biLinearDelta[index],
              NumberLengthA);
            for (index2 = NumberLengthA; index2 < NumberLength; index2++)
            {
              biLinearDelta[index][index2] = 0;
            }
          }
          for (index = 1; index < nbrPrimes; index++)
          {
            rowPrimeTrialDivisionData = &primeTrialDivisionData[index];
            rowPrimeSieveData = &primeSieveData[index];
            // Get current prime.
            currentPrime = rowPrimeTrialDivisionData->value;
            memcpy(Dividend, biQuadrCoeff, sizeof(Dividend));   // Get A mod current prime.
            switch (NumberLengthA)
            {
            case 7:
              Rem = (long)Dividend[6] * rowPrimeTrialDivisionData->exp6 +
                (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
              break;
            case 6:
              Rem = (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
              break;
            case 5:
              Rem = (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
              break;
            case 4:
              Rem = (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
              break;
            default:
              Rem = (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
              break;
            }
            // Get its inverse
            inverseA = modInv((int)((Rem + ((long)Dividend[1] << 31) + Dividend[0]) %
              currentPrime), currentPrime);
            twiceInverseA = inverseA << 1;       // and twice this value.
            rowPrimeSieveData->difsoln = (int)((long)twiceInverseA *
              rowPrimeSieveData->modsqrt % currentPrime);
            switch (NumberLengthA)
            {
            case 7:
              for (index2 = nbrFactorsA - 1; index2 > 0; index2--)
              {
                memcpy(Dividend, biLinearDelta[index2], sizeof(Dividend));
                remE = ((long)Dividend[6] * rowPrimeTrialDivisionData->exp6 +
                  (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                  (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                  (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                  (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                  ((long)Dividend[1] << 31) + Dividend[0]) %
                  currentPrime;
                rowPrimeSieveData->Bainv2[index2 - 1] =
                  (int)(remE * twiceInverseA % currentPrime);
              }
              memcpy(Dividend, biLinearDelta[0], sizeof(Dividend));
              remE = ((long)Dividend[6] * rowPrimeTrialDivisionData->exp6 +
                (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                ((long)Dividend[1] << 31) + Dividend[0]) %
                currentPrime;
              break;
            case 6:
              for (index2 = nbrFactorsA - 1; index2 > 0; index2--)
              {
                memcpy(Dividend, biLinearDelta[index2], sizeof(Dividend));
                remE = ((long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                  (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                  (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                  (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                  ((long)Dividend[1] << 31) + Dividend[0]) %
                  currentPrime;
                rowPrimeSieveData->Bainv2[index2 - 1] =
                  (int)(remE * twiceInverseA % currentPrime);
              }
              memcpy(Dividend, biLinearDelta[0], sizeof(Dividend));
              remE = ((long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
                (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                ((long)Dividend[1] << 31) + Dividend[0]) %
                currentPrime;
              break;
            case 5:
              for (index2 = nbrFactorsA - 1; index2 > 0; index2--)
              {
                memcpy(Dividend, biLinearDelta[index2], sizeof(Dividend));
                remE = ((long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                  (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                  (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                  ((long)Dividend[1] << 31) + Dividend[0]) %
                  currentPrime;
                rowPrimeSieveData->Bainv2[index2 - 1] =
                  (int)(remE * twiceInverseA % currentPrime);
              }
              memcpy(Dividend, biLinearDelta[0], sizeof(Dividend));
              remE = ((long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
                (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                ((long)Dividend[1] << 31) + Dividend[0]) %
                currentPrime;
              break;
            case 4:
              for (index2 = nbrFactorsA - 1; index2 > 0; index2--)
              {
                memcpy(Dividend, biLinearDelta[index2], sizeof(Dividend));
                remE = ((long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                  (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                  ((long)Dividend[1] << 31) + Dividend[0]) %
                  currentPrime;
                rowPrimeSieveData->Bainv2[index2 - 1] =
                  (int)(remE * twiceInverseA % currentPrime);
              }
              memcpy(Dividend, biLinearDelta[0], sizeof(Dividend));
              remE = ((long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
                (long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                ((long)Dividend[1] << 31) + Dividend[0]) %
                currentPrime;
              break;
            default:
              for (index2 = nbrFactorsA - 1; index2 > 0; index2--)
              {
                memcpy(Dividend, biLinearDelta[index2], sizeof(Dividend));
                remE = ((long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                  ((long)Dividend[1] << 31) + Dividend[0]) %
                  currentPrime;
                rowPrimeSieveData->Bainv2[index2 - 1] =
                  (int)(remE * twiceInverseA % currentPrime);
              }
              memcpy(Dividend, biLinearDelta[0], sizeof(Dividend));
              remE = ((long)Dividend[2] * rowPrimeTrialDivisionData->exp2 +
                ((long)Dividend[1] << 31) + Dividend[0]) %
                currentPrime;
              break;
            }
            rowPrimeSieveData->Bainv2_0 =
              (int)(remE * twiceInverseA % currentPrime);
            if (rowPrimeSieveData->Bainv2_0 != 0)
            {
              rowPrimeSieveData->Bainv2_0 =
                currentPrime - rowPrimeSieveData->Bainv2_0;
            }
          }
          for (index2 = 0; index2 < nbrFactorsA; index2++)
          {
            primeSieveData[aindex[index2]].difsoln = -1; // Do not sieve.
          }
          //synchronized(TestNbr2)
          {
            //TestNbr2.notifyAll();
          }
        }           // End initializing first polynomial
      }             // End synchronized
#if 0
      synchronized(TestNbr2)
      {
        while (nbrThreadFinishedPolySet < polySet * numberThreads)
        {
          try
          {
            TestNbr2.wait();
          }
          catch (InterruptedException ie) {}
        }
      }
#endif
      if (/*factorSiqs != null ||*/ congruencesFound >= matrixBLength)
      {
        if (nbrThreadFinishedPolySet > numberThreads*polySet)
        {
          continue;
        }
        //synchronized(amodq)
        {
          nbrThreadFinishedPolySet++;
        }
        return;
      }
      PolynomialIndex = firstPolynomial;
      // Compute first polynomial parameters.
      for (i = 0; i<NumberLength; i++)
      {
        biLinearCoeff[i] = biLinearDelta[0][i];
      }
      for (i = 1; i<nbrFactorsA; i++)
      {
        if ((grayCode & (1 << i)) == 0)
        {
          AddBigNbr(biLinearCoeff, biLinearDelta[i], biLinearCoeff,
            NumberLength);
        }
        else
        {
          SubtractBigNbr(biLinearCoeff, biLinearDelta[i], biLinearCoeff,
            NumberLength);
        }
      }
      for (NumberLengthA = NumberLength; NumberLengthA >= 2; NumberLengthA--)
      {
        if (biQuadrCoeff[NumberLengthA - 1] != 0 ||
          biQuadrCoeff[NumberLengthA - 2] >= 0x40000000)
        {                             // Go out if significant limb.
          break;
        }
      }
      if (biLinearCoeff[NumberLength - 1] >= 0x40000000)
      {                               // Number is negative.
        positive = FALSE;
        memcpy(biT, biLinearCoeff, NumberLength * sizeof(biT[0]));
        ChSignBigNbr(biT, NumberLength);   // Make it positive.
        memcpy(biAbsLinearCoeff, biT, sizeof(biT));
      }
      else
      {
        positive = TRUE;                       // B is positive.
                                               // Get B mod current prime. 
        memcpy(biAbsLinearCoeff, biLinearCoeff, sizeof(biLinearCoeff));
      }
      for (NumberLengthB = NumberLength; NumberLengthB >= 2; NumberLengthB--)
      {
        if (biAbsLinearCoeff[NumberLengthB - 1] != 0 ||
          biAbsLinearCoeff[NumberLengthB - 2] >= 0x40000000)
        {                                // Go out if significant limb.
          break;
        }
      }
      for (i = nbrPrimes - 1; i>0; i--)
      {
        rowPrimeSieveData = &primeSieveData[i];
        rowPrimeSieveData0 = firstPrimeSieveData+i;
        rowPrimeSieveData->difsoln = rowPrimeSieveData0->difsoln;
        rowPrimeSieveData->Bainv2_0 = rowPrimeSieveData0->Bainv2_0;
        rowPrimeTrialDivisionData = &primeTrialDivisionData[i];
        currentPrime = rowPrimeTrialDivisionData->value;     // Get current prime.
        memcpy(Dividend, biQuadrCoeff,sizeof(biQuadrCoeff)); // Get A mod current prime.
        switch (NumberLengthA)
        {
        case 7:
          Rem = (long)Dividend[6] * rowPrimeTrialDivisionData->exp6 +
            (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
            (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
            (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
            (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 6:
          Rem = (long)Dividend[5] * rowPrimeTrialDivisionData->exp5 +
            (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
            (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
            (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 5:
          Rem = (long)Dividend[4] * rowPrimeTrialDivisionData->exp4 +
            (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
            (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 4:
          Rem = (long)Dividend[3] * rowPrimeTrialDivisionData->exp3 +
            (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 3:
          Rem = (long)Dividend[2] * rowPrimeTrialDivisionData->exp2;
          break;
        default:
          Rem = 0;
          break;
        }
        // Get its inverse
        inverseA = modInv((int)((Rem + ((long)Dividend[1] << 31) + Dividend[0]) %
          currentPrime), currentPrime);
        switch (NumberLengthB)
        {
        case 7:
          Rem = (long)biAbsLinearCoeff[6] * rowPrimeTrialDivisionData->exp6 +
            (long)biAbsLinearCoeff[5] * rowPrimeTrialDivisionData->exp5 +
            (long)biAbsLinearCoeff[4] * rowPrimeTrialDivisionData->exp4 +
            (long)biAbsLinearCoeff[3] * rowPrimeTrialDivisionData->exp3 +
            (long)biAbsLinearCoeff[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 6:
          Rem = (long)biAbsLinearCoeff[5] * rowPrimeTrialDivisionData->exp5 +
            (long)biAbsLinearCoeff[4] * rowPrimeTrialDivisionData->exp4 +
            (long)biAbsLinearCoeff[3] * rowPrimeTrialDivisionData->exp3 +
            (long)biAbsLinearCoeff[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 5:
          Rem = (long)biAbsLinearCoeff[4] * rowPrimeTrialDivisionData->exp4 +
            (long)biAbsLinearCoeff[3] * rowPrimeTrialDivisionData->exp3 +
            (long)biAbsLinearCoeff[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 4:
          Rem = (long)biAbsLinearCoeff[3] * rowPrimeTrialDivisionData->exp3 +
            (long)biAbsLinearCoeff[2] * rowPrimeTrialDivisionData->exp2;
          break;
        case 3:
          Rem = (long)biAbsLinearCoeff[2] * rowPrimeTrialDivisionData->exp2;
          break;
        default:
          Rem = 0;
          break;
        }
        RemB = (Rem + ((long)biAbsLinearCoeff[1] << 31) + biAbsLinearCoeff[0]) %
          currentPrime;
        if (positive)
        {
          RemB = currentPrime - RemB;
        }
        rowPrimeSieveData->soln1 = (int)((SieveLimit + (long)inverseA *
          (rowPrimeSieveData0->modsqrt + RemB)) % currentPrime);
      }
      do
      {                       // For each polynomial...
        if (congruencesFound >= matrixBLength /*|| factorSiqs != null*/)
        {
          if (nbrThreadFinishedPolySet > numberThreads*polySet)
          {
            break;
          }
          //synchronized(amodq)
          {
            nbrThreadFinishedPolySet++;
          }
          return;             // Another thread finished factoring.
        }
        if (onlyFactoring)
        {
          polynomialsSieved += 2;
        }
        /***************/
        /* Sieve stage */
        /***************/
        PerformSiqsSieveStage(primeSieveData, SieveArray, nbrPrimes,
          nbrPrimes2,
          firstLimit, secondLimit, thirdLimit,
          smallPrimeUpperLimit, threshold, multiplier,
          amodq, PolynomialIndex,
          nbrFactorsA, biLinearCoeff,
          NumberLength);
        ValuesSieved += 2 * SieveLimit;
        /************************/
        /* Trial division stage */
        /************************/
        index2 = 2 * SieveLimit - 1;
        do
        {
          if (((SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--] |
            SieveArray[index2--] | SieveArray[index2--]) & 0x8080) != 0)
          {
            for (i = 16; i>0; i--)
            {
              if ((SieveArray[index2 + i] & 0x80) != 0)
              {
                if (congruencesFound >= matrixBLength)
                {       // All congruences were found: stop sieving.
                  index2 = 0;
                  break;
                }
                SieveLocationHit(rowMatrixB,
                  rowMatrixBbeforeMerge,
                  vectExpParity,
                  index2 + i, primeSieveData,
                  primeTrialDivisionData, startTime,
                  nbrFactorsA, rowPartials,
                  multiplier, rowSquares,
                  biDividend, NumberLength, biT,
                  biLinearCoeff, biR, biU, biV,
                  indexFactorsA, FALSE);
                if (congruencesFound >= matrixBLength)
                {               // All congruences were found: stop sieving.
                  index2 = 0;
                  break;
                }
              }
              if (SieveArray[index2 + i] < 0)
              {
                if (congruencesFound >= matrixBLength)
                {       // All congruences were found: stop sieving.
                  index2 = 0;
                  break;
                }
                SieveLocationHit(rowMatrixB,
                  rowMatrixBbeforeMerge,
                  vectExpParity,
                  index2 + i, primeSieveData,
                  primeTrialDivisionData, startTime,
                  nbrFactorsA, rowPartials,
                  multiplier, rowSquares,
                  biDividend, NumberLength, biT,
                  biLinearCoeff, biR, biU, biV,
                  indexFactorsA, TRUE);
                if (congruencesFound >= matrixBLength)
                {               // All congruences were found: stop sieving.
                  index2 = 0;
                  break;
                }
              }
            }
          }
        } while (index2 > 0);
        /*******************/
        /* Next polynomial */
        /*******************/
        PolynomialIndex += 2;
      } while (PolynomialIndex <= lastPolynomial &&
        congruencesFound < matrixBLength);
    }
#if 0
  }
  catch (ArithmeticException ae)
  {
    synchronized(matrixB)
    {
      factorSiqs = null;
      matrixB.notify();
    }
  }
#endif
}
#if 0
/* Implementation of algorithm explained in Gower and Wagstaff paper */
/* The variables with suffix 3 correspond to multiplier = 3 */
static int SQUFOF(long N, int queue[])
{
  double sqrt;
  int Q, Q1, P, P1, L, S;
  int i, j, r, s, t, q;
  int queueHead, queueTail, queueIndex;
  long N3;
  int Q3, Q13, P3, P13, L3, S3;
  int r3, s3, t3, q3;
  int queueHead3, queueTail3, queueIndex3;
  int QRev, Q1Rev, PRev, P1Rev;
  int tRev, qRev, uRev;
  /* Step 1: Initialize */
  N3 = 3 * N;
  if ((N & 3) == 1)
  {
    N <<= 1;
  }
  if ((N3 & 3) == 1)
  {
    N3 <<= 1;
  }
  sqrt = sqrt(N);
  S = (int)sqrt;
  if ((long)(S + 1)*(long)(S + 1) <= N)
  {
    S++;
  }
  if ((long)S*(long)S > N)
  {
    S--;
  }
  if ((long)S*(long)S == N)
  {
    return S;
  }
  Q1 = 1;
  P = S;
  Q = (int)N - P*P;
  L = (int)(2 * sqrt(2 * sqrt));
  queueHead = 0;
  queueTail = 0;

  sqrt = sqrt(N3);
  S3 = (int)sqrt;
  if ((long)(S3 + 1)*(long)(S3 + 1) <= N3)
  {
    S3++;
  }
  if ((long)S3*(long)S3 > N3)
  {
    S3--;
  }
  if ((long)S3*(long)S3 == N3)
  {
    return S3;
  }
  Q13 = 1;
  P3 = S3;
  Q3 = (int)N3 - P3*P3;
  L3 = (int)(2 * sqrt(2 * sqrt));
  queueHead3 = 100;
  queueTail3 = 100;

  /* Step 2: Cycle forward to find a proper square form */
  for (i = 0; i <= L; i++)
  {
    /* Multiplier == 1 */
    q = (S + P) / Q;
    P1 = q*Q - P;
    if (Q <= L)
    {
      if ((Q & 1) == 0)
      {
        queue[queueHead++] = Q >> 1;
        queue[queueHead++] = P % (Q >> 1);
        if (queueHead == 100)
        {
          queueHead = 0;
        }
      }
      else if (Q + Q <= L)
      {
        queue[queueHead++] = Q;
        queue[queueHead++] = P % Q;
        if (queueHead == 100)
        {
          queueHead = 0;
        }
      }
    }
    t = Q1 + q*(P - P1);
    Q1 = Q;
    Q = t;
    P = P1;
    {
      r = (int)sqrt(Q);
      if (r*r == Q)
      {
        queueIndex = queueTail;
        for (;;)
        {
          if (queueIndex == queueHead)
          {
            /* Step 3: Compute inverse square root of the square form */
            PRev = P;
            Q1Rev = r;
            uRev = (S - PRev) % r;
            uRev += (uRev >> 31) & r;
            PRev = S - uRev;
            QRev = (int)((N - (long)PRev*(long)PRev) / Q1Rev);
            /* Step 4: Cycle in the reverse direction to find a factor of N */
            for (j = i; j >= 0; j--)
            {
              qRev = (S + PRev) / QRev;
              P1Rev = qRev*QRev - PRev;
              if (PRev == P1Rev)
              {
                /* Step 5: Get the factor of N */
                if ((QRev & 1) == 0)
                {
                  return QRev >> 1;
                }
                return QRev;
              }
              tRev = Q1Rev + qRev*(PRev - P1Rev);
              Q1Rev = QRev;
              QRev = tRev;
              PRev = P1Rev;
              qRev = (S + PRev) / QRev;
              P1Rev = qRev*QRev - PRev;
              if (PRev == P1Rev)
              {
                /* Step 5: Get the factor of N */
                if ((QRev & 1) == 0)
                {
                  return QRev >> 1;
                }
                return QRev;
              }
              tRev = Q1Rev + qRev*(PRev - P1Rev);
              Q1Rev = QRev;
              QRev = tRev;
              PRev = P1Rev;
            }
            break;
          }
          s = queue[queueIndex++];
          t = queue[queueIndex++];
          if (queueIndex == 100)
          {
            queueIndex = 0;
          }
          if ((P - t) % s == 0)
          {
            break;
          }
        }
        if (r > 1)
        {
          queueTail = queueIndex;
        }
        if (r == 1)
        {
          queueIndex = queueTail;
          for (;;)
          {
            if (queueIndex == queueHead)
            {
              break;
            }
            if (queue[queueIndex] == 1)
            {
              return 0;
            }
            queueIndex += 2;
            if (queueIndex == 100)
            {
              queueIndex = 0;
            }
          }
        }
      }
    }
    q = (S + P) / Q;
    P1 = q*Q - P;
    if (Q <= L)
    {
      if ((Q & 1) == 0)
      {
        queue[queueHead++] = Q >> 1;
        queue[queueHead++] = P % (Q >> 1);
        if (queueHead == 100)
        {
          queueHead = 0;
        }
      }
      else if (Q + Q <= L)
      {
        queue[queueHead++] = Q;
        queue[queueHead++] = P % Q;
        if (queueHead == 100)
        {
          queueHead = 0;
        }
      }
    }
    t = Q1 + q*(P - P1);
    Q1 = Q;
    Q = t;
    P = P1;

    /* Multiplier == 3 */
    q3 = (S3 + P3) / Q3;
    P13 = q3*Q3 - P3;
    if (Q3 <= L3)
    {
      if ((Q3 & 1) == 0)
      {
        queue[queueHead3++] = Q3 >> 1;
        queue[queueHead3++] = P3 % (Q3 >> 1);
        if (queueHead3 == 200)
        {
          queueHead3 = 100;
        }
      }
      else if (Q3 + Q3 <= L3)
      {
        queue[queueHead3++] = Q3;
        queue[queueHead3++] = P3 % Q3;
        if (queueHead3 == 200)
        {
          queueHead3 = 100;
        }
      }
    }
    t3 = Q13 + q3*(P3 - P13);
    Q13 = Q3;
    Q3 = t3;
    P3 = P13;
    {
      r3 = (int)sqrt(Q3);
      if (r3*r3 == Q3)
      {
        queueIndex3 = queueTail3;
        for (;;)
        {
          if (queueIndex3 == queueHead3)
          {
            /* Step 3: Compute inverse square root of the square form */
            PRev = P3;
            Q1Rev = r3;
            uRev = (S3 - PRev) % r3;
            uRev += (uRev >> 31) & r3;
            PRev = S3 - uRev;
            QRev = (int)((N3 - (long)PRev*(long)PRev) / Q1Rev);
            /* Step 4: Cycle in the reverse direction to find a factor of N */
            for (j = i; j >= 0; j--)
            {
              qRev = (S3 + PRev) / QRev;
              P1Rev = qRev*QRev - PRev;
              if (PRev == P1Rev)
              {
                /* Step 5: Get the factor of N */
                if ((QRev & 1) == 0)
                {
                  return QRev >> 1;
                }
                return QRev;
              }
              tRev = Q1Rev + qRev*(PRev - P1Rev);
              Q1Rev = QRev;
              QRev = tRev;
              PRev = P1Rev;
              qRev = (S3 + PRev) / QRev;
              P1Rev = qRev*QRev - PRev;
              if (PRev == P1Rev)
              {
                /* Step 5: Get the factor of N */
                if ((QRev & 1) == 0)
                {
                  return QRev >> 1;
                }
                return QRev;
              }
              tRev = Q1Rev + qRev*(PRev - P1Rev);
              Q1Rev = QRev;
              QRev = tRev;
              PRev = P1Rev;
            }
            break;
          }
          s3 = queue[queueIndex3++];
          t3 = queue[queueIndex3++];
          if (queueIndex3 == 200)
          {
            queueIndex3 = 100;
          }
          if ((P3 - t3) % s3 == 0)
          {
            break;
          }
        }
        if (r3 > 1)
        {
          queueTail3 = queueIndex3;
        }
        if (r3 == 1)
        {
          queueIndex3 = queueTail3;
          for (;;)
          {
            if (queueIndex3 == queueHead3)
            {
              break;
            }
            if (queue[queueIndex3] == 1)
            {
              return 0;
            }
            queueIndex3 += 2;
            if (queueIndex3 == 200)
            {
              queueIndex3 = 100;
            }
          }
        }
      }
    }
    q3 = (S3 + P3) / Q3;
    P13 = q3*Q3 - P3;
    if (Q3 <= L3)
    {
      if ((Q3 & 1) == 0)
      {
        queue[queueHead3++] = Q3 >> 1;
        queue[queueHead3++] = P3 % (Q3 >> 1);
        if (queueHead3 == 200)
        {
          queueHead3 = 100;
        }
      }
      else if (Q3 + Q3 <= L3)
      {
        queue[queueHead3++] = Q3;
        queue[queueHead3++] = P3 % Q3;
        if (queueHead3 == 200)
        {
          queueHead3 = 100;
        }
      }
    }
    t3 = Q13 + q3*(P3 - P13);
    Q13 = Q3;
    Q3 = t3;
    P3 = P13;
  }
  return 0;
}
#endif
/* If 2^value mod value = 2, then the value is a probable prime (value odd) */
static unsigned char isProbablePrime(long value)
{
  long mask, montgomery2, Pr, Prod0, Prod1, MontDig;
  int N, MontgomeryMultN;
  long BaseHI, BaseLO, valueHI, valueLO;
  int x = N = (int)value; // 2 least significant bits of inverse correct.
  x = x * (2 - N * x);     // 4 least significant bits of inverse correct.
  x = x * (2 - N * x);     // 8 least significant bits of inverse correct.
  x = x * (2 - N * x);     // 16 least significant bits of inverse correct.
  x = x * (2 - N * x);     // 32 least significant bits of inverse correct.
  MontgomeryMultN = (-x) & 0x7FFFFFFF;
  mask = 1L << 62;
  montgomery2 = 2 * (mask % value);
  if (montgomery2 >= value)
  {
    montgomery2 -= value;
  }
  BaseHI = (int)(montgomery2 >> 31);
  BaseLO = (int)montgomery2 & 0x7FFFFFFF;
  valueHI = (int)(value >> 31);
  valueLO = (int)value & 0x7FFFFFFF;
  while ((mask & value) == 0)
  {
    mask >>= 1;
  }
  mask >>= 1;
  while (mask > 0)
  {
    /* Square the base */
    Pr = BaseLO * BaseLO;
    MontDig = ((int)Pr * MontgomeryMultN) & 0x7FFFFFFFL;
    Prod0 = (Pr = ((MontDig * valueLO + Pr) >> 31) +
      MontDig * valueHI + BaseLO * BaseHI) & 0x7FFFFFFFL;
    Prod1 = Pr >> 31;
    Pr = BaseHI * BaseLO + Prod0;
    MontDig = ((int)Pr * MontgomeryMultN) & 0x7FFFFFFFL;
    Prod0 = (Pr = ((MontDig * valueLO + Pr) >> 31) +
      MontDig * valueHI + BaseHI * BaseHI + Prod1) & 0x7FFFFFFFL;
    Prod1 = Pr >> 31;
    if (Prod1 > valueHI || (Prod1 == valueHI && Prod0 >= valueLO))
    {
      Prod0 = (Pr = Prod0 - valueLO) & 0x7FFFFFFFL;
      Prod1 = ((Pr >> 31) + Prod1 - valueHI) & 0x7FFFFFFFL;
    }
    BaseLO = Prod0;
    BaseHI = Prod1;

    if ((mask & value) != 0)
    {
      /* Multiply by 2 */
      Pr = 2 * ((BaseHI << 31) + BaseLO);
      if (Pr >= value)
      {
        Pr -= value;
      }
      BaseHI = (int)(Pr >> 31);
      BaseLO = (int)Pr & 0x7FFFFFFF;
    }
    mask >>= 1;
  }
  Pr = (BaseHI << 31) + BaseLO;
  return Pr == montgomery2;
}

