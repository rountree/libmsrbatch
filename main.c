#include "msr_safe.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "batch.h"

int main(){

	struct msr_batch_array my_batch;
	my_batch.numops = 0;
	my_batch.ops = NULL;
    fprintf( stdout, "Is msr  0x611 readable?  %s\n", msr_read_check( 0x611 ) ? "true!" : "false!" );
    fprintf( stdout, "Can 0xffffff  be written to 0xE8?  %s\n", msr_write_check( 0xE8, 0xffffff ) ? "true!" : "false!" );
    fprintf( stdout, "Can 12 be written to 0x186?  %s\n", msr_write_check( 0x186, 12 ) ? "true!" : "false!" );
    add_readops( &my_batch, 0, 8, 0x611);
    run_batch(&my_batch);
	return 0;
}
