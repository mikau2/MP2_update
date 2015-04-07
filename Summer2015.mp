/*//————————————————————————————
// 	 		  Actors
//————————————————————————————*/
SCHEMA Summer2015

ROOT You:	Read_Flyer
                Get_Intrigued 
                Check_Out_Website
                ( Like_MP | Decide_MP_Not_for_Me );
     
       Like_MP: Email_Dr_G
                Discuss_Research_Interests
                Propose_a_Fascinating_Problem_to_Model
                Do_an_Awesome_Senior_Project;

Do_an_Awesome_Senior_Project: (* ( accomplish_a_task | got_a_great_idea ) 
				 [ discuss_some_questions ] *);
                
ROOT Dr_G:  ( 	Set_Up_Meeting
                Discuss_Research_Interests
                Assess_MP_as_a_Good_Fit
                Mentor_an_Awesome_Senior_Project  |
	        Does_something_else );

Mentor_an_Awesome_Senior_Project: (* discuss_some_questions *);

/*//————————————————————————————
// 		        Interactions
//————————————————————————————*/

COORDINATE 	$a: Email_Dr_G			FROM You, 
		$b: Set_Up_Meeting		FROM Dr_G
		DO ADD $a PRECEDES $b; OD;

COORDINATE 	$a: Propose_a_Fascinating_Problem_to_Model  FROM You, 
		$b: Assess_MP_as_a_Good_Fit		    FROM Dr_G
		DO ADD $a PRECEDES $b; OD;

COORDINATE 	$a: Assess_MP_as_a_Good_Fit  FROM Dr_G, 
		$b: Do_an_Awesome_Senior_Project  FROM You
		DO ADD $a PRECEDES $b; OD;

You, Dr_G SHARE ALL discuss_some_questions;