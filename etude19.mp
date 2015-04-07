/* basic test for SHARE ALL 
	3/22/15
*/

SCHEMA etude19

ROOT A: a b;
ROOT B: b a;

A, B SHARE ALL a, b;
	