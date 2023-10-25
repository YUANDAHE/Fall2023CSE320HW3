#include <stdio.h>
#include "sfmm.h"
#include "mysfmm.h"
void my_show_freelists()
{
    for(int i = 0; i < NUM_FREE_LISTS; i++){
        printf("sf_free_list_heads[%d]:  (%p)\n",i,  &sf_free_list_heads[i]);
        sf_block *tmp = sf_free_list_heads[i].body.links.next;
        while(tmp && tmp != &sf_free_list_heads[i]){
            sf_block *bp = sf_free_list_heads[i].body.links.next;
            printf("bp = %p, prv_bp = %p, header = 0x%016lx, footer = 0x%016lx\n", bp, sf_free_list_heads[i].body.links.prev, sf_get(sf_hdrp(bp)), sf_get(sf_ftrp(bp)));
            tmp = tmp->body.links.next;
        }
    }
}
int main(int argc, char const *argv[]) {

    sf_errno = 0;
	/* void *x = */ sf_malloc(8);
    my_show_freelists();
	void *y = sf_malloc(200);
    my_show_freelists();
	/* void *z = */ sf_malloc(1);
    my_show_freelists();

	sf_free(y);
    my_show_freelists();
    printf("alloc end\n");
    printf("start = %p, end = %p\n", sf_mem_start(), sf_mem_end());
    printf("sizeof = %ld\n", sizeof(sf_block));
    printf("show_heap:\n");
    //sf_show_heap();
    printf("show_heap end\n");
    printf("show_blocks:\n");
    sf_show_blocks();
    printf("\n");
    printf("show_block 8:\n");
    sf_show_block(&sf_free_list_heads[8]);
    printf("\n");
    sf_show_free_list(4);
    
    //sf_show_heap();
    
    

    return EXIT_SUCCESS;
}
