/* basic test for SHARE ALL 
	3/22/15
*/

SCHEMA etude18

ROOT A:  (* a2  *) ;
ROOT B: (* a2 *) (* b3 *);

A, B SHARE ALL a2;
	
ROOT D: (* b3 *);

B, D SHARE ALL b3;
