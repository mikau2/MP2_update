/* from the ACM TOSEM paper draft 
 runs for scope 1 and 2, for scope 3 takes too long
*/

SCHEMA lexer

ROOT Text_Input:  	(* String_processing *);
	String_processing:	Get_string (+ Unget_char +);
	Get_string:			(+<2..3> Get_char +);

ROOT Token_processing: 	(* Token_recognition *);
	Token_recognition: 	{+ RegExpr_Match +}
				(+ Unget_char +)  Fire_rule;
 	RegExpr_Match:		(+ Get_char +);
 	Fire_rule: 		[ Put_token ];

COORDINATE 	$t: Token_recognition 	FROM 	Token_processing,
			$s: String_processing	FROM 	Text_Input
	DO	
		$t, $s SHARE ALL Unget_char;
		
		COORDINATE <!>    $r: RegExpr_Match      FROM  	$t
			DO 	$r,  $s 	SHARE ALL    Get_char ; 	OD;
	OD;
