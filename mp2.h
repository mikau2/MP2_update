/*
 *  mp2.h
 *  
 *  Created by Mike Auguston on 2/2/15.
 *  recursive descent trace generation
 *	common declarations and globals
 *
 *	last modified: 03/26/15
 */
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <set>
//#include <algorithm>

using namespace std;

//************************
//***** Statistics
//************************
double storage = 0;		// memory for composite segments
int total_events = 0;	// total events stored

clock_t gen_start,gen_end;	// for time measurement 
double	dif;		// for time interval 

//************************
//***** Globals 
//************************
ofstream JSON; // text file for trace visualization
class Event_producer;
class Composite_producer;

typedef Event_producer *		Event_producer_ref;
typedef Composite_producer *	Composite_producer_ref;
typedef vector <Event_producer_ref> trace_segment;

//***************************************
//*** these are used for traverse() work
//***************************************
trace_segment Stack;
	// Stack used by producers for trace segment construction via traverse()

// to maintain relations lists
typedef multimap <int, int> pair_list; 
	// for storing basic relations IN, FOLLOWS, EQUAL for the segment,
	// matricies for segments are build from these when needed
pair_list Follows; // to store FOLLOWS 
pair_list Inside;  // to store IN 
pair_list Equals;  // to store Equals (result of MAP or SHARE ALL)

// stack accompanying the Stack 
vector <int> predecessor;	// to track PRECEDES relation during traverse()
	// push/pop performed in Composite_producer

//===========================
// for SET_node traverse()
//--------------------------------------------------
// initialized in Composite_producer, similar to predecessor.back()
// used in SET_node_producer_container
// replace predecessor when traversing Sets

// heads and tails store for each Set a list 
//	(branch_begin FOLLOWS node) for heads
//	(forthcoming_node FOLLOWS end-of-branch-event) for tails 
// each is a permanent list for Set traverse()
vector<pair_list> heads;
vector<pair_list> tails;

void find_and_copy_heads(int branch_start, int my_index);
void find_and_copy_tails(int first_branch_start, pair_list &destination);
void copy_from_to(int anchor, pair_list &from_list, pair_list &to_list);
void copy_and_adjust_from_to(int anchor, int adjusted_node, 
							 pair_list &from_list, pair_list &to_list);
bool find_in_heads(int node);

//===================================
// composite producers infrastructure
//===================================
void add_relations_for_leader(); // the subroutine for adding relations for 
		//the leading event in Atomic_producer or Composite_secondary_producer

//********** main list of composite event, root, and schema producers ******
//==========================================================================
map <int, Composite_producer_ref> Composite_address_book;
// for each composite event/root/schema name contains a plain pointer to the 
// producer object with the segment list

void show_map(pair_list &x);// for debugging printouts

//****************************
//	event producer types
//****************************
enum Event_type {Atom, Composite_producer_node, Composite_event_instance_node, 
				Composite_secondary_producer_node, 
				ROOT_node, Schema_node, OR_node, AND_node, SET_node, Empty_node,
				Coordinate_op, ShareAll_op };
		
string event_type_string[] = {	"Atom",	"Composite_producer_node", 
								"Composite_event_instance_node",
								"Composite_secondary_producer_node",
								"ROOT_node", "Schema_node",
								"OR_node", "AND_node", "SET_node", 
								"Empty_node", "Coordinate_op", "ShareAll_op" };

//======= traversal results
enum Traversal_result {failed, success_and_completed, success_and_ready_for_next};
string Traversal_result_string[] = {"failed", "success_and_completed", 
									"success_and_ready_for_next"};
					   
//**************************************
//========= generic classes ============
//**************************************

//********** Event producers ************
//***************************************
class Event_producer{
public:

	int			name;	// event name
	Event_type	type;
	
	// constructor
	Event_producer(int n){ 
		name = n; 
		type = Atom; // default
	}
	
	virtual Traversal_result traverse()=0; // will be overloaded
	
	virtual void hold(){} // redefined for OR_nodes
	
	virtual void print_event();	
};
//--------------------------------------
class Atomic_producer: public Event_producer {
public: 
	// constructor 
	Atomic_producer(int n): Event_producer(n){} 
	
	Traversal_result traverse(){ 
		// add relations for new event
		add_relations_for_leader();
		Stack.push_back(this);

		return success_and_completed;
	}
	
};// end class Atomic_producer

//-----------------------------------------
// for empty alternatives in Alt and Optional containers
// does not leave anything on the trace
class Empty_producer: public Event_producer {
public: 
	// constructor 
	Empty_producer(int n): Event_producer(n){
		type = Empty_node;
	} 
	
	Traversal_result traverse(){ 
		return success_and_completed;
	}
	
};// end class Empty_producer

//-----------------------------------------
class OR_node_producer_container: public Event_producer{
public:
	Event_producer_ref * element; // dynamic array of producer elements
	int element_count;	// length of the element array
	int	current_alternative; // the alternative to try, when there is one
	int previous_alternative; // to hold on the alternative waiting until successors complete
	
	// constructor 
	OR_node_producer_container(int n): Event_producer(n){
		type = OR_node;
		current_alternative = 0;
	} 

	Traversal_result traverse(){ 
		
		previous_alternative = current_alternative;
		Traversal_result  result;
		bool done = false; // to interrupt the for loop when success is reached
		// try to find a valid alternative
		for(int i = current_alternative; (i< element_count) && !done; i++){
			switch(result = (element[i] -> traverse()) ){
				case failed:				continue; // try next alternative
					
				case success_and_completed:	current_alternative++;
											done = true;
											break;
					
				case success_and_ready_for_next: done = true; 
					// retain the current_alternative
			};
		}
		if(result == failed) return failed;
				
		return (current_alternative >= element_count)?	
				(current_alternative = 0, success_and_completed): 
				success_and_ready_for_next;						
	}
	
	void hold(){ //follower in an AND_node may hold advance to the next alternative 
				 // because the follower has not yet completed

		current_alternative = previous_alternative;
		// freez producers in the alternative that will be executed next
		element[current_alternative]->hold();
	}
	
}; // end OR_node_producer_container class

//-----------------------------------------------------------
class AND_node_producer_container: public Event_producer{
	// serves sequence producers
public:
	Event_producer_ref * element; // array of producer elements
	int element_count;	// length of the element array
	Event_type target_event;// set in the generated subclass constructor
	int completeness_count; // for harvest() to detect when no more options remain

	// constructor 
	AND_node_producer_container(int n): Event_producer(n){
		type = target_event = AND_node;
	} 
	
	Traversal_result traverse(){ 
		
		completeness_count = 0;
		for(int i =0; i< element_count; i++){
			
			if(target_event == Schema_node) {
				// before element[i] -> traverse()
				predecessor.back() = -1; // block regular ordering
			}
			switch(element[i] -> traverse()){
				case failed:					if(completeness_count == i) 
													// there are no more options to try
													// let harvest() to stop calling this traverse() again
													completeness_count = element_count;
												return failed; 
				case success_and_completed:		completeness_count++;
												break;
				case success_and_ready_for_next:
					// hold all previous OR_nodes until element[i] completes
												for(int j = 0; j<i; j++) 
													element[j]->hold(); 
			};
		}	
		return (completeness_count == element_count)?	success_and_completed: 
														success_and_ready_for_next;		
	}
	
	void hold(){
		// hold all nodes until next element completes
		for(int j = 0; j < element_count; j++){ 
			element[j]->hold();
			}
	}
}; // end AND_node_producer_container class

//----------------------------------------------------------------------
class SET_node_producer_container: public Event_producer{
	// serves set producers
public:
	Event_producer_ref * element;	// array of producer elements
									// set branches
	int element_count;	// length of the element array
	
	// constructor 
	SET_node_producer_container(int n): Event_producer(n){
		type = SET_node;
	} 
	
	Traversal_result traverse(){ 
		int completeness_count = 0;
		
		//========= begin Set traversal ================
		// prepare heads for set branch traversal
		//==============================================
		int last = predecessor.back(); // current predecessor, value stored on the top of stack
		int first_branch_start = Stack.size();
		pair_list temp_list; // needed to push empty map to heads, 
							 // and to store this Set's tails before moving them to global tails
		set<int> my_tails; // will use in this traverse() and store in tails at the very end
		
		
		int my_index = heads.size(); // position in heads vector
		heads.push_back(temp_list); // start heads[my_index] with empty, will update in situ
		
		// prepare heads for the first branch
		if(last >= 0){
			// this Set is not at any branch begin in a parent Set
			heads[my_index].insert(pair<int, int>(first_branch_start, last) );
		}
		// find the earliest parent and copy to heads[my_index] for this branch
		// this Set is at some branch beginning with a parent Set
		find_and_copy_heads(first_branch_start, my_index);
		// if first_branch_start is found in any previous Set tails, copy it to heads[my_index]
		find_and_copy_tails(first_branch_start, heads[my_index]);
		
		//============================
		// main loop starts here
		//============================
		for(int i =0; i< element_count; i++){
			
			//=================================
			// before element[i] -> traverse()
			//=================================
			predecessor.back() = -1; // block regular ordering
			int forthcoming_event = Stack.size();
			
			if(forthcoming_event > first_branch_start){
				// not the first branch beginning, copy and adjust
				copy_and_adjust_from_to(first_branch_start,	 forthcoming_event, 
										heads[my_index], heads[my_index]);
			}
			
			// copy tails from children Set and delete them, to prevent siblings from using it
			for(int k = 0; k < tails.size(); k++){
				multimap<int, int>:: iterator p = tails[k].find(forthcoming_event);
				multimap<int, int>:: iterator q = tails[k].upper_bound(forthcoming_event);
				if(p != tails[k].end()){ // found a tail that should be added to my_tails
					while(p != q) {
						my_tails.insert(p->second);
						p++;
					}
					tails[k].clear();// done with this Set
				}
			}			
			//===============
			// traverse()
			//===============
			switch(element[i] -> traverse()){
				case failed:					return failed; 
				case success_and_completed:		completeness_count++;
												break;
				case success_and_ready_for_next:
					// hold all previous OR_nodes until element[i] completes
					for(int j = 0; j<i; j++) 
						element[j]->hold(); 
			}
			//===============================
			// after element[i] -> traverse()
			//===============================
			int next_event = Stack.size();
			if( next_event > forthcoming_event ){
				// there was a non-empty contribution by traverse()
				
				// forthcoming_event exists, now can forward heads[my_index]
				// for forthcoming_event to Follows
				// and perform this delayed action
				copy_from_to(forthcoming_event, heads[my_index], Follows);
				
				if(predecessor.back() >= 0){
					my_tails.insert(predecessor.back());
				}
				
				if(i == element_count - 1){// if the last element, do it now
					// copy tails from children Set
					for(int k = 0; k < tails.size(); k++){
						multimap<int, int>:: iterator p = tails[k].find(next_event);
						multimap<int, int>:: iterator q = tails[k].upper_bound(next_event);
						if(p != tails[k].end()){ // found a tail that should be added to my_tails
							while(p != q) {
								my_tails.insert(p->second);
								p++;
							}
						tails[k].clear();// done with this Set
						}
					}
				}
			}
			// else continue element[i+1] -> traverse() with the same heads and tails
		} // end main loop
		
		//============================
		// end of Set generation
		//============================		
		
		// store tails accumulated in my_tails
		int next = Stack.size();
		for(set<int>:: iterator tt = my_tails.begin(); tt != my_tails.end(); tt++){
			temp_list.insert(pair<int, int>(next, *tt) );
		}						
		// finally add to the global tails
		tails.push_back(temp_list);
		
		predecessor.back() = -1; // block regular ordering

		return (completeness_count == element_count)?	success_and_completed: 
														success_and_ready_for_next;		
	}
	
	void hold(){
		// hold all nodes until next element completes
		for(int j = 0; j < element_count; j++){ 
			element[j]->hold();
		}
	}
	
}; // end SET_node_producer_container class

//--------------------------------------------------
// this element sits on the generated event trace, along with Atomic_producers
// generated in the Composite_secondary_producer, 
// added at the beginning of each segment,
// brought from the Composite_producer segments storage
class Composite_event_instance: public Event_producer{
public:
	int index; // fetched segment's index in the segment_storage (version of this composite event)
	
	// constructor
	Composite_event_instance(int n, int indx): Event_producer(n){
		type	= Composite_event_instance_node;
		index	= indx;
	}
	
	void print_event();	// defined in the generated part, 
						// because it needs event type and name strings
	
	// to prevent this class from being abstract, as Event_producer
	Traversal_result traverse(){return success_and_completed;}
	
}; // end Composite_event_instance class

//---------------------------------------------------------------
// is a subclass of generated Composite classes
// this object creates and stores list of composite event instances
// traverse() is called from its harvest() only
class Composite_producer: public AND_node_producer_container {
public:
	// storage for secondary producers
	//===================================
	vector <trace_segment> segments;// composite event trace instance list
	// to store relation lists for the segments
	vector <pair_list>	follows_lists; // to store FOLLOWS 
	vector <pair_list>  inside_lists;  // to store IN 
	vector <pair_list>  equals_lists;  // to store Equals (results of MAP or SHARE ALL)
	
	// constructor
	Composite_producer(int n): AND_node_producer_container(n){
		type = Composite_producer_node;
		// posts a reference to itself on the Composite_address_book
		Composite_address_book.insert(pair<int, Composite_producer_ref>(name, this));
	}
	
	Traversal_result traverse(){ 
		// creates a single trace on the Stack
		return this -> AND_node_producer_container::traverse();
	}
		
	// calls traverse to fill the segments list
	void harvest();		// defined in mp2_print_etc.h
	
	void show_traces(); // defined in mp2_print_etc.h
	
	void output_JSON(); // defined in mp2_print_etc.h
	
}; // end Composite_producer class

//-------------------------------------------------------
// sits in the recursive descent graph
// used during the recursive descent to traverse composite storage 
// and to fetch the next composite segment
// previous_index shows the position of segment added to the trace
class Composite_secondary_producer: public Event_producer{
public:
	Composite_producer_ref segment_storage;// ptr to segment info stored in Composite_producer
	int index; // segment info index in the segment_storage to fetch now
	int previous_index; // for hold() implementation
	
	// constructor
	Composite_secondary_producer(int n): Event_producer(n){
		type = Composite_secondary_producer_node;
		
		// find segment list storage
		map <int, Composite_producer_ref> :: iterator p;
		p = Composite_address_book.find(name);
		if(p == Composite_address_book.end()){
			cout<< "Composite_secondary_producer constructor cannot find segment storage for the\n";
			print_event();
			segment_storage = NULL;
		}
		else segment_storage = p-> second;
		index = 0;
		previous_index = 0;
	}
	
	Traversal_result traverse(){
		if(!segment_storage) {
			// add relations for new event and add it
			add_relations_for_leader();
			Stack.push_back(new Composite_event_instance(name, 0));
			return success_and_completed;
		}
		
		// we are going to fetch this segment from the storage
		trace_segment * my_segment_ptr = & ((segment_storage->segments)[index]);
					
		// prepare for added segement scanning for relation update
		int base = Stack.size(); //master Composite_event_instance event's position in the trace
		// base is the position of composite event inside which the segment belongs

		// fetch a valid alternative and adjust relation list
		add_relations_for_leader();// add IN/PRECEDES for the composite event at the beginning of segment
		Stack.insert(Stack.end(), my_segment_ptr->begin(), my_segment_ptr->end() );
						
		// get the relation lists from the storage, adjust them with base, 
		// and add to Follows, Inside and Equals
		multimap <int, int>:: iterator p;
		pair_list * my_rel_list_ptr = & ((segment_storage->follows_lists)[index]);
		
		for(p = my_rel_list_ptr-> begin(); p != my_rel_list_ptr-> end(); p++){
			// scan the list of pairs and add them to Follows adjusted with base
			Follows.insert(pair<int, int>( (p->first)	+ base,
										   (p->second)	+ base ) );
		}
		
		my_rel_list_ptr = & ((segment_storage->inside_lists)[index]);
		for(p = my_rel_list_ptr-> begin(); p != my_rel_list_ptr-> end(); p++){
			// scan the list of pairs and add them to Follows adjusted with base
			Inside.insert(pair<int, int>(	(p->first)	+ base,
											(p->second)	+ base ) );
		}
		
		my_rel_list_ptr = & ((segment_storage->equals_lists)[index]);
		for(p = my_rel_list_ptr-> begin(); p != my_rel_list_ptr-> end(); p++){
			// scan the list of pairs and add them to Follows adjusted with base
			Equals.insert(pair<int, int>(	(p->first)	+ base,
											(p->second)	+ base ) );
		}
		previous_index = index;
		index++;
				
		return (index >= (segment_storage->segments).size() )?	
							(index = 0, success_and_completed): 
							success_and_ready_for_next;						
	}
	
	void hold(){ 
		//follower in an AND_node may hold advance to the next alternative 
		// because the follower has not yet completed
		index = previous_index;
		}
	
}; // end Composite_secondary_producer class
//---------------------------------------------------------------------------

class Coordinate: public Event_producer {
public:
	// these arrays are 2-dimensional, 
	// and are allocated/deleted in generated traverse()
	// by create_matrices() as Stack.size() * Stack.size()
	char * eq_matrix;
	char * in_matrix;
	char * follows_matrix;
	int len; // matrix dimension
	int matrix_len; // len * len
	
	// constructor
	Coordinate(int n): Event_producer(n){
		type = Coordinate_op;
		eq_matrix = in_matrix = follows_matrix = 0;
		len = matrix_len = 0;
	} 
	
	// transitive closures are based on Floyd-Warshall algorithm 
	// [Cormen et al. 3rd Edition,  pp.699]
	//--------------------------------------------------------------
	void eq_transitive_closure(char * m){
		for(int k = 0; k < len; k++){
			for(int i = 0; i < len; i++){
				for(int j = 0; j < len; j++){
					m[i * len + j] = 
						m[i * len + j] || (m[i * len + k] && m[k * len + j]);
				}
			}
		}
	} // end eq_transitive_closure(char * m)
	
	void in_transitive_closure(char * m){
		// merge equal event rows
		for(int i = 0; i < len; i++){
			for(int j = 0; j < len; j++){
				if(eq_matrix[i * len + j] && (i != j)){
					for(int k = 0; k < len; k++){
						m[i * len + k] = 
						m[j * len + k] =
							m[i * len + k] || m[j * len + k];
					}
				}
			}
		}
		// do the closure
		for(int k = 0; k < len; k++){
			for(int i = 0; i < len; i++){
				for(int j = 0; j < len; j++){
					m[i * len + j] = 
					m[i * len + j] || (m[i * len + k] && m[k * len + j]);
				}
			}
		}
		// check for loops: axioms 5-6
		for(int i = 0; i < len; i++){
			if(m[i * len + i]) throw failed;
		}
	} // end in_transitive_closure(char * m)
	
	void fw_transitive_closure(char * m){
		// merge equal event rows
		for(int i = 0; i < len; i++){
			for(int j = 0; j < len; j++){
				if(eq_matrix[i * len + j] && (i != j)){
					for(int k = 0; k < len; k++){
						m[i * len + k] = 
						m[j * len + k] =
						m[i * len + k] || m[j * len + k];
					}
				}
			}
		}
		// propagate FOLLOWS to the inner events
		for(int i = 0; i < len; i++){
			for(int j = 0; j < len; j++){
				// distributivity axioms 9-10
				if(in_matrix[i * len + j]){
					for(int k = 0; k < len; k++){
						m[k * len + i] = 
						m[k * len + i] || m[k * len + j];
					}
					for(int k = 0; k < len; k++){
						m[i * len + k] = 
						m[i * len + k] || m[j * len + k];
					}
				}
			}
		}
		// do the closure
		for(int k = 0; k < len; k++){
			for(int i = 0; i < len; i++){
				for(int j = 0; j < len; j++){
					m[i * len + j] = 
					m[i * len + j] || (m[i * len + k] && m[k * len + j]);
				}
			}
		}
		// check for loops: axioms 5-6
		for(int i = 0; i < len; i++){
			if(m[i * len + i]) throw failed;
		}
		// check for mutual exclusion: axioms 1-4
		for(int k = 0; k < len; k++){
			for(int i = 0; i < len; i++){
				if(in_matrix[k * len + i] && 
				   follows_matrix[k * len + i]) throw failed; 
			}
		}
	} // end fw_transitive_closure(char * m)

	void delete_matrices(){
		if(in_matrix){
			delete [] eq_matrix;
			delete [] in_matrix;
			delete [] follows_matrix;
			eq_matrix = in_matrix = follows_matrix = 0;
		}
	}
	
	void create_matrices(){
		len =			Stack.size();
		matrix_len =	len * len;
		
		// allocate and initialize with 0
		eq_matrix =			new char [matrix_len];
		in_matrix =			new char [matrix_len];
		follows_matrix =	new char [matrix_len];
		for(int i = 0; i < matrix_len; i++){
			eq_matrix[i] =		
			in_matrix[i] =		
			follows_matrix[i] = 0;
		}
		
		multimap <int, int>:: iterator p;
		
		// fill eq matrix
		for(p = Equals.begin(); p != Equals.end(); p++){
			eq_matrix[p->first * len + p->second ] = 
			eq_matrix[p->second * len + p->first ] = 1;
		}
		eq_transitive_closure(eq_matrix);
		
		// fill in matrix
		for(p = Inside.begin(); p != Inside.end(); p++){
			in_matrix[p->first * len + p->second ] = 1;
		}
		in_transitive_closure(in_matrix);
		
		// fill follows matrix
		for(p = Follows.begin(); p != Follows.end(); p++){
			follows_matrix[p->first * len + p->second ] = 1;
		}		
		fw_transitive_closure(follows_matrix);
		
	} // end create_matrices()
		
	void sort_events(vector<int> &L){
		// sorts vector of events by FOLLOWS
		// Selection Sort, Levitin p.99 (OK for small lists)
		// since FOLLOWS is partial order, the sort is topological
		// although selection sort is not stable
		if(L.size() < 2) return;
		int min, temp;

		for(int i = 0; i < L.size() - 1; i++){
			min = i;
			for(int j = i + 1; j < L.size(); j++){
				if(follows_matrix[L[min] * len + L[j]]){
					min = j;
				}
			}
			temp = L[i];
			L[i] = L[min];
			L[min] = temp;
		}		
	}
	
	void sort_and_check_coordinated_events(vector<int> &L){
		// for synchronous COORDINATE
		// sorts vector of events by FOLLOWS
		// then checks for total ordering
		if(L.size() < 2) return;
		sort_events(L);
		
		// check total ordering
		for(int i = 0; i < L.size() - 1; i++){
			if(!follows_matrix[L[i + 1] * len + L[i]]) throw failed;
		}		
	}
	
	void make_equality_complete(int a, int b){
		// add to Equals, but earlier position in Stack is more equal :-)
		// copies all Follows and Inside from b to a
		// and do the same for all inner events
		if(a == b) return;
		if(Stack[a]->name != Stack[b]->name) throw failed;
		if(Stack[a]->type == Composite_event_instance_node && 
		   Stack[b]->type == Composite_event_instance_node &&
		   ( ((Composite_event_instance *)Stack[a])->index != 
			 ((Composite_event_instance *)Stack[b])->index ) )throw failed;		
		
		// earlier position in Stack is more equal :-)
		if(a > b) {int t = a; a = b; b = t;}
		Equals.insert(pair<int, int>(a, b));
		// copy all Follows and Inside from b to a
		multimap<int, int>:: iterator p;
		
		for(p = Follows.begin(); p != Follows.end(); p++) {
			if(p->second == b)
				Follows.insert(pair<int, int>(p->first, a));
			if(p->first == b)
				Follows.insert(pair<int, int>(a, p->second));		
		}
		
		for(p = Inside.begin(); p != Inside.end(); p++) {
			if(p->first == b)
				Inside.insert(pair<int, int>(a, p->second));
		}

		if(Stack[a]->type == Atom) return;
		
		// if composite event, do the same for all inner events
		vector<int> a_list, b_list;
		
		p = Inside.begin();
		while(p != Inside.end()) {
			if(p->second == a) a_list.push_back(p->first);
			if(p->second == b) b_list.push_back(p->first);
			p++;
		}
		if(a_list.size() != b_list.size()) throw failed;
		sort_events(a_list);
		sort_events(b_list);
		for(int i = 0; i < a_list.size() && i < b_list.size(); i++){
			make_equality_complete(a_list[i], b_list[i]);
		}		
	}// end make_equality_complete()
	
	void print_matrix(char * m){
		cout<<" size: "<<len<< " * "<<len<<endl;
		cout<< " \t";
		for(int k = 0; k < len; k++){
			cout<<k<<" \t";
		}
		for(int i= 0; i < len; i++){
			cout<<"\n"<<i<<": \t";
			for(int j = 0; j <len; j++){
				cout<<(int)m[i * len + j]<<" \t";
			}
		}
	}
	
	void print_matrices(){
		cout<<"\n\neq_matrix ";
		print_matrix(eq_matrix);
		cout<<"\n\nin_matrix ";
		print_matrix(in_matrix);
		cout<<"\n\nfollows_matrix ";
		print_matrix(follows_matrix);
	}
	
	// traverse() is defined in the generated subclasses
	
}; // end Coordinate class

//========== auxiliary subroutines and declarations =========================

inline void add_relations_for_leader(){
// add relations for the event which will be in a moment pushed on the top of Stack
// called from Atomic_producer or Composite_secondary_producer
// when they add to the trace
	
	int myindex = Stack.size(); // index of this event in the segment,
								// before the event is actually pushed on the stack
	
	// always inside the master composite of the segment
	// will be further adjusted in Composite_secondary_producer traverse()
	Inside.insert(pair<int, int>(myindex, 0));

	// add PRECEDES according to the position in AND_node_producer_container where it belongs
	int last = predecessor.back(); // value stored on the top of stack
	if(last >= 0) { // if predecessor is defined
		Follows.insert(pair<int, int>(myindex, last) );
	}
	else{// happens seldom, only inside Set, or at the beginning of Composite
		// if heads have this node, do nothing, it will be taken care in Set, 
		// otherwise, if tails have this node, add it to Follows
		if( !find_in_heads(myindex)) find_and_copy_tails(myindex, Follows);
	}
	predecessor.back() = myindex; // place this event as a previous
}

//---------------------------------------------------------------------------
inline void copy_from_to(int anchor, pair_list &from_list, pair_list &to_list){
	// called from SET_node_producer_container.traverse()
	multimap<int, int>:: iterator q = from_list.find(anchor);
	if(q != from_list.end()){
		do {
			to_list.insert(pair<int, int>(anchor, q->second) );
			q++;
		} while ( q != from_list.upper_bound(anchor));
	}
}
//----------------------------------------------------------------------------
inline void copy_and_adjust_from_to(int anchor, int adjusted_node, 
									pair_list &from_list, pair_list &to_list){
	// called from SET_node_producer_container.traverse()

	multimap<int, int>:: iterator p = from_list.find(anchor);
	multimap<int, int>:: iterator q = from_list.upper_bound(anchor);					
	if(p != from_list.end()) {
		set<int> temp;
		// collect all predecessors of anchor in from_list
		do {
			temp.insert( p->second);
			p++;
		} while ( p != q );
		// put them into to_list for adjusted_node
		for(set<int>:: iterator tt = temp.begin(); tt != temp.end(); tt++){
			to_list.insert(pair<int, int>(adjusted_node, *tt) );
		}						
	}	
}
//-----------------------------------------------------------------------------
void find_and_copy_heads(int branch_start, int my_index){
	// called from SET_node_producer_container.traverse()
	// find the first parent Set that contains branch_start
	// and copy all branch_start -> N to heads[my_index]
	for(int i = 0; i < my_index; i++){
		multimap<int, int>:: iterator p = heads[i].find(branch_start);
		multimap<int, int>:: iterator q = heads[i].upper_bound(branch_start);
		if(p != heads[i].end()){ // found a parent Set
			while(p != q) {
				heads[my_index].insert(pair<int, int>(branch_start, p->second) );
				p++;
			}
			return;
		}	
	}
}
//-------------------------------------------------------------------------------
void find_and_copy_tails(int node, pair_list &destination){
	// called from SET_node_producer_container.traverse() and add_relations_for_leader()
	// if node is found in any previous Set tails, copy it to destination
	for(int i = 0; i < tails.size(); i++){
		multimap<int, int>:: iterator p = tails[i].find(node);
		multimap<int, int>:: iterator q = tails[i].upper_bound(node);
		if(p != tails[i].end()){ // found a match
			while(p != q) {
				destination.insert(pair<int, int>(node, p->second) );
				p++;
			}
			// now can discard it
			tails[i].clear();
		}	
	}
	return;	
}
//--------------------------------------------------------------------------------
bool find_in_heads(int node){
	// called from add_relations_for_leader()
	for(int i = 0; i < heads.size(); i++){
		multimap<int, int>:: iterator p = heads[i].find(node);
		if(p != heads[i].end())  return true;// found a match
	}
	return false;		
}
//=======================================================================================
void show_statistics(){
	/*
	cout<<"\n*** Statistics **************************************\n";
	cout<<"size of Event_producer = "<< sizeof(Event_producer)<<endl;
	cout<<"size of OR_node_producer_container = "<< sizeof(OR_node_producer_container)<<endl;
	cout<<"size of AND_node_producer_container = "<< sizeof(AND_node_producer_container)<<endl;
	cout<<"size of SET_node_producer_container = "<< sizeof(SET_node_producer_container)<<endl;
	cout<<"size of Composite_producer = "<< sizeof(Composite_producer)<<endl;
	cout<<"size of Composite_secondary_producer = "<< sizeof(Composite_secondary_producer)<<endl;
	cout<<"size of Coordinate = "<< sizeof(Coordinate)<<endl;
	cout<<"   *** These are stored on the trace\n";
	cout<<"size of Atomic_producer = "<< sizeof(Atomic_producer)<<endl;
	cout<<"size of Composite_event_instance = "<< sizeof(Composite_event_instance)<<endl;
	 */
	cout<<"\ntotal number of stored events= "<< total_events;
	cout<< "\nmemory of composite storage= "<< storage  << " bytes/ "<<
	(double)storage /1024<< "KB/ "<< (double)storage /1024/1024<< "MB (JEDEC binary definition)"<<endl;
	
	//gen_end = clock(); 
	dif = double(gen_end - gen_start) / CLOCKS_PER_SEC;
	cout<<"Elapsed time "<< dif<<" sec, Speed: "<<  total_events/dif  <<" events/sec\n"<<endl; 
		
}
