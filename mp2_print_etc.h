/*
 *  mp2_print_subroutines.h
 *
 *  Created by Mike Auguston on 02/05/15.
 *	last modified 03/22/15
 */
//=======================================
// this declaration uses Event_name enum
// it is a single object referred when needed by containers
// does not leave anything on the trace
Empty_producer Dummy(Dummy_event);

//=======================================
// print uses event_name_string[]
//-----------------------------------------
void Composite_producer::harvest(){
	// calls traverse to fill the segments list

	Traversal_result  result;
	int my_total =	0;
	int min_trace = 10000000;
	int max_trace = 0;

	do {
		// reset Stack and relation lists to start new trace assembly 
		Stack.clear();
		Follows.clear();
		Inside.clear();
		Equals.clear();
		tails.clear();
		heads.clear();
		
		// prepare stacks for traversal
		predecessor.clear();
		predecessor.push_back(-1);
		// no predecessor for the event at beginning of a segment
		// it will be brought by containers in add_relations_for_leader()
		
		// add a Composite event instance event to the trace: name, index, and length
		// the length of segment will be adjusted later
		Stack.push_back(new Composite_event_instance(name, segments.size()));	
		Stack[0]->type = target_event;
		result = traverse(); // fills the Stack and relation lists
		// relations are assembled/processed in the container objects
		
		predecessor.pop_back(); // restore ordering for the previous nesting level
		
		if(result == failed){
			if(completeness_count == element_count) break;
								// there are no more options to try
			else continue;
		}
		
		// store the assembled segment, its relation lists and event stats
		segments.push_back(Stack);
		follows_lists.push_back(Follows);
		inside_lists.push_back(Inside);
		equals_lists.push_back(Equals);
		
		// do statistics: for total number of events stored <<<<<<<<<<<<
		int segment_len = Stack.size();
		total_events += segment_len;
		my_total += segment_len;
		if(segment_len < min_trace) min_trace = segment_len;
		if(segment_len > max_trace) max_trace = segment_len;
		storage += (sizeof Stack) + sizeof(Event_producer_ref) * segment_len + 
		3 * sizeof(pair_list) + 
		sizeof (pair<int, int>) * (Follows.size() + Inside.size() + Equals.size());
				
	} while(result != success_and_completed);
	
	if(segments.size())
		cout<<"completed "<<event_name_string[name]<<": \t"<<segments.size()<<" traces \t"<<
			my_total<<" events \n\t\taverage "<<(double)my_total/segments.size()<< 
			" ev/trace \tmin "<< min_trace<< " \tmax "<<max_trace<<endl<<endl;
	else cout<<"no traces found for "<<event_name_string[name]<<endl;
	
}// end harvest()

//-----------------------------------------------------

void Event_producer::print_event(){
	cout<< " Event " << event_name_string[name] << " \ttype= " << event_type_string[type] 
	<<endl;
}

void Composite_event_instance::print_event(){
	cout<< " Event " << event_name_string[name] << " \ttype= " << event_type_string[type] << 
	" index= "<< index<< endl;
}

//------------- debugging print subroutines -----------------
void show_map(pair_list &x){
	for(multimap<int, int>:: iterator q = x.begin(); q != x.end(); q++){
		cout<<" ("<< q->first<<", "<<q->second<<")\n";
	}
	
}
//----------------------------------------------------------
void Composite_producer::show_traces(){
	cout<< "\nTotal "<< segments.size()<< " traces for Composite "<< event_name_string[name] << endl; 
	cout<<"=========================\n";
	for(int k =0; k < segments.size(); k++){
		cout<< "trace #"<< k+1 <<" with " << segments[k].size() << " events\n";
		for(int i = 0; i < segments[k].size(); i++){
			cout<<'('<< i << ") ";
			segments[k][i] ->print_event();
		}
		
		multimap <int, int>:: iterator p;
		cout<<"\n FOLLOWS list for trace #"<< k+1<<endl;
		for(p = follows_lists[k].begin(); p != follows_lists[k].end(); p++){
			cout<< "   "<< p->first<< " follows "<< p->second<<endl;
		}
		
		cout<<"\n IN list for trace #"<< k+1<<endl;
		for(p = inside_lists[k].begin(); p != inside_lists[k].end(); p++){
			cout<< "   "<< p->first<< " inside "<< p->second<<endl;
		}
		
		cout<<"\n EQUALS list for trace #"<< k+1<<endl;
		for(p = equals_lists[k].begin(); p != equals_lists[k].end(); p++){
			cout<< "   "<< p->first<< " equals "<< p->second<<endl;
		}
		
		cout<<endl;
	}
}
//-----------------------------------------------------------
void Composite_producer::output_JSON(){
	string comma, comma2;
	JSON<< "{\"traces\":[" << endl;
	comma2 = "";
	for(int kk =0; kk < segments.size(); kk++){
		JSON<< comma2;
		comma2 = ",\n";
		JSON<< "[[";	// start trace and event list
		
		// in preparation for equality cleaning
		int len = segments[kk].size();
		int matrix_len = len * len;
		char eq_matrx[matrix_len];
		char invalid[len]; // list of invalidated events

		for(int j = 0; j < matrix_len; j++){
			eq_matrx[j] = 0;
		}
		for(int j = 0; j < len; j++){
			invalid[j] = 0;
		}
		invalid[0] = 1;// invalidate the main schema event
		
		multimap <int, int>:: iterator p;
		multimap <int, int>:: iterator q = equals_lists[kk].end();

		// fill eq_matrx
		for(p = equals_lists[kk].begin(); p != q; p++){
			eq_matrx[p->first * len + p->second ] = 
			eq_matrx[p->second * len + p->first ] = 1;
		}
		
		// transitive closure is based on Floyd-Warshall algorithm 
		// [Cormen et al. 3rd Edition,  pp.699]
		//--------------------------------------------------------------
		for(int t = 0; t < len; t++){
			for(int i = 0; i < len; i++){
				for(int j = 0; j < len; j++){
					eq_matrx[i * len + j] = 
						eq_matrx[i * len + j] || 
						(eq_matrx[i * len + t] && eq_matrx[t * len + j]);
				}
			}
		}
		
		// fill the list of invalidated events
		// all but earliest equal are marked by 1
		for(int k = 1; k < len; k++){
			if(invalid[k]) continue;
			for(int i = 1; i < len; i++){
				if(eq_matrx[k * len + i] && k != i){					
					invalid[i] = 1;
				}
			}
		}
		
		// print event list
		comma = "";
		for(int i = 1; i < len; i++){
			if(!invalid[i]){
				JSON<< comma;
				comma = ",";
				JSON<<"[\""<<		// start event pair
					event_name_string[segments[kk][i] ->name]<<"\",\"";
				switch(segments[kk][i] ->type){
					case Composite_event_instance_node: 
						JSON<<'C'; break;
					case Atom: 
						JSON<<'A'; break;
					case ROOT_node: 
						JSON<<'R'; break;
					case Schema_node: 
						JSON<<'S'; break;
					default: JSON<< "unknown event type: "<< segments[kk][i] ->type;
				}
				JSON<<"\","<<i<<"]"; // end event pair
			}
		}
		JSON<<"],\n["; // end event list and start inside list
		
		// print IN relations
		int prev_first, prev_second; // to avoid duplications
		comma = "";
		prev_first = prev_second = -1;
		for(p = inside_lists[kk].begin(); p != inside_lists[kk].end(); p++){
			if(!invalid[p->first] && !invalid[p->second] && 
			   !(p->first == prev_first && p->second == prev_second)){ 
				JSON<< comma;
				comma = ",";
				JSON<< "["<< p->first<< ","<< p->second<<"]";
				prev_first = p->first;
				prev_second = p->second;
			}
		}
		JSON<<"],\n[";	// end inside list and start follows list
		
		// print FOLLOWS relations
		comma = "";
		prev_first = prev_second = -1;
		for(p = follows_lists[kk].begin(); p != follows_lists[kk].end(); p++){
			if(!invalid[p->first] && !invalid[p->second] && 
			   !(p->first == prev_first && p->second == prev_second)){
				JSON<< comma;
				comma = ",";
				JSON<< "["<< p->first<< ","<< p->second<<"]";
				prev_first = p->first;
				prev_second = p->second;
			}
		}
		JSON<<"]]";	// end follows list and trace
	}
	JSON<<"]}"<<endl;
}


