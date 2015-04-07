/* basic test for SHARE ALL 
	3/22/15
*/
SCHEMA etude17

ROOT A: a1 (*<1..2> a2  *) a3 C;
ROOT B: b1 (*<1..2> a2  *) b3 C;

C: (c1 | c2);

A, B SHARE ALL a2, C;

COORDINATE $x: a1 FROM A, $y: b1 FROM B
	DO ADD $x PRECEDES $y; OD;
	
ROOT D: (*<1..2> (d|e) *);
COORDINATE $v: a2 FROM A, $w: (d|e) FROM D
		DO ADD $w FOLLOWS $v; OD;
	
