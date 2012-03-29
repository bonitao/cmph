/*		 
 * Sux: Succinct data structures
 *
 * Copyright (C) 2007-2011 Sebastiano Vigna 
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef rank9_h
#define rank9_h
#include <stdint.h>
#include "macros.h"

class rank9 {
private:
	const uint64_t *bits;
	uint64_t *counts, *inventory;
	uint64_t num_words, num_counts, inventory_size, ones_per_inventory, log2_ones_per_inventory, num_ones;

	/** Counts the number of bits in x. */
	__inline static int count( const uint64_t x ) {
		register uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
		byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
		byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
		return byte_sums * ONES_STEP_8 >> 56;
	}

public:
	rank9();
	rank9( const uint64_t * const bits, const uint64_t num_bits );
        const uint64_t* scounts() { return counts; }
	~rank9();
	uint64_t rank( const uint64_t pos );
	static uint64_t static_rank( const uint64_t pos, const uint64_t* sbits, const uint64_t* counts );
	uint64_t select( const uint64_t rank ) { return 1; }
	// Just for analysis purposes
	void print_counts();
	uint64_t bit_count();
};

#endif
