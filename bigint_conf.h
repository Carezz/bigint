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

/*
  Constant time flag.

  Note: While certain operations are defined to be constant time (the MIN and MAX macro or the 
        conditional swap for example), the rest of the operations are not.
		As such this flag will attempt to force constant time operations at most places.

  WARNING: AS OF CURRENT, THIS LIBRARY IMPLEMENTS CONSTANT-TIME OPERATIONS RELATED TO
  BRANCHING. IT DOES NOT HOWEVER TAKE CARE OF CERTAIN CACHE TIMING ATTACKS (VARIABLE-ACCESS PATTERNS).
  PLEASE BE ADVISED THIS LIBRARY IS STILL IN ALPHA STAGE AND AS SUCH SHOULD NOT BE USED FOR PRODUCTIVE
  USAGE OF ANY KIND.

  0 - Off
  1 - On
*/
#define BIGINT_CONSTANT_TIME 0
