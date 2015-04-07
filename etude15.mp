/* advanced test for COORDINATE 
	3/20/15
*/

SCHEMA etude15

ROOT Sender:   (* send [ check ] *);
ROOT Receiver: (* receive confirm *);

COORDINATE	$ch: check FROM Sender, 
			$con: confirm FROM Receiver
	DO ADD $ch FOLLOWS $con; OD;
	
ROOT Monitor: {* watching *};
 watching: ( happy | angry );

/* receive happens only when Monitor is happy */
/* variant 1, example with three coordination threads */
COORDINATE <!>	$s: send FROM Sender, 
				$h: happy FROM Monitor,
				$r2: receive FROM Receiver
	DO ADD $h FOLLOWS $s, $r2 FOLLOWS $h; OD;
