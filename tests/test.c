#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfish.h"
#include <errno.h>


// Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
//   void *pointer = sf_malloc(sizeof(short));
//   sf_free(pointer);
//   pointer = (char*)pointer - 8;
//   sf_header *sfHeader = (sf_header *) pointer;
//   cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
//   sf_footer *sfFooter = (sf_footer *) ((char*)pointer - 8 + (sfHeader->block_size << 4));
//   cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
// }