/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2008 Ignacio Castano                      castano@gmail.com

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */

#include <stdio.h>
#include <float.h>
#include <math.h>

struct Precomp {
	float alpha2_sum;
	float beta2_sum;
	float alphabeta_sum;
	float factor;
};


int main()
{
	int i = 0;
	
	printf("struct Precomp {\n");
	printf("\tfloat alpha2_sum;\n");
	printf("\tfloat beta2_sum;\n");
	printf("\tfloat alphabeta_sum;\n");
	printf("\tfloat factor;\n");
	printf("};\n\n");

	printf("static const SQUISH_ALIGN_16 Precomp s_threeElement[153] = {\n");
	
	// Three element clusters:
	for( int c0 = 0; c0 <= 16; c0++)	// At least two clusters.
	{
		for( int c1 = 0; c1 <=  16-c0; c1++)
		{
			int c2 = 16 - c0 - c1;

			Precomp p;
			p.alpha2_sum = c0 + c1 * 0.25f;
			p.beta2_sum = c2 + c1 * 0.25f;
			p.alphabeta_sum = c1 * 0.25f;
			p.factor = 1.0f / (p.alpha2_sum * p.beta2_sum - p.alphabeta_sum * p.alphabeta_sum);

			if (isfinite(p.factor))
			{
				printf("\t{ %ff, %ff, %ff, %ff }, // %d (%d %d %d)\n", p.alpha2_sum, p.beta2_sum, p.alphabeta_sum, p.factor, i, c0, c1, c2);
			}
			else
			{
				printf("\t{ %ff, %ff, %ff, FLT_MAX }, // %d (%d %d %d)\n", p.alpha2_sum, p.beta2_sum, p.alphabeta_sum, i, c0, c1, c2);
			}
			
			i++;
		}
	}
	printf("}; // %d three cluster elements\n\n", i);
	
	printf("static const SQUISH_ALIGN_16 Precomp s_fourElement[969] = {\n");

	// Four element clusters:
	i = 0;
	for( int c0 = 0; c0 <= 16; c0++)
	{
		for( int c1 = 0; c1 <=  16-c0; c1++)
		{
			for( int c2 = 0; c2 <=  16-c0-c1; c2++)
			{
				int c3 = 16 - c0 - c1 - c2;

				Precomp p;			
				p.alpha2_sum = c0 + c1 * (4.0f/9.0f) + c2 * (1.0f/9.0f);
				p.beta2_sum = c3 + c2 * (4.0f/9.0f) + c1 * (1.0f/9.0f);
				p.alphabeta_sum = (c1 + c2) * (2.0f/9.0f);
				p.factor = 1.0f / (p.alpha2_sum * p.beta2_sum - p.alphabeta_sum * p.alphabeta_sum);

				if (isfinite(p.factor))
				{
					printf("\t{ %ff, %ff, %ff, %ff }, // %d (%d %d %d %d)\n", p.alpha2_sum, p.beta2_sum, p.alphabeta_sum, p.factor, i, c0, c1, c2, c3);
				}
				else
				{
					printf("\t{ %ff, %ff, %ff, FLT_MAX }, // %d (%d %d %d %d)\n", p.alpha2_sum, p.beta2_sum, p.alphabeta_sum, i, c0, c1, c2, c3);
				}

				i++;
			}
		}
	}
	printf("}; // %d four cluster elements\n\n", i);

	return 0;
}
