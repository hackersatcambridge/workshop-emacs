//Author: Max Vigdorchik

#include "experiments.h"
#include <cstring>

#define NO_SCREEN_OUT 
Population *lander_test(int gens, char* startgene, bool checkpoint)
{
    Population *pop=0;
    Genome *start_genome;
    char curword[20];
    int id;
    
    ostringstream *fnamebuf;
    int gen;

    int expcount;
    int status;
    int runs[NEAT::num_runs];
    int totalevals;
    int samples;  //For averaging

    memset (runs, 0, NEAT::num_runs * sizeof(int));
    
    cout<<"START LANDER EVOLUTION"<<endl;

    cout<<"Reading in the start genome"<<endl;
    //Read in the start Genome
    ifstream iFile(startgene,ios::in);
    iFile>>curword;
    iFile>>id;
    cout<<"Reading in Genome id "<<id<<endl;
    start_genome = new Genome(id, iFile);
    iFile.close();
    
    for(expcount=0; expcount < NEAT::num_runs; expcount++)
    {
	cout<<"Experiment #"<<expcount<<endl;

	cout<<"Start Genome: "<<start_genome<<endl;

	cout<<"Spawning Population off Genome"<<endl;

	if(checkpoint)
	    pop = new Population(start_genome, NEAT::pop_size, 0.5);
	else
	    pop = new Population(start_genome, NEAT::pop_size);

	cout<<"Verifying Population"<<endl;
	pop->verify();

	for(gen = 1; gen <= gens; gen++)
	{
	    cout<<"generation "<<gen<<endl;

	    fnamebuf = new ostringstream();
	    (*fnamebuf)<<"gen_"<<gen<<ends;

#ifndef NO_SCREEN_OUT
	    cout<<"name of fname: "<<fnamebuf->str()<<endl;
#endif

	    char temp[50];
	    sprintf(temp, "gen_%d", gen);

	    status = lander_epoch(pop,gen,temp);

	    if(status)
	    {
		runs[expcount] = status;
		gens = gens + 1;
	    }

	    fnamebuf->clear();
	    delete fnamebuf;
	}
	if(expcount < NEAT::num_runs - 1) delete pop;
    }

    totalevals = 0;
    samples = 0;
    for(expcount = 0; expcount < NEAT::num_runs; expcount++)
    {
	cout<<runs[expcount]<<endl;
	if(runs[expcount] > 0)
	{
	    totalevals += runs[expcount];
	    samples++;
	}
    }

    cout<<"Failures: "<<(NEAT::num_runs-samples)<<" out of "<<NEAT::num_runs<<" runs"<<endl;
    cout<<"Average evals: "<<(samples > 0 ? (double)totalevals/samples : 0)<<endl;

    return pop;
}

bool lander_evaluate(Organism *org)
{
    Network *net;

    int numnodes;  /* Used to figure out how many nodes
		      should be visited during activation */
    int thresh;  /* How many visits will be allowed before giving up 
		    (for loop detection) */
    double fit1 = 0; //Used to save intermediate result allowing running twice

    //int MAX_STEPS = 100000;
  
    net = org->net;
    numnodes = ((org->gnome)->nodes).size();
    thresh=numnodes*2;  //Max number of visits allowed per activation, outdated and unused.
  
    fit1 = go_lander(net, thresh); //Run twice and take min result, avoids instability.
    org->fitness = std::min(go_lander(net, thresh), fit1); 

#ifndef NO_SCREEN_OUT
    cout<<"Org "<<(org->gnome)->genome_id<<" fitness: "<<org->fitness<<endl;
#endif

    //Decide if its a winner, make sure this coincides with changes to fitness function
    if (org->fitness > 200) { 
	org->winner=true;
	return true;
    }
    else {
	org->winner=false;
	return false;
	}  
}

double go_lander(Network *net, int thresh) //The threshold parameter is not relevant, relic of old ver.
{
    double fitness;
    double in[9];
    double max_time = 10000;
    bool out_of_time = false; //Time limit to prevent it getting stuck in orbit or escape.
    scenario = 1; //Set which scenario to run here. Alternatively, add a loop to run multiple scenarios
    reset_simulation();
    vector<NNode*>::iterator out_iter;
    //To convert to old network trainer, uncomment the orientation inputs, change parachute status
    //to input 11, and remove the last 3 orientation outputs.
    while(!landed && (!out_of_time)) //Essentially run update_lander
    {
	update_closeup_coords();
	last_position = position;
	numerical_dynamics();
	//AI Section!
	in[0] = 1.0; //Constant bias input
	in[1] = position.x/100000;
	in[2] = position.y/100000;
	in[3] = position.z/100000;
	in[4] = velocity.x/10;
	in[5] = velocity.y/10;
	in[6] = velocity.z/10;
	in[7] = fuel;
	//in[8] = orientation.x; //orientation is there for old version of neural net
	//in[9] = orientation.y;
	//in[10] = orientation.z;
	in[8] = parachute_status;

	net->load_sensors(in);
	if(!(net->activate())) return 0; //fitness of 0 if net fails to run
	out_iter = net->outputs.begin(); //For now just a single output
	throttle = (*out_iter)->activation;
	++out_iter;
	parachute_status = static_cast<parachute_status_t>((*out_iter)->activation > 0.9);
	++out_iter;
	orientation.x = (*out_iter)->activation * 360; //allows angles between 0 and 360
	++out_iter;
	orientation.y = (*out_iter)->activation * 360;
	++out_iter;
	orientation.z = (*out_iter)->activation * 360;

	if(stabilized_attitude) attitude_stabilization();

	update_visualization(); //Important to check if landed and adjust fuel
	out_of_time = simulation_time > max_time; //Consider changing this time limit
    }
    //Control fitness function here, most important part of the learning process
    if(out_of_time) 
	fitness = 0; //This needs a much better function to reward reaching the surface.
    if(!crashed) //The extra velocity term here encourages landing safely with 0.5 m/s instead of 1.
	fitness = 200 + fuel * FUEL_CAPACITY + std::min(2.0, 1/velocity.abs()); 
    else //Consider punishing a destroyed parachute
    {
        //Punish landing velocity, so slower is closer to success. (-ve) values not allowed.
	//fitness = 1/velocity.abs(); //Non-linear, really slows down training doing it this way.
	fitness = std::max(0.0, 200.0 - velocity.abs()); //This version has a linear improvement
	
    }
    return fitness;
}

int lander_epoch(Population *pop, int generation, char *filename)
{
    vector<Organism*>::iterator curorg;
    vector<Species*>::iterator curspecies;

    bool win=false;
    int winnernum;

    //Evaluate each organism on a test
    for(curorg = (pop->organisms).begin(); curorg != (pop->organisms).end(); ++curorg) {
	if (lander_evaluate(*curorg)) win=true;
    }

    static double max_fitness = 0;
    bool new_record = false;
    for(curorg = (pop->organisms).begin(); curorg != (pop->organisms).end(); ++curorg)
    {
	if((*curorg)->fitness > max_fitness)
	{
	    max_fitness = (*curorg)->fitness;
	    new_record = true;
	}
    }
    
    //Average and max their fitnesses for dumping to file and snapshot
    for(curspecies = (pop->species).begin(); curspecies != (pop->species).end(); ++curspecies) {

	//This experiment control routine issues commands to collect ave
	//and max fitness, as opposed to having the snapshot do it, 
	//because this allows flexibility in terms of what time
	//to observe fitnesses at

	(*curspecies)->compute_average_fitness();
	(*curspecies)->compute_max_fitness();
    }

    //Only print to file every print_every generations, or if its a winner that broke a record.
    if  ((win && new_record) || ((generation%(NEAT::print_every)) == 0))
	pop->print_to_file_by_species(filename);

    if (win) {
	for(curorg = (pop->organisms).begin(); curorg != (pop->organisms).end(); ++curorg) {
	    if ((*curorg)->winner) {
		winnernum=((*curorg)->gnome)->genome_id;
		cout<<"WINNER IS #"<<((*curorg)->gnome)->genome_id<<endl;
	    }
	}    
    }

    //Create the next generation
    pop->epoch(generation);

    if (win) return ((generation-1) * NEAT::pop_size + winnernum);
    else return 0;

}

