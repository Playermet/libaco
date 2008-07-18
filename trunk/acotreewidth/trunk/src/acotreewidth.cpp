#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <cfloat>
#include <csignal>
#include <tclap/CmdLine.h>
#include <acotreewidth/decomp.h>

static std::string filepath;
static bool hypergraph_flag;
static unsigned int ants = 10;
static unsigned int iterations = UINT_MAX;
static double alpha = 1.0;
static double beta = 1.0;
static double rho = 0.1;
static double initial_pheromone = 0.1;
static unsigned int heuristic = 0;
static unsigned int graph_type = 0;
static bool print_tour_flag = false;
static bool print_stagnation_flag = false;
static double time_limit = DBL_MAX;
static bool simple_as_flag = false;
static bool elitist_as_flag = false;
static bool rank_as_flag = false;
static bool maxmin_as_flag = false;
static bool acs_as_flag = false;
static LocalSearchType local_search = NONE;

static double elitist_weight = 2.0;
static unsigned int ranked_ants = 1;

static unsigned int maxmin_frequency = 5;
static double maxmin_a = 2.0;

static double acs_q0 = 0.5;
static double acs_epsilon = 0.1;

static AntColony<Ant> *colony;

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Ant Colony Optimization for Tree Decomposition", ' ', "0.1");
  TCLAP::ValueArg<unsigned int> ants_arg ("m", "ants", "number of ants", false, ants, "integer");
  TCLAP::ValueArg<unsigned int> iterations_arg ("i", "iterations", "number of iterations", false, iterations, "positive integer");
  TCLAP::ValueArg<double> alpha_arg ("a", "alpha", "alpha (influence of pheromone trails)", false, alpha, "double");
  TCLAP::ValueArg<double> beta_arg("b", "beta", "beta (influence of heuristic information)", false, beta, "double");
  TCLAP::ValueArg<double> rho_arg("r", "rho", "pheromone trail evaporation rate", false, rho, "double");
  TCLAP::ValueArg<double> initial_pheromone_arg("p", "pheromone", "initial pheromone value", false, initial_pheromone, "double");
  std::vector<unsigned int> allowed;
  allowed.push_back(0);
  allowed.push_back(1);
  TCLAP::ValuesConstraint<unsigned int> allowed_values( allowed );
  TCLAP::ValueArg<unsigned int> graph_type_arg("g", "graph", "0: AdjacencyMatrix 1: AdjacencyList", false, graph_type, &allowed_values);
  TCLAP::ValueArg<unsigned int> heuristic_arg("j", "heuristic", "0: min_degree 1: min_fill", false, heuristic, &allowed_values);
  TCLAP::ValueArg<std::string> filepath_arg("f", "file", "path to the graph file", true, "", "filepath");
  TCLAP::SwitchArg hypergraph_arg("", "hypergraph", "input file is a hypergraph");
  TCLAP::SwitchArg print_tour_arg("o", "printord", "print best elimination ordering in iteration");
  TCLAP::SwitchArg print_stagnation_arg("s", "stagnation", "print stagnation measure");
  TCLAP::ValueArg<double> time_limit_arg("t", "time", "terminate after n seconds (after last iteration is finished)", false, time_limit, "double");
  TCLAP::SwitchArg simple_as_arg("", "simple", "use Simple Ant System");
  TCLAP::ValueArg<double> elitist_as_arg("", "elitist", "use Elitist Ant System with given weight", false, elitist_weight, "double");
  TCLAP::ValueArg<unsigned int> rank_as_arg("", "rank", "use Rank-Based Ant System and let the top n ants deposit pheromone", false, ranked_ants, "positive integer");
  TCLAP::SwitchArg maxmin_as_arg("", "maxmin", "use Max-Min Ant System");
  TCLAP::ValueArg<unsigned int> maxmin_frequency_arg("", "maxmin_frequency", "frequency of pheromone updates of best-so-far ant in Max-Min Ant System", false, maxmin_frequency, "double");
  TCLAP::ValueArg<double> maxmin_a_arg("", "maxmin_a", "parameter a in Max-Min Ant System", false, maxmin_a, "double");
  TCLAP::SwitchArg acs_as_arg("", "acs", "use Ant Colony System");
  TCLAP::ValueArg<double> acs_q0_arg("", "acs_q0", "q0 parameter for Ant Colony System", false, acs_q0, "double");
  TCLAP::ValueArg<double> acs_epsilon_arg("", "acs_epsilon", "epsilon parameter for Ant Colony System", false, acs_epsilon, "double");

  std::vector<TCLAP::Arg *> as_variants;
  as_variants.push_back(&simple_as_arg);
  as_variants.push_back(&elitist_as_arg);
  as_variants.push_back(&rank_as_arg);
  as_variants.push_back(&maxmin_as_arg);
  as_variants.push_back(&acs_as_arg);

  TCLAP::SwitchArg no_ls_arg("", "no_ls", "perform no local search");
  TCLAP::SwitchArg it_best_ls_arg("", "it_best_ls", "perform local search only on iteration best ant");
  TCLAP::SwitchArg ls_arg("", "ls", "perform local search on all ants");

  cmd.add(ants_arg);
  cmd.add(iterations_arg);
  cmd.add(alpha_arg);
  cmd.add(beta_arg);
  cmd.add(rho_arg);
  cmd.add(initial_pheromone_arg);
  cmd.add(graph_type_arg);
  cmd.add(heuristic_arg);
  cmd.add(filepath_arg);
  cmd.add(hypergraph_arg);
  cmd.add(print_tour_arg);
  cmd.add(print_stagnation_arg);
  cmd.add(time_limit_arg);
  cmd.add(maxmin_frequency_arg);
  cmd.add(maxmin_a_arg);
  cmd.add(acs_q0_arg);
  cmd.add(acs_epsilon_arg);
  cmd.xorAdd(as_variants);
  cmd.add(no_ls_arg);
  cmd.add(it_best_ls_arg);
  cmd.add(ls_arg);
  cmd.parse(argc, argv);
  ants = ants_arg.getValue();
  iterations = iterations_arg.getValue();
  alpha = alpha_arg.getValue();
  beta = beta_arg.getValue();
  rho = rho_arg.getValue();
  initial_pheromone = initial_pheromone_arg.getValue();
  heuristic = heuristic_arg.getValue();
  graph_type = graph_type_arg.getValue();
  filepath = filepath_arg.getValue();
  hypergraph_flag = hypergraph_arg.getValue();
  print_tour_flag = print_tour_arg.getValue();
  print_stagnation_flag = print_stagnation_arg.getValue();
  time_limit = time_limit_arg.getValue();
  simple_as_flag = simple_as_arg.isSet();
  elitist_as_flag = elitist_as_arg.isSet();
  elitist_weight = elitist_as_arg.getValue();
  rank_as_flag = rank_as_arg.isSet();
  ranked_ants = rank_as_arg.getValue();
  maxmin_as_flag = maxmin_as_arg.isSet();
  maxmin_frequency = maxmin_frequency_arg.getValue();
  maxmin_a = maxmin_a_arg.getValue();
  acs_as_flag = acs_as_arg.isSet();
  acs_q0 = acs_q0_arg.getValue();
  acs_epsilon = acs_epsilon_arg.getValue();

  if(no_ls_arg.isSet()) {
    local_search = NONE;
  } else if(it_best_ls_arg.isSet()) {
    local_search = ITERATION_BEST;
  } else if(ls_arg.isSet()) {
    local_search = ALL;
  }
}

static heuristicf get_heuristic_function() {
  switch(heuristic) {
    case 0:
      return Heuristic::min_degree;
      break;
    case 1:
      return Heuristic::min_fill;
      break;
  }
}

template <class T> OptimizationProblem *get_tree_problem(heuristicf heuristic_function) {
  OptimizationProblem *op;
  T &graph1 = (T &) Parser::parse_dimacs<T>(filepath.c_str());
  op = new TreeDecompProblem<T>(&graph1, heuristic_function);
  return op;
}

template <class T> OptimizationProblem *get_hypertree_problem(heuristicf heuristic_function) {
  OptimizationProblem *op;
  HyperGraph &hypergraph = Parser::parse_hypertreelib(filepath.c_str());
  op = new HyperTreeDecompProblem<T>(&hypergraph, heuristic_function);
  return op;
}

static void set_config(AntColonyConfiguration &config) {
  config.number_of_ants = ants;
  config.alpha = alpha;
  config.beta = beta;
  config.stagnation_measure = print_stagnation_flag;
  config.evaporation_rate = rho;
  config.initial_pheromone = initial_pheromone;
  config.local_search = local_search;
}

AntColony<Ant> *get_ant_colony(OptimizationProblem *problem) {
  AntColony<Ant> *colony;

  if(simple_as_flag) {
    AntColonyConfiguration config;
    set_config(config);
    colony = (AntColony<Ant> *) new SimpleAntColony(problem, config);
  } else if(elitist_as_flag) {
    ElitistAntColonyConfiguration config;
    set_config(config);
    config.elitist_weight = elitist_weight;
    colony = (AntColony<Ant> *) new ElitistAntColony(problem, config);
  } else if(rank_as_flag) {
    RankBasedAntColonyConfiguration config;
    set_config(config);
    config.elitist_ants = ranked_ants;
    colony = (AntColony<Ant> *) new RankBasedAntColony(problem, config);
  } else if(maxmin_as_flag) {
    MaxMinAntColonyConfiguration config;
    set_config(config);
    config.best_so_far_frequency = maxmin_frequency;
    config.a = maxmin_a;
    colony = (AntColony<Ant> *) new MaxMinAntColony(problem, config);
  } else if(acs_as_flag) {
    ACSAntColonyConfiguration config;
    set_config(config);
    config.q0 = acs_q0;
    config.epsilon = acs_epsilon;
    colony = (AntColony<Ant> *) new ACSAntColony(problem, config);
  }
  return colony;
}

static void print_tour(std::vector<unsigned int> tour) {
  for(unsigned int i=0;i<tour.size();i++) {
    std::cout << tour[i] << ((i == (tour.size()-1)) ? "" : ",");
  }
}

static void terminate(int signal) {
  std::cout << std::endl;
  std::cout << "best\tordering" << std::endl;
  std::cout << colony->get_best_tour_length() << "\t";
  print_tour(colony->get_best_tour());
  std::cout << std::endl;
  delete colony;
  exit(0);
}

double timer() {
  static bool initialized_time = false;
  static clock_t time;
  if(!initialized_time) {
    time = clock();
    initialized_time = true;
    return 0.0;
  } else {
    clock_t time_diff = clock() - time;
    double elapsed_time = time_diff * 1.0 / CLOCKS_PER_SEC;
    return elapsed_time;
  }
}

int main(int argc, char *argv[]) {
  signal(SIGINT, terminate);
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(0);
  }

  heuristicf heuristic_function = get_heuristic_function();


  OptimizationProblem *problem;

  try {
    if(hypergraph_flag) {
      switch(graph_type) {
        case 0:
          problem = get_hypertree_problem<AdjacencyMatrixGraph>(heuristic_function);
          break;
        case 1:
          problem = get_hypertree_problem<AdjacencyMapGraph>(heuristic_function);
          break;
      }
    } else {
      switch(graph_type) {
        case 0:
          problem = get_tree_problem<AdjacencyMatrixGraph>(heuristic_function);
          break;
        case 1:
          problem = get_tree_problem<AdjacencyMapGraph>(heuristic_function);
          break;
      }
    }
  } catch(FileNotFoundException e) {
    std::cerr << "error: could not open " << e.what()  << std::endl;
    exit(0);
  }


  colony = get_ant_colony(problem);

  std::cout << "iter\ttime\tbest";
  std::cout << ((local_search != NONE) ? "\tnols" : "");
  std::cout << "\tbest_it";
  std::cout << ((local_search != NONE) ? "\tit_nols" : "");
  std::cout << (print_stagnation_flag ? "\tstagnation" : "");
  std::cout << (print_tour_flag ? "\tordering" : "");
  std::cout << std::endl;
  timer();
  for(int i=0;i<iterations && timer() < time_limit;i++) {
    colony->run();
    std::cout << (i+1) << "\t";
    std::cout << timer() << "\t";
    std::cout << colony->get_best_tour_length() << "\t";
    if(local_search != NONE) {
      std::cout << colony->get_best_tour_length_no_ls() << "\t";
    }
    std::cout << colony->get_best_tour_length_in_iteration() << "\t";

    if(local_search != NONE) {
      std::cout << colony->get_best_tour_length_in_iteration_no_ls() << "\t";
    }
    if(print_stagnation_flag) {
      std::cout << colony->get_stagnation_measure();
    }
    if(print_tour_flag) {
      std::cout << "\t";
      print_tour(colony->get_best_tour_in_iteration());
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "best\tordering" << std::endl;
  std::cout << colony->get_best_tour_length() << "\t";
  print_tour(colony->get_best_tour());
  std::cout << std::endl;
  delete colony;
}