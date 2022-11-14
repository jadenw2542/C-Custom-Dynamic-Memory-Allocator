#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
   // void *x =  sf_malloc(4);
    int number[12]; /* 12 cells, one cell per student */
    int index, sum = 0;

    /* Always initialize array before use */
    for (index = 0; index < 12; index++) {
    number[index] = index;
    }

    /* Now, number[index]=index; Error: Why? What kind?*/
    for (index = 0; index < 12; index = index + 1) {
    sum += number[index]; /* Sum array elements */
    }
 


    void *a = sf_malloc(16);
    sf_malloc(200);
    sf_memalign(32, 256);
	 /*void *z = sf_malloc(60);*/

   printf(" \n \n \n %p ", a);

    

   //void * test = sf_memalign(32 , 256);


    printf("\n SHOW BLOCKS : \n");
    sf_show_blocks();
    printf("\n SHOW FREE LIST : \n");
    sf_show_free_lists();
    printf("\n SHOW HEAP : \n");
    sf_show_heap();
    /**
    double* ptr = sf_malloc(2000); // sizeof(double) 1952 = max bytes before adding another page

    *ptr = 320320320e-320;

    double* ptr1 = sf_malloc(2000); // sizeof(double) 1952 = max bytes before adding another page

    *ptr1 = 320320320e-320;

    double* ptr2 = sf_malloc(1900); // sizeof(double) 1952 = max bytes before adding another page

    *ptr2 = 320320320e-320;

    double* ptr3 = sf_malloc(sizeof(double)); // sizeof(double) 1952 = max bytes before adding another page

    *ptr3 = 320320320e-320;

    free(ptr3);

    printf("%f\n", *ptr);
    **/



    //sf_free(ptr);

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
