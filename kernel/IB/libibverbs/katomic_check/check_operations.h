#include <linux/types.h>
#include <vl.h>

#ifndef __CHECK_OPERATIONS__
#define __CHECK_OPERATIONS__
/* macros to handle masks */
//#define VL_MASK_IS_SET(mask, attr)      (!!((mask)&(attr)))

#define ADD_SIZE 64

/* calculate the expected value in the remote memory in masked Cmp & Swap */
inline uint64_t calc_msk_cmp_and_swap(
			uint64_t compare_value,
			uint64_t swap_value,
			uint64_t cmp_mask,
			uint64_t swp_mask,
			uint64_t orig_value)
{
	uint64_t value;

	if (!((compare_value ^ orig_value) & cmp_mask))
		value = (orig_value & ~(swp_mask)) | (swap_value & swp_mask);
	else
		value = orig_value;

	return value;
}

static inline int bit_adder(int ci, int b1, int b2, int *co)
{
	int value = ci + b1 + b2;
	
	*co = !!(value & 2);

	return value & 1;
}

/* calculate the expected value in the remote memory in masked Fetch & Add */
inline uint64_t calc_msk_fetch_and_add(
			uint64_t add_value,
			uint64_t mask_value,
			uint64_t orig_value)
{
	uint64_t value = 0;
	uint64_t mask = 1;
	int carry = 0, new_carry, bit_add_res;
	int i;

	for (i = 0; i < ADD_SIZE; i ++, mask =  mask << 1) {

		bit_add_res = bit_adder(carry, VL_MASK_IS_SET(orig_value, mask), VL_MASK_IS_SET(add_value, mask), &new_carry);
		if (bit_add_res)
			value |= mask;

		carry = ((new_carry) && (!VL_MASK_IS_SET(mask_value, mask)));
	}

	return value;
}

#endif
