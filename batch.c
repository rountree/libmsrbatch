#include "msr_safe.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#define max_ops (10000)

int fd, rc;

struct msr{
    uint32_t address;
    uint64_t writemask;
};

static struct msr *msr_list = NULL;
static bool msr_list_initialized = false;

static uint32_t msr_count;
#define MSR_BUF_SIZE (1023) // Maximum length of a msr_allowed_list entry.

int add_ops( struct msr_batch_array *batch, uint16_t firstcpu, uint16_t lastcpu, bool isrdmsr, uint32_t msr, uint64_t msrdata, uint64_t wmask ){
    int i,j;

int add_readops(struct msr_batch_array *batch, __u16 firstcpu, __u16 lastcpu, __u32 msr){
    uint32_t prev_numops = batch->numops;
    if(firstcpu > lastcpu){
        printf(" first cpu should be < last cpu.");
        exit(-1);
    }
    batch->numops = batch->numops+(lastcpu-firstcpu)+1;
    batch->ops = realloc( batch->ops, sizeof(struct msr_batch_op) * batch->numops );
        for(j=prev_numops, i = firstcpu; i <= lastcpu; j++,i++){
            batch->ops[j].cpu = i;
            batch->ops[j].isrdmsr = 1;
            batch->ops[j].err = 0;
            batch->ops[j].msr = msr;
            batch->ops[j].msrdata = 0;
            batch->ops[j].wmask = 0;

    }
#if DEBUG
	if(batch->ops[i].err == -13){
		perror("MSR permission error check msr_approved_list to make sure MSR exists and is readable");
		exit(-1);
	}
#endif
    return 0;
}

void parse_msr_approved_list(){
	FILE *fp;
	int rc;
	char buf[MSR_BUF_SIZE];
	fp = fopen("/dev/cpu/msr_approved_list", "r");

	if(fp == NULL){
        printf("File could not be opened check that you have read permissions and that the file exists");
		exit(-1);
	}

	while(fgets(buf, MSR_BUF_SIZE, fp) != NULL){
		if(buf[0] == '#'){
			continue;
		}
		msr_list = realloc( msr_list, sizeof( struct msr ) * ++msr_count );
       		rc = sscanf( buf, "0x%" PRIx32 " 0x%" PRIx64 "\n", &(msr_list[msr_count-1].address), &(msr_list[msr_count-1].writemask) );
        	if(rc < 2){
           		fprintf( stderr, "Ooops, don't know how to parse '%s'.\n", buf );
            		exit(-1);
        	}
    	}

	if(msr_count == 0){
		fprintf(stderr, "msr_approved is readable but empty %s \n", buf);
		exit(-1);
	}

	assert(EOF != fclose(fp));
}

void print_approved_list(){
	int i;
    if( !msr_list_initialized ){
        parse_msr_approved_list();
        msr_list_initialized = true;
    }
	for( i=0; i<msr_count; i++ ){
        	fprintf( stdout, "0x%08" PRIx32 " 0x%016" PRIx64 "\n", msr_list[i].address, msr_list[i].writemask );
	}
}

bool msr_read_check(uint32_t address){

    if( !msr_list_initialized ){
        parse_msr_approved_list();
        msr_list_initialized = true;
    }

	for( int i=0; i<msr_count; i++ ){
        	if( address == msr_list[i].address ){
         		return true;
        	}
    	}
	return false;
}

bool msr_write_check(uint32_t address, uint64_t value){

    if( !msr_list_initialized ){
        parse_msr_approved_list();
        msr_list_initialized = true;
    }
	for( int i=0; i<msr_count; i++ ){
        if( address == msr_list[i].address ){
            if( (msr_list[i].writemask | value ) == msr_list[i].writemask ){
                     return true;
            }else{
                    return false;
            }
        }
	}
    return false;
}

int add_writeops(struct msr_batch_array *batch, __u16 first_cpu, __u16 last_cpu, __u32 msr, __u64 writemask){
	int i;

	if(first_cpu >= last_cpu){
		printf("in (firstcpu, lastcpu) first cpu should be < last cpu");
		exit(-1);
	}

	batch->numops = batch->numops+(last_cpu-first_cpu)+1;
	batch->ops = realloc( batch->ops, sizeof(struct msr_batch_op) * batch->numops );

	for(i = first_cpu; i <= last_cpu; i++){

		batch->ops[i].cpu = i;
        	batch->ops[i].isrdmsr = 1;
        	batch->ops[i].err = 0;
        	batch->ops[i].msr = msr;
        	batch->ops[i].msrdata = 0;
        	batch->ops[i].wmask = writemask;

    		printf("MSR Add: %" PRIx32 " MSR value: %llu"  " CPU core: %" PRIu16 "\n",
    			batch->ops[i].msr,
        		batch->ops[i].msrdata,
        		batch->ops[i].cpu);
    	}

	if(batch->ops[i].err == -13){
		perror("MSR permission error check msr_approved_list to make sure MSR is writable");
		exit(-1);
	}

	return 0;

}

#if DEBUG
int print_op( struct msr_batch_op *op ){
	printf("cpu: %" PRIu16 "  isrdmsr: %" PRIu16  " err: %" PRId32 "  msraddr: %" PRIx32 "  msrdata: %" PRIu64  "   wmask: %" PRIx64 " \n",
		(uint16_t)op->cpu,
		(uint16_t)op->isrdmsr,
		(int32_t)op->err,
		(uint32_t)op->msr,
		(uint64_t)op->msrdata,
		(uint64_t)op->wmask);

	return 0;
}
#endif 
#if DEBUG
extern int print_batch( struct msr_batch_array *batch ){
	int i;
	printf("Batch %p has numops=%" PRIu32 "\n", batch, (uint32_t)batch->numops);

	for(i=0; i < batch->numops; i++){
        print_op(&(batch->ops[i]));
    }

	return 0;
}
#endif
int run_batch( struct msr_batch_array *batch ){
	fd = open("/dev/cpu/msr_batch", O_RDWR);

	if(fd == -1){
		perror("err cannot access directory make sure permissions are set!");
            exit(-1);
	}

	if(fd == -13){
		perror("error file or directory may not exist or be accessible");
		exit(-1);
	}
#if DEBUG
    print_batch( batch );
#endif
	rc = ioctl(fd, X86_IOC_MSR_BATCH, batch);
#if DEBUG
    print_batch( batch );
#endif
	if(rc == -13){
		perror("ioctl failed");
		fprintf(stderr,"%s::%d rc=%d \n Check that msr_safe kernel module is loaded and that the approved list is populated", __FILE__, __LINE__, rc);
	}

	if(rc < 0){
		rc = rc * -1;
		printf("ioctl failed, check that the MSR exists, and is read/writable \n");
		fprintf(stderr, "%s::%d rc=%d\n", __FILE__, __LINE__, rc);
        exit(-1);
    }
	return 0;
}

