/*//————————————————————————————
// 	Kristin Giammarco
//	11 February 2015 - Initial Model Started by KGiammarco
//	12 February 2015 - Made Executable for Eagle6 KGiammarco
//      21 Feb 2015 - Interactions 12, 13, 14a, 14b, 15 causing execution issues,
//                      tried retyping Interaction 15 with no success
//      Based on "Swarm CONOPs" Draft 11 February 2015 
//---------------------------------------------------------

	notes:
1. COORDINATE should be applied to roots, hence
	I’ve replaced UAV (which is just a composite event) with SWARM
2. Swarm is a set (no ordering), this requires Asynchronous COORDINATE <!>

3. Heavy coordination implies that even for scope 2 there is only 1 trace. 
	Need to look carefully where in the model appears a coordination 
	with root that allows only a single event participating in the COORDINATE.
//---------------------------------------------------------
// 	 		Main Actors
//————————————————————————————*/
SCHEMA Swarm2

ROOT Flight_Crew: 	Conduct_PreMission_Briefing
			(* PreFlight_UAV *)
			Propose_Staging_Plans
			(* Request_UAV_Launch_Permission
			   Launch_UAV
			   Assess_Flight_Behavior
			   Confirm_Nominal_Flight_Behavior
			   Stage_UAV_for_Ingress *)

			   Alert_All_UAVs_Staged
			  (* Observe_New_UAV_in_Recovery_List
			     [ Receive_Recovery_Prioritization_List ]  
			     Recover_UAV 
			     Retrieve_UAV
			     Alert_RC_UAV_Landing *)
			   Confirm_UAVs_Recovered_Retrieved
			   Conduct_PostMission_Hotwash;


ROOT Swarm: 		{+ UAV +};

/* composite event */
   UAV:			Undergo_PreFlight
			Launch
			Report_Flight_Status
			Confirm_Staged_for_Ingress
			Confirm_Ingress_SubSwarm_ID
			Commence_Mission
			Confirm_Egress_SubSwarm_ID
			Confirm_Staged_for_Egress
			Land;

COORDINATE <!>	$a: PreFlight_UAV			FROM Flight_Crew, 
		$b: Undergo_PreFlight			FROM Swarm
		DO ADD $a PRECEDES $b; OD;  		/* 2 */

COORDINATE <!>	$a: Launch_UAV				FROM Flight_Crew, 
		$b: Launch 				FROM Swarm
		DO ADD $a PRECEDES $b; OD;		/* 6 */

COORDINATE <!>	$a: Report_Flight_Status 		FROM Swarm, 
		$b: Assess_Flight_Behavior		FROM Flight_Crew
		DO ADD $a PRECEDES $b; OD;		/* 7 */

COORDINATE <!>	$a: Stage_UAV_for_Ingress		FROM Flight_Crew, 
		$b: Confirm_Staged_for_Ingress		FROM Swarm
		DO ADD $a PRECEDES $b; OD;		/* 8a */

COORDINATE <!>	$a: Confirm_Staged_for_Egress 		FROM Swarm, 
		$b: Observe_New_UAV_in_Recovery_List	FROM Flight_Crew
		DO ADD $a PRECEDES $b; OD;		/* 12 */

COORDINATE <!>	$a: Recover_UAV				FROM Flight_Crew, 
		$b: Land 				FROM Swarm
		DO ADD $b PRECEDES $a; OD;		/* 14a */


ROOT Mission_Commander:	Conduct_PreMission_Briefing
                        Confirm_Staging_Plans
			Confirm_Swarm_Mission_Plan
			(* Grant_UAV_Launch_Permission *)
			Conduct_PostMission_Hotwash;

COORDINATE 	$a: Propose_Staging_Plans		FROM Flight_Crew, 
		$b: Confirm_Staging_Plans		FROM Mission_Commander
		DO ADD $a PRECEDES $b; OD;		/* 3 */

COORDINATE 	$a: Request_UAV_Launch_Permission	FROM Flight_Crew, 
		$b: Grant_UAV_Launch_Permission		FROM Mission_Commander
		DO ADD $a PRECEDES $b; OD;		/* 5 */

COORDINATE 	$a: Confirm_UAVs_Recovered_Retrieved	FROM Flight_Crew, 
		$b: Conduct_PostMission_Hotwash		FROM Mission_Commander
		DO ADD $a PRECEDES $b; OD;		/* 16a */


ROOT Swarm_Operator:	Conduct_PreMission_Briefing
			Propose_Swarm_Mission_Plan
			(* Assign_Ingress_SubSwarmID *)
			Command_Swarm_to_Commence_Mission
			(* Assign_Egress_SubSwarmID *)
			Provide_Recovery_Prioritization_List
			Conduct_PostMission_Hotwash;
			   
COORDINATE 	$a: Propose_Swarm_Mission_Plan		FROM Swarm_Operator, 
		$b: Confirm_Swarm_Mission_Plan		FROM Mission_Commander
		DO ADD $a PRECEDES $b; OD;		/* 4 */

COORDINATE <!>	$a: Confirm_Staged_for_Ingress		FROM Swarm, 
		$b: Assign_Ingress_SubSwarmID		FROM Swarm_Operator
		DO ADD $a PRECEDES $b; OD;		/* 8b */

COORDINATE <!>	$a: Assign_Ingress_SubSwarmID		FROM Swarm_Operator, 
		$b: Confirm_Ingress_SubSwarm_ID		FROM Swarm
		DO ADD $a PRECEDES $b; OD;		/* 9 */

COORDINATE 	$a: Alert_All_UAVs_Staged		FROM Flight_Crew,
		$b: Command_Swarm_to_Commence_Mission	FROM Swarm_Operator
		DO ADD $a PRECEDES $b; OD;		/* 10a */

COORDINATE <!>	$a: Command_Swarm_to_Commence_Mission	FROM Swarm_Operator, 
		$b: Commence_Mission			FROM Swarm
		DO ADD $a PRECEDES $b; OD;		/* 10b */

COORDINATE <!>	$a: Assign_Egress_SubSwarmID		FROM Swarm_Operator, 
		$b: Confirm_Egress_SubSwarm_ID		FROM Swarm
		DO ADD $a PRECEDES $b; OD;		/* 11 */

COORDINATE 	$a: Provide_Recovery_Prioritization_List	FROM Swarm_Operator, 
		$b: Receive_Recovery_Prioritization_List	FROM Flight_Crew
		DO ADD $a PRECEDES $b; OD;		/* 13 */

ROOT Range_Control: 	(* Receive_UAV_Landing_Notification *);

COORDINATE 	$a: Alert_RC_UAV_Landing		FROM Flight_Crew, 
		$b: Receive_UAV_Landing_Notification	FROM Range_Control
		DO ADD $a PRECEDES $b; OD;		/* 15 */

ROOT Safety_Coordinator:	Conduct_PreMission_Briefing
			        Conduct_PostMission_Hotwash;

/*
//---------------------------------------------------------
// 		         Interactions
//		SHARE ALL Simulating COORDINATE
//---------------------------------------------------------

Mission_Commander, Flight_Crew, Swarm_Operator, Safety_Coordinator SHARE ALL Conduct_PreMission_Briefing;
*/

/*
ROOT    Interaction2: (* (PreFlight_UAV   Undergo_PreFlight) *);
Flight_Crew,   Interaction2     SHARE ALL	PreFlight_UAV;
UAV, Interaction2       	SHARE ALL 	Undergo_PreFlight;


ROOT    Interaction3: (* (Propose_Staging_Plans   Confirm_Staging_Plans) *);
Flight_Crew,   Interaction3     	SHARE ALL	Propose_Staging_Plans;
Mission_Commander, Interaction3       	SHARE ALL 	Confirm_Staging_Plans;


//COORDINATE 	$a: Propose_Swarm_Mission_Plan		FROM Swarm_Operator, 
//		$b: Confirm_Swarm_Mission_Plan		FROM Mission_Commander
//		DO ADD $a PRECEDES $b OD;		//4

ROOT    Interaction4: (* (Propose_Swarm_Mission_Plan   Confirm_Swarm_Mission_Plan) *);
Swarm_Operator,   Interaction4	    	SHARE ALL	Propose_Swarm_Mission_Plan;
Mission_Commander, Interaction4       	SHARE ALL 	Confirm_Swarm_Mission_Plan;


//COORDINATE 	$a: Request_UAV_Launch_Permission	FROM Flight_Crew, 
//		$b: Grant_UAV_Launch_Permission		FROM Mission_Commander
//		DO ADD $a PRECEDES $b OD;		//5

ROOT    Interaction5: (* (Request_UAV_Launch_Permission   Grant_UAV_Launch_Permission) *);
Flight_Crew,   Interaction5     	SHARE ALL	Request_UAV_Launch_Permission;
Mission_Commander, Interaction5       	SHARE ALL 	Grant_UAV_Launch_Permission;


//COORDINATE 	$a: Launch_UAV				FROM Flight_Crew, 
//		$b: Launch 				FROM UAV
//		DO ADD $a PRECEDES $b OD;		//6

ROOT    Interaction6: (* (Launch_UAV   Launch) *);
Flight_Crew,   Interaction6     SHARE ALL	Launch_UAV;
UAV, Interaction6       	SHARE ALL 	Launch;


//COORDINATE 	$a: Report_Flight_Status 		FROM UAV, 
//		$b: Assess_Flight_Behavior		FROM Flight_Crew
//		DO ADD $a PRECEDES $b OD;		//7

ROOT    Interaction7: (* (Report_Flight_Status    Assess_Flight_Behavior) *);
UAV,   Interaction7     	SHARE ALL	Report_Flight_Status;
Flight_Crew, Interaction7       SHARE ALL 	Assess_Flight_Behavior;


//COORDINATE 	$a: Stage_UAV_for_Ingress		FROM Flight_Crew, 
//		$b: Confirm_Staged_for_Ingress		FROM UAV
//		DO ADD $a PRECEDES $b OD;		//8a

ROOT    Interaction8a: (* (Stage_UAV_for_Ingress   Confirm_Staged_for_Ingress) *);
Flight_Crew,   Interaction8a    SHARE ALL	Stage_UAV_for_Ingress;
UAV, Interaction8a	      	SHARE ALL 	Confirm_Staged_for_Ingress;


//COORDINATE 	$a: Confirm_Staged_for_Ingress		FROM UAV, 
//		$b: Assign_Ingress_SubSwarmID		FROM Swarm_Operator
//		DO ADD $a PRECEDES $b OD;		//8b

ROOT    Interaction8b: (* (Confirm_Staged_for_Ingress    Assign_Ingress_SubSwarmID) *);
UAV,   Interaction8b     	SHARE ALL	Confirm_Staged_for_Ingress;
Swarm_Operator, Interaction8b   SHARE ALL 	Assign_Ingress_SubSwarmID;


//COORDINATE 	$a: Assign_Ingress_SubSwarmID		FROM Swarm_Operator, 
//		$b: Confirm_Ingress_SubSwarm_ID		FROM UAV
//		DO ADD $a PRECEDES $b OD;		//9

ROOT    Interaction9: (* (Assign_Ingress_SubSwarmID    Confirm_Ingress_SubSwarm_ID) *);
Swarm_Operator, Interaction9  	SHARE ALL	Assign_Ingress_SubSwarmID;
UAV, Interaction9   		SHARE ALL 	Confirm_Ingress_SubSwarm_ID;


//COORDINATE 	$a: Alert_All_UAVs_Staged		FROM Flight_Crew
//		$b: Command_Swarm_to_Commence_Mission	FROM Swarm_Operator
//		DO ADD $a PRECEDES $b OD;		//10a

ROOT    Interaction10a: (* (Alert_All_UAVs_Staged   Command_Swarm_to_Commence_Mission) *);
Flight_Crew,   Interaction10a    SHARE ALL	Alert_All_UAVs_Staged;
Swarm_Operator, Interaction10a	 SHARE ALL 	Command_Swarm_to_Commence_Mission;


//COORDINATE 	$a: Command_Swarm_to_Commence_Mission	FROM Swarm_Operator, 
//		$b: Commence_Mission			FROM UAV
//		DO ADD $a PRECEDES $b OD;		//10b

ROOT    Interaction10b: (* (Command_Swarm_to_Commence_Mission    Commence_Mission) *);
Swarm_Operator, Interaction10b  SHARE ALL	Command_Swarm_to_Commence_Mission;
UAV, Interaction10b   		SHARE ALL 	Commence_Mission;


//COORDINATE 	$a: Assign_Egress_SubSwarmID		FROM Swarm_Operator, 
//		$b: Confirm_Egress_SubSwarm_ID		FROM UAV
//		DO ADD $a PRECEDES $b OD;		//11

ROOT    Interaction11: (* (Assign_Egress_SubSwarmID    Confirm_Egress_SubSwarm_ID) *);
Swarm_Operator, Interaction11  	SHARE ALL	Assign_Egress_SubSwarmID;
UAV, Interaction11   		SHARE ALL 	Confirm_Egress_SubSwarm_ID;


//COORDINATE 	$a: Confirm_Staged_for_Egress 		FROM UAV, 
//		$b: Observe_New_UAV_in_Recovery_List	FROM Flight_Crew
//		DO ADD $a PRECEDES $b OD;		//12

//ROOT    Interaction12: (* (Confirm_Staged_for_Egress    Observe_New_UAV_in_Recovery_List) *);
//UAV,   Interaction12     	SHARE ALL	Confirm_Staged_for_Egress;
//Flight_Crew, Interaction12     	SHARE ALL 	Observe_New_UAV_in_Recovery_List;


//COORDINATE 	$a: Provide_Recovery_Prioritization_List	FROM Swarm_Operator, 
//		$b: Receive_Recovery_Prioritization_List	FROM Flight_Crew
//		DO ADD $a PRECEDES $b OD;		//13

//ROOT    Interaction13: (* (Provide_Recovery_Prioritization_List    Receive_Recovery_Prioritization_List) *);
//Swarm_Operator, Interaction13  	SHARE ALL	Provide_Recovery_Prioritization_List;
//Flight_Crew, Interaction13   	SHARE ALL 	Receive_Recovery_Prioritization_List;


//COORDINATE 	$a: Recover_UAV				FROM Flight_Crew, 
//		$b: Land 				FROM UAV
//		DO ADD $a PRECEDES $b OD;		//14a

//ROOT    Interaction14a: (* (Recover_UAV	   Land) *);
//Flight_Crew,   Interaction14a   SHARE ALL	Recover_UAV;
//UAV, Interaction14a       	SHARE ALL 	Land;


//COORDINATE 	$a: Land 				FROM UAV, 
//		$b: Retrieve_UAV			FROM Flight_Crew
//		DO ADD $a PRECEDES $b OD;		//14b

//ROOT    Interaction14b: (* (Land    Retrieve_UAV) *);
//UAV,   Interaction14b     	SHARE ALL	Land;
//Flight_Crew, Interaction14b     SHARE ALL 	Retrieve_UAV;


//COORDINATE 	$a: Alert_RC_UAV_Landing		FROM Flight_Crew, 
//		$b: Receive_UAV_Landing_Notification	FROM Range_Control
//		DO ADD $a PRECEDES $b OD;		//15

//ROOT Interaction15: (* (Alert_RC_UAV_Landing  Receive_UAV_Landing_Notification) *);
//Flight_Crew, Interaction15    SHARE ALL  Alert_RC_UAV_Landing;
//Range_Control, Interaction15  SHARE ALL  Receive_UAV_Landing_Notification;

ROOT Interaction15: (* (Alert_RC_UAV_Landing Receive_UAV_Landing_Notification) *);
Flight_Crew, Interaction15 SHARE ALL Alert_RC_UAV_Landing;
Range_Control, Interaction15 SHARE ALL Receive_UAV_Landing_Notification;

//COORDINATE 	$a: Confirm_UAVs_Recovered_Retrieved	FROM Flight_Crew, 
//		$b: Conduct_PostMission_Hotwash	FROM Mission_Commander
//		DO ADD $a PRECEDES $b OD;		//16a

ROOT    Interaction16a: (* (Confirm_UAVs_Recovered_Retrieved   Conduct_PostMission_Hotwash) *);
Flight_Crew, Interaction16a     	SHARE ALL	Confirm_UAVs_Recovered_Retrieved;
Mission_Commander, Interaction16a	SHARE ALL 	Conduct_PostMission_Hotwash;


Mission_Commander, Flight_Crew, Swarm_Operator, Safety_Coordinator  	
				SHARE ALL 	Conduct_PostMission_Hotwash;
							//16b
*/