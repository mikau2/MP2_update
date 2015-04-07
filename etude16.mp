/* basic test for COORDINATE 
	3/11/15
*/

SCHEMA etude16

ROOT Sender:   (* send  *);
ROOT Receiver: (* receive  *);

COORDINATE	$s: send FROM Sender, 
			$r: receive FROM Receiver
	DO ADD $r FOLLOWS $s; OD;
	
