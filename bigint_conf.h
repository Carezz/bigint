/*
 Bigint implementation by Carez.
*/

/*
 Set the type of allocation bigint would use.

 Static allocation = 0
 Dynamic allocation = 1
*/
#define BIGINT_ALLOC 1

/*
 In the case of dynamic allocation MAX_LIMBS is the maximum number of limbs it will grow
 automatically.

 In the case of static allocation MAX_LIMBS is the maximum number of limbs allocated 
 altogether.
*/

#define BIGINT_MAX_LIMBS 1000

