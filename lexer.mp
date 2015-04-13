/* from the ACM TOSEM paper draft 
 runs for scope 1 and 2 (~16 sec), 
 for scope 3 it takes much longer.
 
 The iteration limits <n..m> should be used to curb 
 traveling too far into the combinatorial ocean.
  
 When ENSURE construct will be added to the Text_input
 constraining the numbers of Get_char and Unget_char further,
 the number of traces will drop significantly, 
 and hopefully will become affordable for scope 3.
 Meanwhile, this example demonstrates that we should be 
 carefull with selecting limits for iterations, 
 to avoid going too far into combinatorial exploration without real need.
*/

SCHEMA lexer

ROOT Text_Input:  	(*<1..2> String_processing *);
	String_processing:	Get_string (+<1..2> Unget_char +);
	Get_string:			(+<2..3> Get_char +);

ROOT Token_processing: 	(*<1..2> Token_recognition *);
	Token_recognition: 	{+ RegExpr_Match +}
				(+<1..2> Unget_char +)  Fire_rule;
 	RegExpr_Match:		(+<2..3> Get_char +);
 	Fire_rule: 		[ Put_token ];

COORDINATE 	$t: Token_recognition 	FROM 	Token_processing,
			$s: String_processing	FROM 	Text_Input
	DO	
		$t, $s SHARE ALL Unget_char;
		
		COORDINATE <!>    $r: RegExpr_Match      FROM  	$t
			DO 	$r,  $s 	SHARE ALL    Get_char ; 	OD;
	OD;
