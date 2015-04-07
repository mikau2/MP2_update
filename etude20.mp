/* basic test for SHARE ALL 
	3/25/15
*/

SCHEMA etude20

ROOT A: (* a b *);
ROOT B: (* (b|a) (a|b) *);

A, B SHARE ALL a, b;

ROOT C: (* a *);

A, B, C SHARE ALL a;