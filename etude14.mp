/* basic test for COORDINATE 
	3/11/15
*/

SCHEMA etude14

ROOT Sender:   (* send *);
ROOT Receiver: (* receive *);

/* COORDINATE	$x: send FROM Sender, 
				$y: receive FROM Receiver
	DO ADD $x PRECEDES $y OD;
*/
